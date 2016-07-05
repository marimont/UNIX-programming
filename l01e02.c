/*signal and kill system calls*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

#define MAXBUFL 80

char *prog_name;

int keepalive = 1;
int keeprinting = 1;

static void signal_handler(int signo){
	if(signo == SIGUSR1){
		fprintf(stdout, "[%s] --- received SIGUSR1.\n", prog_name); fflush(stdout);
								/*no need to store current print position, since the fp is unique and will be incremented at
									each read and stay blocked  if no one reads*/
		if(keeprinting == 1)
			keeprinting = 0;
		else
			keeprinting = 1;
		signal(SIGUSR1, signal_handler);	/*reinstall the signal handler*/
	} else if(signo == SIGUSR2){
		fprintf(stdout, "[%s] --- received SIGUSR2. Terminating.\n", prog_name); fflush(stdout);
		keepalive = 0;
	} 
}

static void alarm_handler(int signo){
	keepalive = 0;
	kill(getppid(), SIGUSR2);
	fprintf(stdout, "[%s] --- sent SIGUSR2 to process %d\n", prog_name, getppid()); fflush(stdout);
}


void child_proc(pid_t pid){
	int wait_time;
	
	sprintf(prog_name, "%s - PID = %d", prog_name, getpid());

	/*set alarm of 15 seconds*/
	signal(SIGALRM, alarm_handler);
	alarm(15);

	/*seed rand()*/
	srand(time(NULL));
	
	while(keepalive){
		/*I'm gonna sleep a rand numb of nanosecs from 0 to 3*/
		wait_time = rand()%4;
		sleep(wait_time);
		kill(pid, SIGUSR1);
		fprintf(stdout, "[%s] --- sent SIGUSR1 to process %d\n", prog_name, pid); fflush(stdout);
	}
	
	
}

int main(int argc, char* argv[]){

	FILE *fp;
	char readline[MAXBUFL+1];

	prog_name = argv[0];

	if(argc < 2){
		fprintf(stderr, "[%s] --- wrong number of input values\n", prog_name);
		return 1;
	}
	
	if((fp = fopen(argv[1], "r")) == NULL){
		fprintf(stderr, "[%s] --- can't open '%s' - %s\n", prog_name, argv[1], strerror(errno));
		return 2;
	}

	fprintf(stdout, "[%s] --- forking...\n", prog_name);
	if(fork() == 0){
		/*child proc*/
		child_proc(getppid());
		exit(0);
	}
	/*parent proc*/
	
	sprintf(prog_name, "%s - PID = %d", prog_name, getpid());
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);

	int lineN = 0;
	while(keepalive){
		if(keeprinting){
			fgets(readline, MAXBUFL+1, fp);
			fprintf(stdout, "[%s] --- line %d: '%s'\n", prog_name, lineN, readline); fflush(stdout);
			lineN++;

		} else{
			fprintf(stdout, "[%s] --- pause()\n", prog_name); fflush(stdout);
			pause();
		}
		
	}

	fclose(fp);

	return 0;
}

