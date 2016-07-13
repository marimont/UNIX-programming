/*a better solution would be using a semaphores list ordered according to 
 * the correspondant thread priority
 * */

#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <errno.h>


typedef struct{
	int **threads;
	pthread_mutex_t lock;
} matrix;


typedef struct{
	int tid;
	int priority;
	int fd;
} arg_t;

sem_t *sem;
int k;
matrix waiting;

void *Thread(void *args);

int main(int argc, char *argv[]){
	pthread_t *threads;
	arg_t *args;
	FILE *fp;
	int **pip;
	char recvline[80];
	fd_set rset;
	int i, j;
	
	if(argc < 3){
		fprintf(stderr, "invalid number of parameters\n"); return -1;
	}
	
	k = atoi(argv[1]);
	fp = fopen(argv[2], "w");
	
	if(fp == NULL){
		fprintf(stderr, "fopen() failed\n"); return -1;
	}
	
	threads = (pthread_t*) malloc(k * sizeof(pthread_t));
	args = (arg_t*) malloc(k * sizeof(arg_t));
	sem = (sem_t*) malloc(k * sizeof(sem_t));
	
	pip = (int**) malloc(k * sizeof(int*));
	for(i = 0; i < k; i++)
		pip[i] = (int*) malloc(2 * sizeof(int));
	
	waiting.threads = (int**)malloc(3 * sizeof(int*));
	for(i = 0; i < 3; i++){
		waiting.threads[i] = (int*)malloc(k * sizeof(int*));
	}
	pthread_mutex_init(&waiting.lock, NULL);
	
	for(i = 0; i < 3; i++){
		for(j = 0; j < k; j++){
			waiting.threads[i][j] = 0;
		}
	}
	
	for(i = 0; i < k; i++)
		sem_init(&sem[i], 0, 0);
		
	srand(time(NULL));
	
	for(i = 0; i < k; i++){
		pipe(pip[i]);
		
		args[i].tid = i;
		args[i].priority = rand()%3 + 1;
		args[i].fd = pip[i][1];
		
		pthread_create(&threads[i], NULL, Thread, (void*) &args[i]);
		
	}
	
	/*i let the first start*/
	sem_post(&sem[0]);
	
	int fd;
	FD_ZERO(&rset);
	for(i = 0; i < k; i++)
		FD_SET(pip[i][0], &rset);
	while(1){
		fd = select(FD_SETSIZE, &rset, NULL, NULL, NULL);
		if(fd > 0){
			for(i = 0; i < k; i++){
				if(FD_ISSET(pip[i][0], &rset)){
					bzero(recvline, 80);
					read(pip[i][0], recvline, 80);
					fprintf(fp, "%s", recvline); fflush(fp);				
				}
			}
		} else if(fd < 0){
			fprintf(stderr, "Select() failed - %s\n", strerror(errno)); return -1;
		}
	}
	
}

void *Thread(void *args){
	arg_t *arg = (arg_t*) args;
	int priority = (*arg).priority;
	int id = (*arg).tid;
	int fd = (*arg).fd;
	struct timespec tim;
	int i, j, time_sleep;
	char sendline[50];
	
	srand(time(NULL));
	
	while(1){
		time_sleep = rand() % 9 + 2;
		sleep(time_sleep);
		
		pthread_mutex_lock(&waiting.lock);
		waiting.threads[priority-1][id] = 1;
		pthread_mutex_unlock(&waiting.lock);
		
		sprintf(sendline, "%d, %d, 0\n", id, priority);
		write(fd, sendline, strlen(sendline));
		
		sem_wait(&sem[id]);
		pthread_mutex_lock(&waiting.lock);
		waiting.threads[priority-1][id] = 0;
		pthread_mutex_unlock(&waiting.lock);
		
		fprintf(stdout, "thread n.%d priority %d --- I've entered my critical region\n", id, priority);
		sprintf(sendline, "%d, %d, 1", id, priority);
		write(fd, sendline, strlen(sendline));
		
		/*debugging purposes*/
		sleep(1);
		
		int found = 0;
		pthread_mutex_lock(&waiting.lock);
		for(i = 2; i >= 0 && !found; i--){
			for(j = 0; j < k && !found; j++){
				if(waiting.threads[i][j] == 1){
					sem_post(&sem[j]); found = 1;
				}
			}
		}
		if(found == 0){
			sem_post(&sem[id]);
		}
		pthread_mutex_unlock(&waiting.lock);
	}
		
}













