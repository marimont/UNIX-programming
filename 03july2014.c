#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <linux/limits.h>
#include <semaphore.h>

#define M 0
#define F 1

typedef struct item *Item;
struct item{
	char id[50];
	int gendre;
	int arrival_time; //minutes
	int reg_time;
	int reg_time_from_start;
	int vot_time;
};

typedef struct node* link;
struct node{
	Item item;
	link next;
};

link NEW(Item item, link next){
	link x = malloc(sizeof *x);
	x -> item = item;
	x -> next = next;
	return x;
}

typedef struct queue *Q;
struct queue{
	link head;
	link tail;
	int maxN;
	int count;
	int count2[2];
	pthread_mutex_t lock;
};

Q QUEUEinit(int maxN){
	Q q = malloc(sizeof *q);
	q -> head = (q -> tail = NULL);
	q -> count = 0;
	q -> count2[0] = 0;
	q -> count2[1] = 0;
	q -> maxN = maxN;
	pthread_mutex_init(&(q -> lock), NULL);
	return q;
}

bool QUEUEempty(Q q){
	return (q -> count == 0);
}

bool QUEUEfull(Q q){
	return (q -> count = q -> maxN);
}

void QUEUEput(Q q, Item item){
	if(q -> head == NULL){	//empty list
		q -> head = (q -> tail = NEW(item, q -> head)); 
		q -> count++;
		q -> count2[item -> gendre]++;
		return;	
	}
	q -> tail -> next = NEW(item, q -> tail -> next);	/*tail insertion*/
	q -> tail = q -> tail -> next;
	q -> count++;
	q -> count2[item -> gendre]++;
}

void QUEUEputH(Q q, Item item){
	link x = NEW(item, q -> head);
	q -> head = x;
	q -> count++;
	q -> count2[item -> gendre]++;
}

Item QUEUEget(Q q){
	Item item = q -> head -> item;
	link x = q -> head -> next;
	q -> head = x;
	q -> count--;
	q -> count2[item -> gendre]--;
	return item;
}

void QUEUEfree(Q q){
	link t;
	if(q -> head != NULL){
		do{
			t = q -> head;
			free(t);
			q -> head = q -> head -> next;
		}while(q -> head == NULL);
	}
	free(q);
}

void QUEUEscan(Q q){
	link x = q -> head;
	fprintf(stdout, "List content: ");
	while(x != NULL){
		fprintf(stdout, "id = %s, ", x -> item -> id);	
		x = x -> next;
	}
	fprintf(stdout, "\n");
}

Q reg_list, vot_list;
sem_t notempty, notfull;
int tot_reg_timeM, tot_reg_timeF;
int nM, nF;
int max_reg_time;
int sum_wait_time[2];

typedef struct{
	FILE *fp;
	pthread_mutex_t lock;
}file;

file fp;

typedef struct{
	int gendre;
}arg_t;

void *Reg_thread(void *args);
void *Vot_thread(void *args);

int main(int argc, char* argv[]){
	int i, n, m;
	pthread_t *vot_threads, reg_threads[2];
	arg_t reg_args[2], *vot_args;
	FILE *fpout, *fpin;
	Item item;
	char id[50], arrival_time[10], gendre; 
	int reg_time, vot_time;
	
	if(argc < 5){
		fprintf(stderr, "invalid number of parameters\n"); return -1;
	}

	n = atoi(argv[1]);
	m = atoi(argv[2]);
	/*input_file = argv[3], output_file = argv[4]*/

	
	fpin = fopen(argv[3], "r");
	if(fpin == NULL){
		fprintf(stderr, "fopen() failed\n"); return -1;
	}
	
	fp.fp = fopen(argv[4], "w");	
	if(fp.fp == NULL){
		fprintf(stderr, "fopen() failed\n"); return -1;	//this will kill all threads
	}
		
	reg_list = QUEUEinit(n);

	int arrival_timeM = 0;
	char *sub_str;
	
	max_reg_time = 0;
	while(fscanf(fpin, "%s %*s %*s %c %s %d %d", id, &gendre, arrival_time, &reg_time, &vot_time) != EOF){
		item = malloc(sizeof *item);
		if(gendre == 'M'){
			item -> gendre = M;
			nM++;
			
			/*i keep track of the effective time a person 
			has to wait before registering*/
			tot_reg_timeM += reg_time;
			item -> reg_time_from_start = tot_reg_timeM;
		} else{
			item -> gendre = F;
			nF++;
			tot_reg_timeF += reg_time;
			item -> reg_time_from_start = tot_reg_timeF;
		}
		
		strcpy(item -> id, id);
		item -> reg_time = reg_time;
		item -> vot_time = vot_time;
		sub_str = strtok(arrival_time, ":");
		arrival_timeM = atoi(sub_str);
		sub_str = strtok(NULL, ":");
		arrival_timeM = arrival_timeM*60 + atoi(sub_str);

		item -> arrival_time = arrival_timeM;
		
		QUEUEput(reg_list, item);

		if(reg_time > max_reg_time)
			max_reg_time = reg_time;		/*i'll use it for sem_timedwait*/
	}

	sem_init(&notfull, 0, n);
	sem_init(&notempty, 0, 0);

	reg_args[0].gendre = M;
	pthread_create(&reg_threads[0], NULL, Reg_thread, (void*)&reg_args[0]);
	reg_args[1].gendre = F;
	pthread_create(&reg_threads[1], NULL, Reg_thread, (void*)&reg_args[1]);

	vot_list = QUEUEinit(m);
	pthread_mutex_init(&fp.lock, NULL);

	vot_threads = (pthread_t*) malloc(n * sizeof(pthread_t));
	vot_args = (arg_t*) malloc(n * sizeof(arg_t));
	for(i = 0; i < n; i++){
		vot_args[i].gendre = i;
		pthread_create(&vot_threads[i], NULL, Vot_thread,(void*)&vot_args[i]);
	}

	for(i = 0; i < n; i++){
		pthread_join(vot_threads[i], NULL);
	}

	fprintf(stdout, "Male average waiting time: %d\n", sum_wait_time[0]/nM);
	fprintf(stdout, "Female average waiting time: %d\n", sum_wait_time[1]/nF);

	QUEUEfree(vot_list); QUEUEfree(reg_list);
	free(vot_threads); fclose(fpin); fclose(fp.fp);

	return 0;
}

