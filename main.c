#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>
#include <semaphore.h>
#include <math.h>
#include <complex.h>
#include <liquid/liquid.h>
#include <poll.h>
#include <unistd.h>

//#include "data_proc.h"
#include "queue.h"
#include "common_func.h"

#define MAX_CLASSIF 8
#define MAX_NAME_LEN 32
#define BUFFER_SIZE 50 //otherwise known as queue size
#define PACKET_SIZE 256

// General Variables
int i, j, k;

// filter stuff
struct Queue *ax;
struct Queue *ay;
struct Queue *az;
iirfilt_crcf filter_ax;
iirfilt_crcf filter_ay;
iirfilt_crcf filter_az;
float complex filter_in;
float complex filter_out;
struct filter_options {
  unsigned int order; // filter order
  float fc; // cutoff frequency
  float f0; // center frequency
  float Ap; // pass-band attenuation
  float As; // stop-band attenuation
  liquid_iirdes_filtertype ftype; // filter type
  liquid_iirdes_bandtype btype; // band type
  liquid_iirdes_format format; // coefficient format
};
struct point_3D {
  float x_comp;
  float y_comp;
  float z_comp;
};
struct point_3D acc_bias;

// flags/ options
int bias_init_flag;
int bias_flag;
int run_flag = 0;

// pipe stuff
int pipe_stm2bb[2];
int pipe_acc_stream[2];
int pipe_ctrl2bb[2];

// process id's
int pid_BLE = 123;
int pid_DSP = 123;
int pid_ML_tr = 123;
int pid_ML_te = 123;


////////////////////////////////////////////////////////////////////
// Initialization Functions
////////////////////////////////////////////////////////////////////
int init_BLE(){
  // run BASH script to setup BLE connection
  int pid_setup = 123;
  pid_setup = fork();
  if (pid_setup == 0)
    execv("./bctl_auto.sh", NULL);
  else
    wait(NULL);
}

int init_pipes(){
  // create pipe from sensor to computer
  pipe(pipe_stm2bb);
  pipe(pipe_acc_stream);
  pipe(pipe_ctrl2bb);
}

int init_queue(){
  // initialize all queues
  ax = createQueue(BUFFER_SIZE);
  ay = createQueue(BUFFER_SIZE);
  az = createQueue(BUFFER_SIZE);
}

int init_filter(){
  // set filter parameters (butterworth?)
  struct filter_options butter;
  butter.order = 2;
  butter.fc = 0.4;
  butter.f0 = 0.0f;
  butter.Ap = 3.0f;
  butter.As = 60.0f;
  butter.ftype = LIQUID_IIRDES_BUTTER;
  butter.btype = LIQUID_IIRDES_LOWPASS;
  butter.format = LIQUID_IIRDES_SOS;

  // create filters
  filter_ax = iirfilt_crcf_create_prototype(butter.ftype, butter.btype, butter.format, butter.order, butter.fc, butter.f0, butter.Ap, butter.As);
  filter_ay = iirfilt_crcf_create_prototype(butter.ftype, butter.btype, butter.format, butter.order, butter.fc, butter.f0, butter.Ap, butter.As);
  filter_az = iirfilt_crcf_create_prototype(butter.ftype, butter.btype, butter.format, butter.order, butter.fc, butter.f0, butter.Ap, butter.As);

  // initialize moving average (will start at 10th BLE packet in stream)
  bias_init_flag = 0;
  bias_flag = 0;
  acc_bias.x_comp = 0.0f;
  acc_bias.y_comp = 0.0f;
  acc_bias.z_comp = 0.0f;

}

////////////////////////////////////////////////////////////////////
// Process 1: Raw Data Collection (BLE)
////////////////////////////////////////////////////////////////////
int BLE_driver(){
  pid_BLE = fork();
  if (pid_BLE == 0){
    close(pipe_stm2bb[0]);
    dup2(pipe_stm2bb[1], STDOUT_FILENO);

    char MAC_addr [256];
    FILE *bctl_file = fopen("bctl.txt", "r");
    while(fgets(MAC_addr, sizeof(MAC_addr), bctl_file) != NULL);
    fclose(bctl_file);
    MAC_addr[17] = '\0'; // remove the newline at end

    const char* suffix[9] =
    {
      "/usr/bin/gatttool",
      "-b",
      "-t",
      "random",
      "--char-write-req",
      "--handle=0x0012",
      "--value=0100",
      "--listen"
    };
    execl("/usr/bin/gatttool", suffix[0], suffix[1], MAC_addr, suffix[2], suffix[3], suffix[4], suffix[5], suffix[6], suffix[7], (char *) NULL);
    return -1;
  }
  else{
    close(pipe_stm2bb[1]);
    return 0;
  }
}

