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

#include "Queue.h"
#include "BLE_parser.h"

#define MAX_CLASSIF 10
#define MAX_NAME_LEN 12 
#define QUEUE_MAX 100 // MUST BE LARGER THAN 20 DUE TO GRAVITY

int i, j;
int *state_machine;
int *semaphore;
pid_t pid_setup = 123;
pid_t pid_sensing = 123;
pid_t pid_parsing = 123;
struct QNode *ax;
struct QNode *ay;
struct QNode *az;
struct QNode *gx;
struct QNode *gy;
struct QNode *gz;
struct QNode *mx;
struct QNode *my;
struct QNode *mz;
FILE *output;

// Automated BLE Connection
void setupBLE()
{
	// Run bctl_auto.sh
	pid_setup = fork();
	if (pid_setup == 0){
		execv("./bctl_auto.sh", NULL);
	}
	else{
		wait(NULL);
	}
}

// BlueZ Data Collection
void enableSTM()
{
	// Run gattool
	pid_sensing=fork();
	if (pid_sensing == 0)
	{
		// raw hex data stored in motion_data.txt
		int fd = open("motion_data.txt", O_RDWR);
		dup2(fd, 1);
		close(fd);

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
}

// Parsing HEX data
void parseSTM(){
	//Parsing from HEX to decimal
	pid_parsing=fork();
	if (pid_parsing == 0)
	{
		FILE *ble_file = fopen("motion_data.txt", "r");
		init_parsing();

		char raw[BUFF_MAX];

		// Advance line over file header
		fgets(raw, BUFF_MAX, ble_file);

		// Read motion data
		for (i = 0; i < QUEUE_MAX; ++i){
			//update Queue on arrival of new data
			if (fgets(raw, BUFF_MAX, ble_file)){
				if(stream_parser(raw, enQueue) == 0){ return; } // stream_parser modified for Queues	
			}
			else{ --i; }
		}
		*semaphore = 0; //notify parent process queue of size QUEUE_MAX 

		// Maintain queue size, QUEUE_MAX
		while(1){
			switch(*state_machine){
			case 0:
				if(fgets(raw, BUFF_MAX, ble_file)){
					if(stream_parser(raw, denQueue) == 0){ return; }
				}
				break;
			case 1:
				// save current queue to csv file
				ax = out_ax->front;
				ay = out_ay->front;
				az = out_az->front;
				gx = out_gx->front;
				gy = out_gy->front;
				gz = out_gz->front;
				mx = out_mx->front;
				my = out_my->front;
				mz = out_mz->front;
				output = fopen("output.csv", "w");
				for (i = 0; i < QUEUE_MAX; ++i){
					fprintf(output,"%f,%f,%f,%f,%f,%f,%f,%f,%f\n",ax->key,ay->key,az->key,gx->key,gy->key,gz->key,mx->key,my->key,mz->key);
					ax = ax->next;
					ay = ay->next;
					az = az->next;
					gx = gx->next;
					gy = gy->next;
					gz = gz->next;
					mx = mx->next;
					my = my->next;
					mz = mz->next;
				}
				fclose(output);
				*semaphore = 0; // notify parent process file safly written
				*state_machine = 0; // return to parsing 
				break;
			default:
				destr_parsing(); //free memory
				return;
			}
		}
	}
}

int main(int argc, char **argv)
{
	if(argc != 2){
		printf("Please enter:\nClassification Number (rec. 3)\n");
		return 0;
	}

	// Initialize values
	// float cutoff_freq = atof(argv[1]); <- deprecated, cutoff hardcoded now
	// Classification values for ML System
	char classif_names[MAX_CLASSIF][MAX_NAME_LEN+1];
	int classif_num = atoi(argv[1]);
	if (classif_num > MAX_CLASSIF){
		printf("Error: Maxmimum number of classifications is 10\n");
		return 0;
	}
	int dataset_size;
	// State Machine to keep track of program progress
	state_machine = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	*state_machine = 0;
	semaphore = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	*semaphore = 1;

	// Enable Bluetooth Low Energy Connection
	setupBLE(); //spawns new process to do this.
	if (pid_setup == 0){ return; }

	// Ask for training motion names (i.e. circle triangle square none)
	printf("Please keep the names of motion unique and below 12 characters.\n");
	for (i = 0; i < classif_num; ++i){
		printf("What is the name of the motion %d/%d?\n",i+1,classif_num);
		fflush(stdin);
		scanf("%s", classif_names[i]);
	}
	printf("Collect how many data samples per motion?\n");
	fflush(stdin);
	scanf("%d", &dataset_size);
	printf("setup complete!\n");

	// Initiate data collection
	enableSTM(); //spawns new process to do this.
	if (pid_sensing == 0){ return; }

	// Initiate data parsing
	parseSTM(); //spawns new process to do this.
	if (pid_parsing == 0) { return; }
	printf("Collecting buffered data...\n");
	while (*semaphore == 1){ ; } //TODO: change this to actual semaphore

	// clear input buffer
	fflush(stdin);
	getchar();

	// Create the .csv files
	char output_path [MAX_NAME_LEN+20];
	for (i = 0; i < classif_num; ++i){
		for (j = 0; j < dataset_size; ++j){
			printf("Type [Enter] to save files for training sample %d/%d [%s].\n", j+1, dataset_size, classif_names[i]);
			fflush(stdin);
			getchar();

			*semaphore = 1;	
			*state_machine = 1;	
			printf("Creating csv file...\n");
			sprintf(output_path, "./training_set/%s%d.csv", classif_names[i],j);
			while (*semaphore == 1){ ; }
			rename("output.csv",output_path);
		}
	}

	// Exit Program
	printf("Type [Enter] to stop recording.\n");
	fflush(stdin);
	getchar();

	*state_machine = 2;	

	// Wait for child processes to finish
	waitpid(pid_parsing, &i, 0);
	kill(pid_sensing, SIGKILL);

	printf("Program Exit Successful!\n");
}
