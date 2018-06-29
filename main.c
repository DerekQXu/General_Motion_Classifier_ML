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

#include "data_proc.h"
#include "queue.h"

#define MAX_CLASSIF 10
#define MAX_NAME_LEN 12 

int i, j;
FILE *output;
char *output_file = "training_set.txt";
pid_t pid_setup = 123;
pid_t pid_sensing = 123;
pid_t pid_parsing = 123;
pid_t pid_keras_trainer = 123;
pid_t pid_keras_classify = 123;
pid_t pid_clock = 123;
int *run_flag;
int *valid_flag;
int *gather_flag;
sem_t *mutex_general;
sem_t *mutex_beaglebone;

int classif_num;
char classif_names[MAX_CLASSIF][MAX_NAME_LEN+1];

int pipe_stm2bb[2];

int sensor_init()
{
	// enable BLE
	pid_setup = fork();
	if (pid_setup == 0){
		execv("./bctl_auto.sh", NULL);
	}
	else{
		wait(NULL);
	}

	// setup pipe
	if (pipe(pipe_stm2bb) == -1){
		fprintf(stderr, "Pipe failed.");
		return -1;
	}

	// run gattool
	pid_sensing=fork();
	if (pid_sensing == 0)
	{
		close(pipe_stm2bb[0]);
		dup2(pipe_stm2bb[1], STDOUT_FILENO);

		const char* suffix[9] =
		{
			"/usr/bin/gatttool",
			"-b",
			"C0:83:26:30:4F:4D",
			"-t",
			"random",
			"--char-write-req",
			"--handle=0x0012",
			"--value=0100",
			"--listen"
		};
		execl("/usr/bin/gatttool", suffix[0], suffix[1], suffix[2], suffix[3], suffix[4], suffix[5], suffix[6], suffix[7], suffix[8], (char *) NULL);
	}

	return 0;
}

int parse_init(int classif_num, int training_samples, int testing_samples)
{
	// remove existing training_data.txt
	remove("training_data.csv");

	// setup data structures
	char sensor_data[BUFF_SIZE];

	// setup data processing
	init_parsing(training_samples + 1);
	init_filter();

	// setup pipes
	close(pipe_stm2bb[1]);
	dup2(pipe_stm2bb[0], STDIN_FILENO);

	// skip over the first line
	while(!fgets(sensor_data, BUFF_SIZE, stdin));

	// buffer motion data
	/* NOTE: we buffer motion data because it takes time for:
		sensor to calibrate to gravity
		sensor to reach steady state
		filter to calibrate to real-time
		*/
	for (i = 0; i < training_samples; ++i){
		while(!fgets(sensor_data, BUFF_SIZE, stdin)); //TODO: poll instead of spinning
		parse_and_filter(sensor_data, enQueue);
		if (!(i % 10))
			printf("\r [%d/%d]", i, training_samples);
	}
	// notify beaglebone data buffered 
	sem_post(mutex_beaglebone);

	// collect training data
	int count = 0;
	while(1){
		// always parse through data
		while(!fgets(sensor_data, BUFF_SIZE, stdin)); //TODO: poll instead of spinning
		parse_and_filter(sensor_data, denQueue);

		/* if beaglebone requesting data,
		   buffer up required amount of data
		   save the data to file
		   notify beaglebone
		 */
		if (*valid_flag){
			if (!(count % 10))
				printf("\r [%d/%d]", count, training_samples);
			++count;
			if (count == training_samples){
				// save data
				output = fopen("training_data.csv", "a");
				for (i = 0; i < training_samples; ++i)
				//fprintf(output,"%f,%f,%f,%f,%f,%f\n",getElt(ax,i),getElt(ay,i), getElt(az,i),getElt(gx,i),getElt(gy,i),getElt(gz,i));
				fprintf(output,"%f,%f,%f\n",getElt(ax,i),getElt(ay,i), getElt(az,i));
				fclose(output);

				// reset valid_bit and count
				sem_wait(mutex_general);
				*valid_flag = 0;
				sem_post(mutex_general);
				count = 0;

				// notify beaglebone data saved 
				sem_post(mutex_beaglebone);
			}
		}
		/* depending on a clock, we save a data to a file
		   to run out python Keras model on
		*/
		if (*gather_flag){
			remove("testing_data.csv");
			output = fopen("testing_data.csv", "a");
			for (i = 0; i < 100; ++i)
				//fprintf(output,"%f,%f,%f,%f,%f,%f\n",getElt(ax,i),getElt(ay,i), getElt(az,i),getElt(gx,i),getElt(gy,i),getElt(gz,i));
				fprintf(output,"%f,%f,%f\n",getElt(ax,i),getElt(ay,i), getElt(az,i));
			fclose(output);
			// run keras model on our data 
			pid_keras_classify = fork();
			if (pid_keras_classify == 0){
				//execv("./classify.py", NULL);
				exit(0);
			}
			waitpid(pid_keras_classify, NULL, 0);
			*gather_flag = 0;
		}
		// check if main process is done
		if (*run_flag == 0)
			break;
	}

	return 0;
}