////////////////////////////////////////////////////////////////////
// Process 2: Raw Data Filtering and Storage (DSP)
////////////////////////////////////////////////////////////////////
int parse_and_filter(char raw[PACKET_SIZE], struct point_3D *acc_data){
  int offset = 36; // skip everything before the :
  char *data_off = raw + offset;

  // setup lock iter ptr
  int iter = 0;
  char data[41] = "";
  while(iter <= 40 && *data_off != '\0' && *data_off != '\n'){
    if(*data_off != ' '){
      data[iter] = *data_off;
      ++iter;
    }
    ++data_off;
  }

  // we are only interested in the first 16 hex values
  for (iter = 0; iter < 16; iter += 4){
    switch(iter){
      case 4:
        filter_in = (float)(hex_to_decimal_4bit(&data[iter]));
        iirfilt_crcf_execute(filter_ax, filter_in, &filter_out);
        if (bias_flag){
          acc_data->x_comp = creal(filter_out) - acc_bias.x_comp;
        }
        else{
          acc_data->x_comp = creal(filter_out);
        }
        break;
      case 8:
        filter_in = (float)(hex_to_decimal_4bit(&data[iter]));
        iirfilt_crcf_execute(filter_ay, filter_in, &filter_out);
        if (bias_flag)
          acc_data->y_comp = creal(filter_out) - acc_bias.y_comp;
        else
          acc_data->y_comp = creal(filter_out);
        break;
      case 12:
        filter_in = (float)(hex_to_decimal_4bit(&data[iter]));
        iirfilt_crcf_execute(filter_az, filter_in, &filter_out);
        if (bias_flag)
          acc_data->z_comp = creal(filter_out) - acc_bias.z_comp;
        else
          acc_data->z_comp = creal(filter_out);
        break;
    }
  }

  return 0;
}

int DSP_driver(){
  pid_DSP = fork();
  if (pid_DSP == 0){
    close(pipe_acc_stream[0]);
    close(pipe_ctrl2bb[1]);
    struct pollfd poll_ctrl;
    poll_ctrl.fd = pipe_ctrl2bb[0];
    poll_ctrl.events = POLLIN;

    char raw_data[PACKET_SIZE] = "";
    FILE *data_stream = fdopen(pipe_stm2bb[0], "rb");

    // skip over the first line
    while (!fgets(raw_data, PACKET_SIZE, data_stream));

    int q_size = 0;
    int periodic_report_flag = 0;
    int batch_report_flag = 0;
    struct point_3D LPF_data_acc;
    int training_size, count;
    int bias_sample_init = 15 < BUFFER_SIZE ? 15 : BUFFER_SIZE-1;
    int bias_sample_end = 30 < BUFFER_SIZE ? 30 : BUFFER_SIZE;
    FILE *output_file;
    while (1){
      //parse and filter sensor_data with enqueue
      while (!fgets(raw_data, PACKET_SIZE, data_stream));
      parse_and_filter(raw_data, &LPF_data_acc);

      poll(&poll_ctrl, 1, 0);
      if (poll_ctrl.revents & POLLIN){
        char mssg[256] = "";
        read(pipe_ctrl2bb[0], mssg, 256);
        training_size = atoi(mssg);

        if (training_size == -1)
          periodic_report_flag = 1;
        else
          batch_report_flag = 1;
        count = 0;
      }

      if (batch_report_flag){
        write(pipe_acc_stream[1], &LPF_data_acc, sizeof(struct point_3D));
        ++count;
        if (count == training_size)
          batch_report_flag = 0;
      }

      if (periodic_report_flag){
        ++count;
        if (count == 50){
          output_file = fopen("testing_data.csv", "wb");
          for (i = 0; i < BUFFER_SIZE; ++i)
            fprintf(output_file,"%f,%f,%f\n",getElt(ax,i),getElt(ay,i), getElt(az,i));
          fclose(output_file);
          count = 0;
        }
      }

      if(q_size < BUFFER_SIZE){
        enQueue(ax, LPF_data_acc.x_comp);
        enQueue(ay, LPF_data_acc.y_comp);
        enQueue(az, LPF_data_acc.z_comp);
        ++q_size;
        if (q_size == bias_sample_init)
          bias_init_flag = 1;
        if (bias_init_flag){
          acc_bias.x_comp += LPF_data_acc.x_comp;
          acc_bias.y_comp += LPF_data_acc.y_comp;
          acc_bias.z_comp += LPF_data_acc.z_comp;
          if (q_size == bias_sample_end){
            printf("Offset Setup.\n%f,%f,%f\n", acc_bias.x_comp, acc_bias.y_comp, acc_bias.z_comp);
            acc_bias.x_comp /= bias_sample_end-bias_sample_init+1;
            acc_bias.y_comp /= bias_sample_end-bias_sample_init+1;
            acc_bias.z_comp /= bias_sample_end-bias_sample_init+1;
            bias_init_flag = 0;
            bias_flag = 1;
          }
        }
      }
      else{
        denQueue(ax, LPF_data_acc.x_comp);
        denQueue(ay, LPF_data_acc.y_comp);
        denQueue(az, LPF_data_acc.z_comp);
      }
    }
  }
  else{
    close(pipe_acc_stream[1]);
    close(pipe_ctrl2bb[0]);
    return 0;
  }
}

////////////////////////////////////////////////////////////////////
// Process 3: Keras Training Driver (ML_training)
////////////////////////////////////////////////////////////////////
int ML_driver_tr(){
  pid_ML_tr = fork();
  if (pid_ML_tr == 0){
    execv("./train_auto.py", NULL);
  }
  else{
    waitpid(pid_ML_tr, NULL, 0);
  }
}

