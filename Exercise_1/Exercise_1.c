//including all the headers related to POSIX RT patch and pthreads
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <math.h>
#include <sys/param.h>
#include <stdint.h>

#define NSEC_PER_SEC (1000000000) //conversion factors for nanoseconds to seconds
#define NSEC_PER_MSEC (1000000)   //conversion factors for nanoseconds to milliseconds
#define NSEC_PER_MICROSEC (1000)

#define FIB_LIMIT_FOR_32_BIT 47

uint32_t seqIterations = FIB_LIMIT_FOR_32_BIT; //variables to be used in FIB_TEST
uint32_t idx = 0, jdx = 1;        //thread IDs
uint32_t fib = 0, fib0 = 0, fib1 = 1;

//Definition of the FIB_TEST function used as synthetic load
#define FIB_TEST(seqCnt, iterCnt) \
for(idx=0; idx < iterCnt; idx++)  \
  {                               \
    while(jdx < seqCnt)           \
      {                           \
        if (jdx == 0)             \
          {                       \
            fib = 1;              \
          }                       \
          else                    \
          {                       \
            fib0 = fib1;          \
            fib1 = fib;           \
            fib = fib0 + fib1;    \
          }                       \
          jdx++;                  \
        }                         \
      }

//variables to define threads and attributes for the threads.
pthread_t fib10_thread, fib20_thread;
pthread_attr_t fib10_sched_attr, fib20_sched_attr, main_sched_attr;
sem_t fib10_sem, fib20_sem;//variables to define semaphores
struct timespec start_time = {0,0};//struct to get time from clock_gettime funtion
int rt_max_prio, rt_min_prio, abortTest_10 = 0,abortTest_20=0;  //variables for assignment of priorities
//structures to pass parameters to threads
struct sched_param rt_param;
struct sched_param nrt_param;
struct sched_param fib10_param;
struct sched_param fib20_param;
struct sched_param main_param;

//Function to calculate the elapsed time between start and finish
int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  if(dt_sec >= 0)
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
  }
  else
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
  }
}
//Function to print the schedular information
void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
	   printf("Pthread Policy is SCHED_FIFO\n");
	   break;
     case SCHED_OTHER:
	   printf("Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:
	   printf("Pthread Policy is SCHED_OTHER\n");
	   break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}
// Thread for a load of almost 10 milliseconds
void *fib10(void *threadid)
{
  struct timespec stop_time = {0, 0};
  struct timespec time_diff = {0, 0};
   while(!abortTest_10)
   {
	   sem_wait(&fib10_sem);                   //wait for the semaphore
	   FIB_TEST(seqIterations, 1304000);
     clock_gettime(CLOCK_REALTIME, &stop_time);   //get system time
     delta_t(&stop_time, &start_time, &time_diff);
     printf("Fib10, Time stamp: %ld msec\n", (time_diff.tv_nsec / NSEC_PER_MSEC));
   }
}
// Thread for a load of almost 20 milliseconds
void *fib20(void *threadid)
{
  struct timespec stop_time = {0, 0};
  struct timespec time_diff = {0, 0};
   while(!abortTest_20)
   {
	   sem_wait(&fib20_sem);               //wait for the semaphore
	   FIB_TEST(seqIterations, 2208000);
     clock_gettime(CLOCK_REALTIME, &stop_time); //get system time
     delta_t(&stop_time, &start_time, &time_diff);
     printf("Fib20, Time stamp: %ld msec\n", (time_diff.tv_nsec / NSEC_PER_MSEC));
   }
}

