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
int *run_flag;
int *valid_flag;
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

int parse_init(int training_samples, int testing_samples)
{
	// setup data structures
	char sensor_data[BUFF_SIZE];

	// setup data processing
	output = fopen("output.csv", "w");
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
	}
	// notify beaglebone data buffered 
	sem_post(mutex_beaglebone);


	/* begin parsing */
	int count = 0;
	while(run_flag){
		// always parse through data
		while(!fgets(sensor_data, BUFF_SIZE, stdin)); //TODO: poll instead of spinning
		parse_and_filter(sensor_data, denQueue);

		/* if beaglebone requesting data,
			buffer up required amount of data
			save the data to file
			notify beaglebone
		*/
		if (*valid_flag)
			++count;
		if (count == training_samples){
			// save data
			for (i = 0; i < training_samples; ++i)
				fprintf(output,"%f,%f,%f,%f,%f,%f\n",getElt(ax,i),getElt(ay,i),getElt(az,i),
				getElt(gx,i),getElt(gy,i),getElt(gz,i));

			// reset valid_bit and count
			*valid_flag = 0;
			count = 0;

			// notify beaglebone data saved 
			sem_post(mutex_beaglebone);
		}
	}

	// close everything
	fclose(output);
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
	sem_init(mutex_general, 1, 0);
	sem_init(mutex_beaglebone, 1, 0);
	*run_flag = 1;
	*valid_flag = 0;

	// start parsing
	output = fopen("output.csv", "w");
	pid_parsing=fork();
	if (pid_parsing == 0)
		parse_init(10, 1);
	printf("Initiating data collection...\n");
	printf("Collecting buffered data...\n");

	// run training test
	for (i = 0; i < classif_num; ++i){
		// wait for sensor to save data
		sem_wait(mutex_beaglebone);
		printf("Type [Enter] to start saving files for training sample %s.\n", classif_names[i]);
		fflush(stdin);
		getchar();

		// save name manually
		fprintf(output, "%s", classif_names[i]);

		// notify sensor to gather data
		*valid_flag = 1;
		printf("Gathering Data... please wait.\n");
	}

	// notify process is done
	*run_flag = 0;

	// close everything
	kill(pid_sensing, SIGKILL);
	waitpid(pid_parsing, NULL, 0);
	fclose(output);

	exit(0);
	// run real-time test
}
