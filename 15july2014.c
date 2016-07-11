#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

sem_t num, den;
volatile int numer, denom;
volatile float result = 1;

int num_alive = 1, den_alive = 1;

typedef struct{
	int n;
	int tmp;
	int *array;
} arg_t;

void *Numerator(void* args);
void *Denominator(void *args);

int main(int argc, char *argv[]){
	int n, k, i, tmp;
	int *k_array, *n_array;
	pthread_t th[2];
	arg_t args[2];

	if(argc < 3){
		fprintf(stdout, "invalid number of input parameters\n"); return -1;
	}

	n = atoi(argv[1]);
	k = atoi(argv[2]);
	tmp = n - k;

	n_array = (int*) malloc((n - tmp)* sizeof(int));
	k_array = (int*) malloc(k * sizeof(int));


	sem_init(&num, 0, 0);
	sem_init(&den, 0, 0);


	args[0].n = n;
	args[0].tmp = tmp;
	pthread_create(&th[0], NULL, Numerator, (void*) &args[0]);

	args[1].n = k;
	pthread_create(&th[1], NULL, Denominator, (void*) &args[1]);

	for(i = 0; i < 2; i++){
		pthread_join(th[i], NULL);
	}

	fprintf(stdout, "[Main thread] --- result: %.0f\n", result);

	return 0;

}



void *Numerator(void* args){
	arg_t *arg = (arg_t*) args;
	int n = (*arg).n;
	int tmp = (*arg).tmp;
	int n_array[n];
	int remaining, i;
	int nn = n - tmp;
	
	for(i = 0; i < nn; i++){
		n_array[i] = n-i;		
	}

	num_alive = 1;
	remaining = nn; i = 0;
	while(remaining > 0){
		numer = n_array[i];
		i++; remaining--;
		if(remaining > 0){
			numer = numer * n_array[i];
			i++; remaining--;
		}
		sem_post(&den);
		sem_wait(&num);
		
	}
	num_alive = 0;
	sem_post(&den);
	pthread_exit(NULL);
}

void *Denominator(void *args){
	arg_t *arg = (arg_t*) args;
	int k = (*arg).n;
	int k_array[k];
	int remaining, i;
	float tmp_res;

	
	for(i = 0; i < k; i++){
		k_array[i] = k-i;
	}

	den_alive = 1;
	remaining = k; i = 0;
	while(remaining > 0){
		denom = k_array[i];
		i++; remaining--;
		if(remaining > 0){
			denom = denom * k_array[i];
			i++; remaining--;
		}
		sem_wait(&den);
		tmp_res = (float) numer/denom;
		result = result * tmp_res;
		sem_post(&num);
	}

	pthread_exit(NULL);
}