int main (void)
{
  int rc, scope,i;
  useconds_t t_10,t_20;
  sem_init (&fib10_sem, 0, 1);    //initialize the semaphores
  sem_init (&fib20_sem, 0, 1);
  struct timespec main_stop = {0, 0};//structure to get time after execution of main
  struct timespec main_time_diff = {0, 0};//structure for execution time
  t_10 = 10000;                         // variabkes of value 10000/2000 to produce 10/20 milisec delay 
  t_20 = 20000;
  printf("Before adjustments to scheduling policy:\n");
  print_scheduler();

//setting the attributes for the threads fib10, fib20 and main thread
  pthread_attr_init (&fib10_sched_attr);//initialize the attributes for the threads
  pthread_attr_init (&fib20_sched_attr);
  pthread_attr_init (&main_sched_attr);
  pthread_attr_setinheritsched (&fib10_sched_attr, PTHREAD_EXPLICIT_SCHED);//set the inheritance of attributes from the parent thread
  pthread_attr_setschedpolicy (&fib10_sched_attr, SCHED_FIFO);              //setting the scheduling policy to SCHED_FIFO
  pthread_attr_setinheritsched (&fib20_sched_attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy (&fib20_sched_attr, SCHED_FIFO);
  pthread_attr_setinheritsched (&main_sched_attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy (&main_sched_attr, SCHED_FIFO);

  rt_max_prio = sched_get_priority_max (SCHED_FIFO);//get max and min priorities from SCHED_FIFO
  rt_min_prio = sched_get_priority_min (SCHED_FIFO);

//setting max priority to main and next priority to fib10 and fib20
  rc=sched_getparam (getpid(), &nrt_param);
  main_param.sched_priority = rt_max_prio;
  fib10_param.sched_priority = rt_max_prio-1;
  fib20_param.sched_priority = rt_max_prio-2;

//start the main thread execution
  rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
  if (rc)
    {
      printf("ERROR; sched_setscheduler rc is %d\n", rc); perror(NULL); exit(-1);
    }
  printf("After adjustments to scheduling policy:\n");
  print_scheduler();

  printf("min prio = %d, max prio = %d\n", rt_min_prio, rt_max_prio);

//get the scope of the thread execution
  pthread_attr_getscope (&fib10_sched_attr, &scope);
  if(scope == PTHREAD_SCOPE_SYSTEM)
    printf("PTHREAD SCOPE SYSTEM\n");
  else if (scope == PTHREAD_SCOPE_PROCESS)
    printf("PTHREAD SCOPE PROCESS\n");
  else
    printf("PTHREAD SCOPE UNKNOWN\n");

// Allocating parameters to attributes of threads
  pthread_attr_setschedparam(&fib10_sched_attr,&fib10_param);
  pthread_attr_setschedparam (&fib20_sched_attr,&fib20_param);
  pthread_attr_setschedparam (&main_sched_attr,&main_param);

//start the time for profiling and start the threads fib10 and fib20
  clock_gettime(CLOCK_REALTIME, &start_time);
  rc = pthread_create(&fib10_thread, &fib10_sched_attr,fib10 , (void *)0);
  if (rc)
    {
      printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);
    }
  rc = pthread_create(&fib20_thread, &fib20_sched_attr,fib20 , (void *)0);
  if (rc)
    {
      printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);
    }
  /* Basic sequence of releases after CI */
  usleep(t_20);     //delay for 20 milliseconds
  sem_post(&fib10_sem);//incrementing semaphore
  usleep(t_20);
  sem_post(&fib10_sem);
  usleep(t_10);
  abortTest_20 = 1;   //aborting the fib20 thread
  sem_post(&fib20_sem);
  usleep(t_10);
  sem_post(&fib10_sem);
  usleep(t_20);
  abortTest_10 = 1;
  sem_post(&fib10_sem);
  usleep(t_20);

  clock_gettime(CLOCK_REALTIME, &main_stop);  //get ending time from the system
  delta_t(&main_stop, &start_time, &main_time_diff);
  printf("Test Conducted over %ld msec\n", (main_time_diff.tv_nsec / NSEC_PER_MSEC));

//wait for the threads to finish
  pthread_join(fib10_thread,NULL);
  pthread_join(fib20_thread,NULL);

//destroy the threads for exiting
  if(pthread_attr_destroy(&fib10_sched_attr) != 0)
    perror("attr destroy");
  if(pthread_attr_destroy(&fib20_sched_attr) != 0)
    perror("attr destroy");
//destroying semaphores
  sem_destroy(&fib10_sem);
  sem_destroy(&fib20_sem);

  rc=sched_setscheduler(getpid(), SCHED_OTHER, &nrt_param);
  printf("completed");
}
