/*There are n stations and m trains.
 * Each station contains two tracks: one going clockwise and the other one goig counter-clockwise.
 * Each train starts in a station, on a track and keeps going in the same sense.
 * 
 * It sleeps for a 1 to 5 seconds in the station, then tries to proceed(backward or forward, according
 * to its sense). It can proceed only if the connection and the track in the next station are free.
 * 
 * It takes 10 seconds to traverse the connection.
 * 
 * The initial configuration is provided my the main thread with the function select_station_and_tracks
 * which assigns initial station and track, to each train, randomly.
 * 
 * Connections and tracks within each stations are protected by array of mutexes:
 * 
 * - a train that wants to occupy it, has to lock the correspondant mutex.
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

typedef struct{
	int id;
	int track;
	int station;
	int stationsN;
}arg_t;

pthread_mutex_t  *connection_mutex,
					*internal_mutex[2];
int *stations, not_assigned;

void select_station_and_track(int *station, int *track){
		int my_station, my_track, i;
		my_track = rand()%2;
		
		
		i = rand()%not_assigned;
		my_station = stations[i];
		
		stations[i] = stations[not_assigned-1];
		not_assigned--;
		
		*station = my_station;
		*track = my_track;
}

void *Train(void *args);

int main(int argc, char *argv[]){
	int n, m, i;
	int station, track;
	pthread_t *threads;
	arg_t *args;
	
	if(argc < 3){
		fprintf(stdout, "invalid number of parameters\n"); return -1;
	}
	
	n = atoi(argv[1]);
	m = atoi(argv[2]);
	
	if(n < m){
		fprintf(stderr, "n must be greater than m\n"); return -1;
	}
	
	stations = (int*) malloc(n * sizeof(int));
	not_assigned = n;
	
	for(i = 0; i < n; i++)
		stations[i] = i;
		
	connection_mutex = (pthread_mutex_t*) malloc(n * sizeof(pthread_mutex_t));
	for(i = 0; i < 2; i++)
		internal_mutex[i] = (pthread_mutex_t*) malloc(n * sizeof(pthread_mutex_t));
		
	for(i = 0; i < n; i++){
		pthread_mutex_init(&connection_mutex[i], NULL);
		pthread_mutex_init(&internal_mutex[0][i], NULL);
		pthread_mutex_init(&internal_mutex[1][i], NULL);
	}
	
	srand(time(NULL));
	
	threads = (pthread_t*) malloc(m * sizeof(pthread_t));
	args = (arg_t*) malloc(m * sizeof(arg_t));
	for(i = 0; i < m; i++){
		bzero(&station, sizeof(station));
		bzero(&track, sizeof(track));
		select_station_and_track(&station, &track);
		
		args[i].id = i;
		args[i].stationsN = n;
		args[i].station = station;
		args[i].track = track;
		pthread_mutex_lock(&internal_mutex[track][station]);
		pthread_create(&threads[i], NULL, Train, (void*) &args[i]);
		
	}
	
	pthread_exit(NULL);
	
}

void *Train(void *args){
	arg_t *arg = (arg_t*) args;
	int id = (*arg).id;
	int station = (*arg).station;
	int track = (*arg).track;
	int N = (*arg).stationsN;
	int sleep_time;
	char direction[50];
	int next_connection, next_station;
	
	if(track == 0)
		sprintf(direction, "CLOCKWISE");
	else
		sprintf(direction, "COUNTERCLOCKWISE");
	
	
	while(1){
		if(track = 0){
			next_connection = station;
			next_station = station+1%N;
		} else{
			next_station = station - 1;
			if(next_station < 0)
				next_station = N-1;
			next_connection = next_station;
		}
		
		fprintf(stdout, "Train n. %d in station %d going %s\n", id, station, direction);
		srand(time(NULL));
		sleep_time = rand()%6 + 1;
		sleep(sleep_time);
		
			
		/*tries to occupy connection*/
		pthread_mutex_lock(&connection_mutex[next_connection]);
		
		/*the first time the thread enters the loop, the lock has been acquired by the main,
		 * followinf times it has been acquired during the preceeding loops.
		 * Only when I enter the connection i leave last station;
		 * */
		pthread_mutex_unlock(&internal_mutex[track][station]);
		
		fprintf(stdout, "Train n. %d travelling toward station %d\n", id, next_station);
		sleep(10);
		
		/*tries to enter station*/
		pthread_mutex_lock(&internal_mutex[track][next_station]);
		/*only when I enter the station i leave the connection*/
		pthread_mutex_unlock(&connection_mutex[next_connection]);
		fprintf(stdout, "Train n. %d arrived at station %d\n", id, next_station);
		station = next_station;	
	}
	
	pthread_exit(NULL);
}











