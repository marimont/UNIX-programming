#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pthread.h"
#include "semaphore.h"

#define NUM_OFFICES 4

typedef struct Info{
  int id;
  int office_no;
  int urgent;
}Info;

typedef struct Num_students{
  pthread_mutex_t lock;
  int num;
}Num_students;

typedef struct Buffer{
Info *buffer;
pthread_mutex_t lock;
pthread_cond_t *notfull;
pthread_cond_t *notempty;
int in;
int out;
int count;
int dim;
}Buffer;

typedef struct Cond{
  pthread_mutex_t lock;  
  pthread_cond_t  *cond;	// to be allocated
  int             *urgent;	// NUM_OFFICES to be allocated
  int             normal;
} Cond;

Buffer *B_init (int dim);
Cond * cond_init(int n);

void send(Buffer *buf, Info info);
Info receive(Buffer *buf);

Buffer *normal_Q, *special_Q, **urgent_Q, **answer_Q;
Cond *cond;

Num_students num_students;
int k;
