#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>

typedef struct{
	int *array;
	int l;
	int r;
	int id;
} arg_t;

typedef struct{
	pthread_mutex_t lock;
	int count;
} Counter;


sem_t barrier, barrier1;
Counter count;
int nthreads, swapped = 1, regionsize, n;


static int comp_integers(const void *p1, const void *p2){
	int *x = (int*) p1;
	int *y = (int*) p2;

	int a = *x;
	int b = *y;

	return (*x) - (*y);
}

static void* Th_func(void *args){
	arg_t *arg = (arg_t*) args;
	int *array = (*arg).array;
	int left = (*arg).l;
	int right = (*arg).r;
	int i, l, r, tmp;
	int id = (*arg).id;
	
	pthread_detach(pthread_self());

	
	while(1){	/*cyclic threads require 2 barriers*/
		sem_wait(&barrier1);
		if(swapped == 0){
			break;	
		}

								/*unlock the second barrier if it is the last thread to enter the CS*/
		pthread_mutex_lock(&count.lock);
		count.count++;
		if(count.count == nthreads){
			for(i = 0; i < nthreads; i++)
				sem_post(&barrier);
		}
		pthread_mutex_unlock(&count.lock);

		int dim = right - left;
		qsort(&array[left], right - left +1, sizeof(int), comp_integers);

			/*second barrier*/

		sem_wait(&barrier);

		pthread_mutex_lock(&count.lock);
		count.count--;

		if(count.count == 0){

			/*perform the swap on adjacent areas, set the swapped flag.
			If it is equal to 1, is unlocks the first barrier so threads can sort their area again*/
			l = regionsize-1;
			r = l+1;
			/*we don't care of the acual dimension of the last region
			 */	
			swapped = 0;
			for(i = 0; i < nthreads - 1; i++){
				if(array[l] > array[r]){
					
					tmp = array[l];
					array[l] = array[r];
					array[r] = tmp;
					swapped = 1;
				}
				l = l + regionsize;
				r = l+1;
			}
		
			for(i = 0; i < nthreads; i++)
				sem_post(&barrier1);
		}
		pthread_mutex_unlock(&count.lock);

	}

	pthread_exit(NULL);
	
}

int main(int argc, char* argv[]){
	int fd, *array, filesize, i, limitsN;
	struct stat buf;
	pthread_t *tid;
	arg_t *arguments;


	if(argc < 3){
		fprintf(stderr, "invalid number of parameters\n"); return -1;	
	}
	
	n = atoi(argv[1]);	
	array = (int*) malloc(n *sizeof(int));
	
	fd = open(argv[2], O_RDWR);

	array = mmap((caddr_t) 0, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	
	pthread_mutex_init(&count.lock, NULL);
	count.count = 0;

	nthreads = ceil(log10(n));
	regionsize = n / nthreads;	/*apart from the last region that can differ*/

	sem_init(&barrier, 0, 0);
	sem_init(&barrier1, 0, nthreads);

	
	tid = (pthread_t*) malloc(nthreads * sizeof(pthread_t));
	arguments = (arg_t*) malloc(nthreads * sizeof(arg_t));	
	int l; int r;
	l = 0;
	r = regionsize - 1;
	for(i = 0; i < nthreads; i++){
		arguments[i].l = l;
		arguments[i].r = r;
		arguments[i].array = array;
		arguments[i].id = i;

		pthread_create(&tid[i], NULL, Th_func, (void*) &arguments[i]);

		l += regionsize;
		if(i + 1 == nthreads-1)
			/*if the next is the last region assign as right 
			 * margin the last element of the array
			 */
			r = n;
		else
			r += regionsize;
	}
	
	pthread_exit(NULL);
}




