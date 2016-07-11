/*Write a concurrent C program concurrent_file_processing. c in the Unix environment which takes from
the command line an argument n , which must be an even integer, and generates n A_threads a n d n B_threads.

These threads perform the same task, but belong to two different types.
The synchronization among the threads follows these specifications:

	• The main thread generates all the other threads, then it terminates.
	• All the threads run concurrently, and are not cyclic.
	• Then A_threads are created with an associated identifier (0 to n-1).
	• Then B_threads are created with an associated identifier (0 to n-1).
	• Each thread sleeps a random number of seconds (max 3), then it is supposed to process a file identified by the thread identifier, 		but in our case it does nothing.
	• When a pair of threads of type A has processed their “files”, one of them (the last) must 
		concatenate the two files. In our case it simply prints for example: A4 cats: A4 A8
	• When a pair of threads of type B has processed their “files”, one of them (the last) must 
		concatenate the two file, in our case it simply prints for example: B5 cats B5 BO
	• When a pair of A_threads and a pair of B_threads have completed their concatenate operation, one
		of them (the last) must combine: A1 merges A1 A4 B3 B4
			
Professor hint was to use two arrays of counters for each kind of process, but, honestly I haven't found a feasible solution
to the problem in that way, so I've used lists in a producer-consumer style:

	-there is one list for threads A and one list for threads B
	-two lists, combined in a single structurer of completed pairs, for the last combination
	-each list tracks its size and is protected by means on a mutex

	The algorithm works this way:
		- there's only one thread routine which knows it's type throught the parameters passed by the main thread at creation time
		- each thread, performs its processing, then locks its list and checks, explicitly (rather then waiting on a ready queue, 
			I should have used a sem_timedwait in that case) it explicitely checks if some threads of the same kind has already finished
			-> if no, it puts itself into the queue and terminates
			-> if yes, it prints out info to the user and then locks the other struct which, acutally contains two lists and two counters.
				In fact, with only one queue, I should have inserted an additional field in the node, storing info about the type
				and scan the whole list; with two queues and two mutexes I could have caused a deadlock. So I've preferred this way to 
				perform the whole thing atomically.
				So, if there are elements of the other type in the queue, then the threads removes it, prints and terminates.
				Otherwise, it enqueues its couple in its queue and terminates.
*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define A 0
#define B 1

typedef struct node_list *list_link;
struct node_list{
	int id;
	list_link next;
};



typedef struct{
	list_link t;
	int count;
	pthread_mutex_t lock;
} *List;

list_link NEWlist_node(int id){
	list_link x = malloc(sizeof *x);
	x -> id = id;
	x -> next = NULL;
	return x;
}

List LISTinit(){
	List list = malloc(sizeof(*list));
	list -> t = NULL;
	return list;
}

void LISTput(List list, list_link x){
	list_link y = list -> t;
	list -> t = x;
	list -> t -> next = y;
	list -> count++;
}

list_link LISTget(List list){
	list_link x = list -> t;
	list -> t = list -> t -> next;
	list -> count--;
	return x;
}

int LISTsize(List list){
	return list -> count;
}

typedef struct node_pairs *pairs_link;
struct node_pairs{
	int id1;
	int id2;
	pairs_link next;
};

typedef struct{
	pairs_link t[2];
	int count[2];
	pthread_mutex_t lock;
} *Pairs;

pairs_link NEWpairs_node(int id1, int id2){
	pairs_link x = malloc(sizeof *x);
	x -> id1 = id1;
	x -> id2 = id2;
	x -> next = NULL;
	return x;
}

Pairs PAIRSinit(){
	Pairs pairs = malloc(sizeof *pairs);
	pairs -> t[1] = NULL;
	pairs -> t[0] = NULL;
	return pairs;
}

void PAIRSput(Pairs list, pairs_link x, int type){
	pairs_link y = list -> t[type];
	list -> t[type] = x;
	list -> t[type] -> next = y;
	list -> count[type]++;
}

pairs_link PAIRSget(Pairs list, int type){
	pairs_link x = list -> t[type];
	list -> t[type] = list -> t[type] -> next;
	list -> count[type]--;
	return x;
}

int PAIRSsize(Pairs list, int type){
	return list -> count[type];
}


typedef struct{
	int id;
	int type; 
} arg_t;


List list[2];
Pairs pairs;
//sem_t ready[2], ready_p;


void *Thread(void *args);

int main(int argc, char *argv[]){
	int n, i;
	pthread_t *tids[2];
	arg_t *args[2];

	if(argc < 2){
		return -1;
	}
	
	n = atoi(argv[1]);

	/*sem_init(&ready_p, 0, 0);
	sem_init(&ready[0], 0, 0);
	sem_init(&ready[1], 0, 0);*/

	for(i = 0; i < 2; i++){
		list[i] = LISTinit();
		pthread_mutex_init(&(list[i] -> lock), NULL);

	}
	pairs = PAIRSinit();
	pthread_mutex_init(&(pairs -> lock), NULL);

	for(i = 0; i < 2; i++){
		tids[i] = (pthread_t*) malloc(n * sizeof(pthread_t));	
		args[i] = (arg_t*) malloc(n * sizeof(arg_t));	
	}

	for(i = 0; i < n; i++){
		args[0][i].id = i;
		args[0][i].type = A;
		pthread_create(&tids[0][i], NULL, Thread, (void*)&args[0][i]);

		args[1][i].id = i;
		args[1][i].type = B;
		pthread_create(&tids[1][i], NULL, Thread, (void*) &args[1][i]);
	}	

	pthread_exit(0);
	
}

void *Thread(void *args){
	arg_t *arg = (arg_t*) args;
	int id = (*arg).id;
	int type = (*arg).type;
	int i, sleep_time, count;
	list_link l;
	pairs_link p;

	pthread_detach(pthread_self());

	srand(time(NULL));
	sleep_time = rand()%4 + 1;
	sleep(sleep_time);

	pthread_mutex_lock(&(list[type] -> lock));
	if(list[type] -> count == 0){
		l = NEWlist_node(id);
		LISTput(list[type], l);
	} else {
		l = LISTget(list[type]);
		char typeC;
		if(type == A)
			typeC = 'A';
		else
			typeC = 'B';
		fprintf(stdout, "%c%d cats: %c%d %c%d\n", typeC, id, typeC, id, typeC, l -> id);
		int type1;
		if(type == A)
			type1 = B;
		else
			type1 = A;

		/*only one lock on both lists -> no deadlock*/
		pthread_mutex_lock(&(pairs -> lock));
		if(pairs -> count[type1] == 0){
			p = NEWpairs_node(id, l -> id);
			PAIRSput(pairs, p, type);
		}else{
			char type1C;
			if(type1 == A)
				type1C = 'A';
			else
				type1C = 'B';
			p = PAIRSget(pairs, type1);
			fprintf(stdout, "%c%d merges: %c%d %c%d %c%d %c%d\n", typeC, id, typeC, id, typeC, l -> id, type1C, p -> id1, type1C, p ->id2);
		}
		pthread_mutex_unlock(&(pairs-> lock));
	}
	pthread_mutex_unlock(&(list[type] -> lock));
	
	pthread_exit(NULL);
}















