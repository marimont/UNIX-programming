#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

typedef struct{
	int ranking;
	int leader_id;
	pthread_mutex_t lock;
} Best_ranking;

typedef struct{
	int tid;
	int ranking;
	int count;
} arg_t;

typedef struct{
	int nt;
	pthread_mutex_t lock;
} Counter;

sem_t barrier1, barrier2;
Counter c;
Best_ranking best_ranking;

int Random_Generator(int *rankings, int *N){
	int ret;
	int k = *N;
	int x = rand()%k;
	
	ret = rankings[x];
	rankings[x] = rankings[k-1];
	k--;

	(*N) = k;
	
	return ret;
}

static void signal_handler(int signo){
	if(signo == SIGUSR1){
		signal(SIGUSR1, signal_handler);
	}
}

void *Thread(void *args);

int main(int argc, char* argv[]){
	int N, i, *rankings, not_assigned, tid, *threads_rand, not_assignedT;
	pthread_t *threads;
	arg_t *args;

	if(argc < 2){
		fprintf(stderr, "Invalid number of input parameters\n"); return -1;
	}
	
	N = atoi(argv[1]);
	if(N <= 8){
		fprintf(stderr, "N must be greater than 8\n"); return -1;
	}

	threads = (pthread_t*) malloc(N * sizeof(pthread_t));
	args = (arg_t*) malloc(N * sizeof(pthread_t));
	rankings = (int*) malloc(N * sizeof(int));
	threads_rand = (int*) malloc(N * sizeof(int));

	not_assigned = N;
	srand(time(NULL));
	for(i = 0; i < N; i++){
		rankings[i] = i;
	}

	sem_init(&barrier1, 0, N-3);
	sem_init(&barrier2, 0, 0);
	pthread_mutex_init(&best_ranking.lock, NULL);
	pthread_mutex_init(&c.lock, NULL);
	
	
	for(i = 0; i < N; i++){
		args[i].tid = i;
		args[i].ranking = Random_Generator(rankings, &not_assigned);
		args[i].count = N - 3;
		pthread_create(&threads[i], NULL, Thread, (void*)&args[i]);
	}
	
	srand(time(NULL));
	while(1){
		int sleep_time = rand() % 4 +2;
		sleep(sleep_time);

		/*i need to reset threads num generator, at each loop*/
		not_assignedT = N;
		for(i = 0; i < N; i++)
			threads_rand[i] = i;
		fprintf(stdout, "\n------------>New elections<------------\n");
		best_ranking.ranking = 0; best_ranking.leader_id = 0;
		for(i = 0; i < N -3; i++){
			tid = Random_Generator(threads_rand, &not_assignedT);
			pthread_kill(threads[tid], SIGUSR1);
		}
	}

	return 0;
}

void *Thread(void *args){
	arg_t *arg = (arg_t*) args;
	int ranking = (*arg).ranking;
	int tid = (*arg).tid;
	int count = (*arg).count;
	int i;

	signal(SIGUSR1, signal_handler);
	while(1){
		/*first barrier*/
		pause();
		
		/*i need this barrier in order to avoid fast threads to consume the semaphores of slow threads.
		 * moreover it also blocks the threads that are awaken from the  main thread if there is still
         * an election running
		 **/
		sem_wait(&barrier1);	

		/*signal received*/
		pthread_mutex_lock(&best_ranking.lock);
		if(ranking > best_ranking.ranking){
			best_ranking.ranking = ranking;
			best_ranking.leader_id = tid;
		}
		pthread_mutex_unlock(&best_ranking.lock);

		/*i cannot unlock the barrier at the beginning of the critical section
		 * because when unlocked, the threads will start printing but, maybe the last thread
		 * ,i.e. the one that unlocked them, has not done its job yet and it's possible that
		 *  it will change the value again. So I must unlock anybody only after having done 
		 * the job
		 **/
		pthread_mutex_lock(&c.lock);
		c.nt++;
		if(c.nt == count){
			for(i = 0; i < count; i++)
				sem_post(&barrier2);
		}
		pthread_mutex_unlock(&c.lock);

		/*synchronize with other threads*/
		sem_wait(&barrier2);
		fprintf(stdout, "Thread n. %d rank = %d. Leader thread = %d best_ranking = %d\n", tid, ranking, best_ranking.leader_id, best_ranking.ranking);

		pthread_mutex_lock(&c.lock);
		c.nt--;
		if(c.nt == 0){
			for(i = 0; i < count; i++)
				sem_post(&barrier1);
		}
		pthread_mutex_unlock(&c.lock);
	}
	
}