void *Reg_thread(void *args){
	arg_t *arg = (arg_t*) args;	
	int gendre = (*arg).gendre;
	Item item;

	while(reg_list -> count2[gendre] != 0){
		/*extract voter from the list of the thread gender*/
		pthread_mutex_lock(&reg_list -> lock);
		item = QUEUEget(reg_list);
		/*i extract the first element from the list, if it is not
			of my gendre, I put it on the head of the list, to save the FIFO
			mechanism*/
		if(item -> gendre != gendre){
			QUEUEputH(reg_list, item); 
			pthread_mutex_unlock(&reg_list -> lock);
			continue;
		}
		pthread_mutex_unlock(&reg_list -> lock);

		/*sleep for the given time*/
		sleep(item -> reg_time);
		sem_wait(&notfull);
		pthread_mutex_lock(&vot_list -> lock);
		QUEUEput(vot_list, item);
		fprintf(stdout, "[Reg Station %d] --- %s inserted in vot_list\n", gendre, item -> id);
		pthread_mutex_unlock(&vot_list -> lock);
		sem_post(&notempty);
	}
	fprintf(stdout, "count2[gendre] == %d. terminating\n", reg_list -> count2[gendre]);
	pthread_exit(NULL);
}

void *Vot_thread(void *args){
	arg_t *arg = (arg_t*) args;
	int id = (*arg).gendre;
	struct timespec tim;
	Item item;
	int HH, MM;
	int retVal;
	
	while(1){
		/*get current time*/
		clock_gettime(CLOCK_REALTIME, &tim);
		/*set timeout: it will expire whene CLOCK_REALTIME 
		 * will equal the specified timeout*/
		tim.tv_sec += 2 * max_reg_time;
		//sem_wait(&notempty);
		if( (retVal = sem_timedwait(&notempty, &tim)) == -1){
			fprintf(stdout, "No more voters: terminating\n");
			break;
		}
		
		/*extract first user in queue*/
		pthread_mutex_lock(&vot_list -> lock);
		item = QUEUEget(vot_list);
		fprintf(stdout, "[Vot Station %d] --- %s extracted from vot_list\n", id, item -> id);
		/*this  is still thread -> safe, cos I'm protected by a mutex!*/
		sum_wait_time[item -> gendre] += (item -> reg_time_from_start + item -> vot_time);
		pthread_mutex_unlock(&vot_list -> lock);
		sem_post(&notfull);
		
		/*these operations are local to the thread -> no need for mutex*/
		sleep(item -> vot_time);
		
		HH = (item -> arrival_time + item -> reg_time_from_start + item -> vot_time) / 60;
		MM = (item -> arrival_time + item -> reg_time_from_start + item -> vot_time) % 60;
		
		/*the file pointer is updated at each write, if threads are synchronized, they will not
		overwrite*/		
		pthread_mutex_lock(&fp.lock);
		
		fprintf(fp.fp, "%s VotingStation%d %d.%d\n", item -> id, id, HH, MM);
		pthread_mutex_unlock(&fp.lock);
	}

	pthread_exit(NULL);
}





