int main(int argc, char **argv)
{
	// parse options
	if(argc != 2){
		printf("Error: expected classification number.\n");
		exit(0);
	}
	if (atoi(argv[1]) > MAX_CLASSIF){
		printf("Error: Maxmimum number of classifications is 10\n");
		return 0;
	}

	classif_num = atoi(argv[1]);

	for (i = 0; i < classif_num; ++i){
		printf("What is the name of the motion %d of %d?\n",i+1,classif_num);
		fflush(stdin);
		scanf("%s", classif_names[i]);
	}

	printf("setup complete!\n");

	// start bluetooth
	if(sensor_init() < 0){
		printf("Error: hardware initiation failed.\n");
		exit(0);
	}

	// setup synchronization
	mutex_general = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
	mutex_beaglebone = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
	run_flag = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
	valid_flag = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
	gather_flag = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
	sem_init(mutex_general, 1, 1);
	sem_init(mutex_beaglebone, 1, 0);
	*run_flag = 1;
	*valid_flag = 0;
	*gather_flag = 0;
	
	// start parsing
	pid_parsing=fork();
	if (pid_parsing == 0){
		parse_init(classif_num, 1000, 1);
		exit(0);
	}
	printf("Initiating data collection...\n");
	printf("Collecting buffered data...\n");

	// BUG: must have this to clean stream for some reson
	fflush(stdin);
	getchar();
	// run training test
	for (i = 0; i < classif_num; ++i){
		// wait for sensor to save data
		sem_wait(mutex_beaglebone);
		printf("\r [1000/1000]\n");

		printf("Type [Enter] to start saving files for training sample %s.\n", classif_names[i]);
		fflush(stdin);
		getchar();
		output = fopen("training_data.csv", "a");

		// print proper output
		if (i == 0)
			fprintf(output, "1");
		else
			fprintf(output, "0");
		for (j = 1; j < i; ++j)		
			fprintf(output, ",0");
		if (j == i){
			fprintf(output, ",1");
			++j;
		}
		for (; j < classif_num; ++j)		
			fprintf(output, ",0");
		fprintf(output, "\n");

		fclose(output);

		sem_wait(mutex_general);
		// notify sensor to gather data
		*valid_flag = 1;
		sem_post(mutex_general);
		printf("Gathering data....\n");
	}
	// wait for sensor to save data
	sem_wait(mutex_beaglebone);
	printf("\r [1000/1000]\n");

	// run python script to train keras model
	pid_keras_trainer = fork();
	if (pid_keras_trainer == 0){
		//execv("./train.py", NULL);
		exit(0);
	}
	waitpid(pid_keras_trainer, NULL, 0);

	// testing is handled by parsing process
	printf("\nTesting Data:\n");

	// periodically notifies clock
	pid_clock = fork();
	if (pid_clock == 0){
		// check for changed state every second 
		while (*run_flag){
			sleep(1);	
			*gather_flag = 1;
		}
		exit(0);
	}

	// let user tell us when to quit
	printf("Type [Enter] to exit program.\n");
	getchar();

	// notify everyone that main is done 
	*run_flag = 0;

	// close everything
	waitpid(pid_clock, NULL, 0);
	waitpid(pid_parsing, NULL, 0);
	kill(pid_sensing, SIGKILL);

	printf("Successful program exit!\n");

	exit(0);
}