////////////////////////////////////////////////////////////////////
// Process 4: Keras Testing Driver (ML_testing)
////////////////////////////////////////////////////////////////////
int ML_driver_te(char *name_arg){
  pid_ML_te = fork();
  if (pid_ML_te == 0){
    execl("test_auto.py", "test_auto.py", name_arg, (char *) NULL);
    return -1;
  }
  else{
    return 0;
  }
}

////////////////////////////////////////////////////////////////////
// Process 0: parent process (master)
////////////////////////////////////////////////////////////////////
int main (int argc, char **argv)
{
  if (argc > 1)
    run_flag = 1;
  // collect classification number, names, and hyper-parameters
  int classif_num;
  printf("Please enter the number of classifications being made [2-8]: ");
  fflush(stdin);
  scanf("%d", &classif_num);
  if (classif_num > 8 || classif_num < 2){
    printf("Improper input. Program Exiting.\n");
    return -1;
  }

  char classif_mnemo[MAX_CLASSIF][MAX_NAME_LEN];
  for (i = 0; i < classif_num; ++i){
    printf("Please enter the name of classifications [%d/%d]: ", i+1, classif_num);
    fflush(stdin);
    scanf("%s", classif_mnemo[i]);
  }

  // initialization stage
  init_BLE();
  init_pipes();
  init_queue();
  init_filter();

  printf("Setup Complete!\n----------------------\nCollected input:\n");
  for (i = 0; i < classif_num; ++i)
    printf("Motion Type %d: %s\n", i+1, classif_mnemo[i]);
  printf("----------------------\n");

  // data collection stage
  BLE_driver();
  DSP_driver();
  // buffering stage (offset gravity)
  printf("Buffering data . . .\n");
  sleep(3);
  printf("Data Buffered\n\n");

  if (!run_flag){
    /* collect sample data */
    int training_size = 1000;
    // write header
    FILE *training_file = fopen("training_data.csv", "w");
    fprintf(training_file, "%d,%d,%d", training_size, classif_num, BUFFER_SIZE);
    for (i = 0; i < classif_num; ++i)
      fprintf(training_file, ",%s", classif_mnemo[i]);
    fprintf(training_file, "\n");

    struct point_3D LPF_data_acc;
    char notif_mssg[128];
    snprintf(notif_mssg, sizeof(notif_mssg), "%d", training_size);

    struct pollfd poll_acc;
    poll_acc.fd = pipe_acc_stream[0];
    poll_acc.events = POLLIN;

    // clear stream
    int c; while ((c = getchar()) != '\n' && c != EOF);

    for (i = 0; i < classif_num; ++i){
      for (j = 0; j < classif_num; ++j){
        if (i == j)
          fprintf(training_file, "1");
        else
          fprintf(training_file, "0");
        if (j != classif_num-1)
          fprintf(training_file, ",");
        else
          fprintf(training_file, "\n");
      }

      printf("Type [Enter] to start saving training data for sample %s.\n", classif_mnemo[i]);
      getchar();
      printf("[0/%d] data collected.", training_size);

      write(pipe_ctrl2bb[1], notif_mssg, strlen(notif_mssg));
      for(j = 0; j < training_size;){
        poll(&poll_acc, 1, 0);
        if (poll_acc.revents & POLLIN){
          read(pipe_acc_stream[0], &LPF_data_acc, sizeof(struct point_3D));
          printf("\r[%d/%d] data collected.", j, training_size);
          //printf("%f\t%f\t%f\n", LPF_data_acc.x_comp, LPF_data_acc.y_comp, LPF_data_acc.z_comp);
          fprintf(training_file, "%f,%f,%f\n", LPF_data_acc.x_comp, LPF_data_acc.y_comp, LPF_data_acc.z_comp);
          ++j;
        }
      }

      printf("\r[%d/%d] data collected.\n", j, training_size);
    }
    fclose(training_file);

    /* spawn training process */
    printf("\nRunning Keras Training Script:\n");
    ML_driver_tr();
  }
  // testing stage
  /* spawn testing process */
  printf("\nStarting Keras real-time classification driver.\n");
  char name_arg [512] = "";
  strcat(name_arg, classif_mnemo[0]);
  for (i = 1; i < classif_num; ++i){
    strcat(name_arg, ":");
    strcat(name_arg, classif_mnemo[i]);
  }

  ML_driver_te(name_arg);

  /* periodically save text file */
  write(pipe_ctrl2bb[1], "-1", 16);
  printf("Type [Enter] to end program.\n\n");
  int c; while ((c = getchar()) != '\n' && c != EOF);
  while(1){
    usleep(50000);
    if(getchar() != EOF)
      break;
  }

  // cleanup
  kill(pid_BLE, SIGKILL);
  kill(pid_DSP, SIGKILL);
  kill(pid_ML_te, SIGKILL);
  clear(ax);
  clear(ay);
  clear(az);

  wait(NULL);
  printf("Successful Program Exit.\n");
}
