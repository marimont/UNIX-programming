#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "office.h"

typedef struct{
	int id;
} arg_t;

int special_count;

bool isSpecial(){
	 double x = 0.4 * ((double)RAND_MAX+1) - rand();
		if ( x >= 1 )  return true;
		if ( x <= 0 ) return false;

    return true;
}

void send(Buffer *buf, Info info){
	pthread_mutex_lock(&buf -> lock);
	while(buf -> count >= buf -> dim)
		pthread_cond_wait(buf -> notfull, &buf -> lock);
	buf -> buffer[buf -> in] = info;
	buf -> in++;
	if(buf -> in == buf -> dim)
		buf -> in = 0;
	buf -> count++;
	pthread_cond_broadcast(buf -> notempty);
	pthread_mutex_unlock(&buf -> lock);
}

Info receive(Buffer *buf){
	Info info;

	pthread_mutex_lock(&buf -> lock);
	while(buf -> count <= 0)
		pthread_cond_wait(buf -> notempty, &buf -> lock);
	info = buf -> buffer[buf -> out];
	buf -> out++;
	if(buf -> out >= buf -> dim)
		buf -> out = 0;
	buf -> count--;
	pthread_cond_broadcast(buf->notfull);
	pthread_mutex_unlock(&buf -> lock);

	return info;
}

void *Student(void *args){
	arg_t *arg = (arg_t*) args;
	int id = (*arg).id;

	Info info;
	info.id = id;
	/*send request to normal office*/	
	
	pthread_detach(pthread_self());

	pthread_mutex_lock(&cond -> lock);
	send(normal_Q, info);
	cond -> normal++;
	pthread_cond_broadcast(cond -> cond);
	pthread_mutex_unlock(&cond -> lock);

	/*wait for answer on stud queue*/
	info = receive(answer_Q[id]);
	fprintf(stdout, "[student %d] --- accepted by office %d\n", id, info.office_no);

	if(info.urgent == 1){
		/*redirection to special_Q*/
		fprintf(stdout, "[student %d] --- redirected to special office\n", id);
		send(special_Q, info);
		
		/*wait for answer*/
		info = receive(answer_Q[id]);
		
		/*enqueue on office_no urgent queue*/
		pthread_mutex_lock(&cond -> lock);
		send(urgent_Q[info.office_no], info);
		cond -> urgent[info.office_no]++;
		pthread_cond_broadcast(cond -> cond);
		pthread_mutex_unlock(&cond -> lock);

		/*waiting to be served*/
		info = receive(answer_Q[id]);
	} 
	
	fprintf(stdout, "[student %d] --- served by office %d. Terminating.\n", id, info.office_no);

	pthread_mutex_lock(&num_students.lock);
	num_students.num--;
	if(num_students.num == 0){
		pthread_cond_broadcast(cond -> cond);
		/*sending special info to special queue to terminte it*/
		info.id = -1;
		info.office_no = -1;
		info.urgent = -1;
		send(special_Q, info);
	}
	pthread_mutex_unlock(&num_students.lock);
	
	pthread_exit(NULL);
}

void *Normal_office(void *args){
	arg_t *arg = (arg_t*) args;
	int id = (*arg).id, wait_time;
	Info info;

	pthread_detach(pthread_self());
	srand(time(NULL));

	while(1){
		pthread_mutex_lock(&cond->lock);
		while(cond -> normal <= 0 && cond -> urgent[id] <= 0){
		/*cond is triggered in both cases in which a request has been inserted in the normal queue 
		 * or in the urgent queue
		 */	
			pthread_cond_wait(cond -> cond, &cond -> lock);
		
			pthread_mutex_lock(&num_students.lock);
				if (num_students.num == 0){
					/*i need to unlock both locks or the other threads won't notice that the num of alive students is equal to 0*/
					pthread_mutex_unlock(&num_students.lock);
					pthread_mutex_unlock(&cond -> lock);		
					fprintf(stdout, "[office %d] --- 		terminating\n", id);
					pthread_exit(NULL);
				}
			pthread_mutex_unlock(&num_students.lock);
		}		
												
	/*is it urgent?*/  								 
		if(cond -> urgent[id] > 0){
			/*urgent*/
			info = receive(urgent_Q[id]);
			fprintf(stdout, "[office %d] --- 		serving urgent request from student %d\n", id, info.id);
			cond -> urgent[id]--;
			pthread_mutex_unlock(&cond -> lock);
			sleep(1);
		
		} else {
			info = receive(normal_Q);
			fprintf(stdout, "[office %d] --- 		received request from student %d\n", id, info.id);
			cond -> normal--;
			pthread_mutex_unlock(&cond -> lock);
			/*is it special?*/
			if(isSpecial()){
				info.urgent = 1;
				fprintf(stdout, "[office %d] --- 		sending student %d to special office\n", id, info.id);
			}else {
				fprintf(stdout, "[office %d] --- 		serving request from student %d\n", id, info.id);
				wait_time = rand()%4 + 3; /*from 0 to 3 shifted by 3 = from 3 to 6)*/
				sleep(wait_time);
			}		
				
		}

		info.office_no = id;
		send(answer_Q[info.id], info);
	
	}

}

