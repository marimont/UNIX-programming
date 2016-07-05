/*signal and kill system calls:
This program performs the following actions:
	- father process forks and starts printing on stdout the content of a file
	- child waits for a randomic time [0-5 secs] then sends SIGUSR1 to the father
	- this causes the father stdout alternatively redirected to a file and restored
	- after 15 secs the childs sends a SIGUSR2 and both processes terminate
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <sys/select.h>

#define MAXBUFL 80

char *prog_name;

int keepalive = 1;
int redirected = 0;
int savedstdout, fd;

void redirect_stdout(){
	savedstdout = dup(1);
	close(1);
	dup(fd);
	//dup2(fd, 1);
}

void reset_stdout(){
	close(1);
	dup(savedstdout);
	//dup2(savedstdout, 1);
}


static void signal_handler(int signo){
	if(signo == SIGUSR1){
	
		if(redirected == 1){
			/*reset stdout*/
			reset_stdout();
			redirected = 0;
			fprintf(stdout, "[%s] --- received SIGUSR1 - stdout restored.\n", prog_name); fflush(stdout);
								/*no need to store current print position, since the fp is unique and will be incremented at
									each read and stay blocked  if no one reads*/

		}else{
			fprintf(stdout, "[%s] --- received SIGUSR1 - redirect stdout.\n", prog_name); fflush(stdout);
								/*no need to store current print position, since the fp is unique and will be incremented at
									each read and stay blocked  if no one reads*/
			redirect_stdout();
			/*redirect stdout*/
			redirected = 1;
		}
		
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
	struct timeval tval;
	fd_set rset;
	char readline[MAXBUFL+1];
	
	/*close write terminal*/

	sprintf(prog_name, "%s - PID = %d", prog_name, getpid());

	/*set alarm of 15 seconds*/
	signal(SIGALRM, alarm_handler);
	alarm(15);

	/*seed rand()*/
	srand(time(NULL));
	
	while(keepalive){
		/*I'm gonna sleep a rand numb of secs from 0 to 5*/
		wait_time = rand()%6;

		sleep(wait_time);
		/*if the timeout expires I'm gonna wait for a wait_time interval and then execute this code in any case*/
		kill(pid, SIGUSR1);
		fprintf(stdout, "[%s] --- sent SIGUSR1 to process %d\n", prog_name, pid); fflush(stdout);
	}
	
	
}

int main(int argc, char* argv[]){

	FILE *fp, fpout;
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

	fd = creat("out.txt", 0666);
	
	while(keepalive){
		/*rewind inserted to see "something meaningful" when reaching EOF*/
		if (fgets(readline, MAXBUFL+1, fp) == NULL)
			rewind(fp);
		write(1, readline, strlen(readline));
		/*for debug purposes*/
		//sleep(1);
		
	}

	fclose(fp); close(fd);

	return 0;
}

