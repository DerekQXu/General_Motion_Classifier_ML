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
#define MAX_NAME_LEN 18 
#define QUEUE_MAX 100

int i;
pid_t pid_setup = 123;
pid_t pid_sensing = 123;
pid_t pid_parsing = 123;

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

void parseSTM(){
	//Parsing from HEX to decimal
	pid_parsing=fork();
	if (pid_parsing == 0)
	{
		FILE* ble_file;
		ble_file = fopen("motion_data.txt", "r");

		out_ax = createQueue();
		out_ay = createQueue();
		out_az = createQueue();
		out_gx = createQueue();
		out_gy = createQueue();
		out_gz = createQueue();
		out_mx = createQueue();
		out_my = createQueue();
		out_mz = createQueue();
		printf("%f", getElt(out_ax,0));

		char raw[BUFF_MAX];

		// Advance line over file header
		fgets(raw, BUFF_MAX, ble_file);

		// Read motion data
		for (i = 0; i < QUEUE_MAX; ++i){
			//update Queue on arrival of new data
			if (fgets(raw, BUFF_MAX, ble_file)){
				// stream_parser modified for Queues
				if(stream_parser(raw, enQueue) == 0){ return; }		
			}
		}
		// Maintain queue size, QUEUE_MAX
		while(1){
			if(fgets(raw, BUFF_MAX, ble_file)){
				if(stream_parser(raw, denQueue) == 0){ return; }
				//printf("%f\n", getElt(out_ax,3));
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
	char classif_names[MAX_CLASSIF][MAX_NAME_LEN+1];
	int classif_num = atoi(argv[1]);
	if (classif_num > MAX_CLASSIF){
		printf("Error: Maxmimum number of classifications is 10\n");
		return 0;
	}

	// Enable Bluetooth Low Energy Connection
	setupBLE();
	if (pid_setup == 0){ return; }

	// Ask for training motion names (i.e. circle triangle square none)
	printf("Please keep the names of motion unique and below 18 characters.\n");
	for (i = 0; i < classif_num; ++i){
		printf("What is the name of the motion %d/%d?\n",i+1,classif_num);
		fflush(stdin);
		scanf("%s", classif_names[i]);
	}
	printf("setup complete!\n");

	// Initiate data collection
	enableSTM();
	if (pid_sensing == 0){ return; }

	// Initiate data parsing
	parseSTM();
	sleep(10);

	// TODO: Check here for seg fault
	kill(pid_sensing, SIGKILL);
	kill(pid_parsing, SIGKILL);
	int ret = 1;

	while (ret != 0){
		deQueue(out_ax);	
		deQueue(out_ay);	
		deQueue(out_az);	
		deQueue(out_gx);	
		deQueue(out_gy);	
		deQueue(out_gz);	
		deQueue(out_mx);	
		deQueue(out_my);	
		ret = deQueue(out_mz);	
	}

	printf("Program Exit Successful!\n");
}
