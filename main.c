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

int i;
static int *state_machine;

int main(int argc, char **argv)
{
	//check arguments
	if(argc != 2){
		printf("Please provide a cutoff frequency.\n");
		return 0;
	}

	// allows state_machine variable to be accessed from different processes. 
	state_machine = mmap(NULL, sizeof *state_machine, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*state_machine = 0;
	
	//spawns new process to connect to bluetooth
	system("./bctl_auto.sh");

	//fork 2 threads: 1 for sensing, 1 for parsing
	pid_t pid_sensing = 123;
	pid_t pid_parsing = 123;
	pid_sensing=fork();
	if (pid_sensing != 0){
		pid_parsing = fork();
	}

	//sensing process
	if (pid_sensing == 0)
	{
		//save output from gatttool in motion_data.txt
		int fd = open("motion_data.txt", O_RDWR);
		dup2(fd, 1);
		close(fd);
		//execute gatttool
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
	else if (pid_parsing == 0){
		//wait for state_machine to continue executing
		while(*state_machine != 1){sleep(1);}
		//execute parsing (can be altered for parallelization)
		execv("./Autofilter", argv);
	}
	else if (pid_sensing > 0) //&& pid_parsing > 0)
	{
		// wait for bctl_auto to finish executing
		sleep(1);
		
		//Apply data collection
		printf("\nApplying Sensing...\n");
		printf("Press [Enter] key when finished.\n");
		fflush(stdin);
		getchar();

		//Stop data collection
		kill(pid_sensing, SIGKILL);
		*state_machine=1;
		
		//Apply post-processing
		printf("Applying Parsing and DSP...\n");
		wait(NULL);
		printf("Program End.\n");
	}
	return 0;
}