void* Special_office(void *args){
	arg_t *arg  = (arg_t*) args;
	Info info;
	int wait_time;

	srand(time(NULL));
	while(1){
		info = receive(special_Q);
		pthread_mutex_lock(&num_students.lock);
			if(num_students.num == 0){
				pthread_mutex_unlock(&num_students.lock);
				fprintf(stdout, "[special office] --- 				terminating\n");
				pthread_exit(NULL);
			}
		pthread_mutex_unlock(&num_students.lock);
		fprintf(stdout, "[special office] --- 				serving request from student %d\n", info.id);
		wait_time = rand()%4 + 3;
		sleep(wait_time);
		fprintf(stdout, "[special office] --- 				sending back to office %d\n", info.office_no);
		send(answer_Q[info.id], info);
		
	}
	
}

Buffer* B_init(int k){
	Buffer *buffer;
		
	buffer = (Buffer*) malloc(sizeof(Buffer));	
	buffer-> notfull = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
	buffer -> notempty = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
	buffer -> buffer = (Info*) malloc(k*sizeof(Info));
	pthread_mutex_init(&buffer->lock, NULL);
	pthread_cond_init(buffer -> notfull, NULL);
	pthread_cond_init(buffer -> notempty, NULL);
	buffer -> count = 0;
	
	buffer -> dim = k;
	buffer-> in = (buffer -> out = 0);
	return buffer;
}

Cond* cond_init(int dim){
	Cond *cond = (Cond*) malloc(sizeof(Cond));
	pthread_mutex_init(&cond->lock, NULL);
	cond -> cond = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
	pthread_cond_init(cond->cond, NULL);
	cond -> urgent = (int*) malloc(k*sizeof(int));
	return cond;
}

int main(int argc, char* argv[]){
	int k, i;
	arg_t *stud_args, office_args[NUM_OFFICES];
	pthread_t *stud_tid, off_tid[NUM_OFFICES], spec_tid;
	
	if(argc < 2){
		fprintf(stderr, "invalid number of parameters\n"); return -1;		
	}

	k = atoi(argv[1]);

	pthread_mutex_init(&num_students.lock, NULL);
	num_students.num = k;

	stud_tid = (pthread_t*) malloc(k *sizeof(pthread_t));

	normal_Q = B_init(k);
	special_Q = B_init(k);
	urgent_Q = (Buffer**) malloc(NUM_OFFICES * sizeof(Buffer*));
	for(i = 0; i < NUM_OFFICES; i++){
		urgent_Q[i] = B_init(k);
	}
	answer_Q = (Buffer**) malloc(k * sizeof(Buffer*));
	for(i = 0; i < k; i++){
		answer_Q[i] = B_init(4);	/*max four answers*/
	}

	cond = cond_init(NUM_OFFICES);

	stud_args = (arg_t*) malloc(k * sizeof(arg_t));

	/*create offices so they start waiting*/
	for(i = 0; i < NUM_OFFICES; i++){
		office_args[i].id = i;
		pthread_create(&off_tid[i], NULL, Normal_office, (void*)&office_args[i]);
	}
	
	pthread_create(&spec_tid, NULL, Special_office, NULL);

	/*create students*/
	for(i = 0; i < k; i++){
		stud_args[i].id = i;
		pthread_create(&stud_tid[i], NULL, Student, (void*)&stud_args[i]);
	}

	pthread_exit(NULL);
}










