#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

int write_ID_driver(char* str){
	int fd, result, len;
	char buf[10];

	if ((fd = open ("/dev/simple", O_RDWR)) == -1) {
				perror("open failed");
				return -1;
	}
 
	len = strlen(str)+1;

	if ((result = write (fd, str, len)) != len) 
	{
		perror("write failed");
		return -1;
	}

	printf("%d bytes written \n", result);
	close(fd);
}

void task1_code( ); //periodic tasks
void task2_code( );
void task3_code( );


void task4_code( );//aperiodic task

//characteristic function of the thread (timing and synchronization)

void *task1( void *);
void *task2( void *);
void *task3( void *);

void *task4( void *);

// initialization of mutexes and conditions (only for aperiodic scheduling)
pthread_mutex_t mutex_task_4 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_task_4 = PTHREAD_COND_INITIALIZER;

#define INNERLOOP 1000
#define OUTERLOOP 100

#define NPERIODICTASKS 3
#define NAPERIODICTASKS 1
#define NTASKS NPERIODICTASKS + NAPERIODICTASKS

long int periods[NTASKS];
struct timespec next_arrival_time[NTASKS];
double WCET[NTASKS]; //WCET: Worst Case Execution Time
pthread_attr_t attributes[NTASKS];
pthread_t thread_id[NTASKS];
struct sched_param parameters[NTASKS];
int missed_deadlines[NTASKS];

int main()
{
  	//set period of periodic tasks:
  	periods[0]= 300000000; //period 300 millisecond
  	periods[1]= 500000000; //period 500 millisecond
  	periods[2]= 800000000; //period 800 millisecond

  	//aperiodic tasks: set the period = 0
  	periods[3]= 0; 

	/* 	priomin: minimum priority
	priomax: maximum priority
	*/
  	struct sched_param priomax; priomax.sched_priority=sched_get_priority_max(SCHED_FIFO);
  	struct sched_param priomin; priomin.sched_priority=sched_get_priority_min(SCHED_FIFO);

	/* set priomax to the current thread (requires to be superuser) and check that the main 
	thread is executed with superuser privileges
	*/
  	if (getuid() == 0) pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomax);

	/* Execute every task separately to measure execution time of each one in order to compute
	the worst case execution time (WCET) of each task
	*/
 	int i;
	struct timespec time_1, time_2; //variables used to read clock
  	for (i =0; i < NTASKS; i++){
		//struct timespec time_1, time_2; //variables used to read clock
		clock_gettime(CLOCK_REALTIME, &time_1);

		if (i==0) task1_code();
		if (i==1) task2_code();
		if (i==2) task3_code();
		if (i==3) task4_code();

		clock_gettime(CLOCK_REALTIME, &time_2);

		WCET[i]= 1000000000*(time_2.tv_sec - time_1.tv_sec) +(time_2.tv_nsec-time_1.tv_nsec);//compute WCET
		printf("\nWorst Case Execution Time %d=%f \n", i, WCET[i]);
    	}

    // compute U value:
	double U = WCET[0]/periods[0]+WCET[1]/periods[1]+WCET[2]/periods[2];

    // compute Ulub value:
	//double Ulub = 1; //case with harmonic relationships between periods
	double Ulub = NPERIODICTASKS*(pow(2.0,(1.0/NPERIODICTASKS)) -1);//case without  harmonic relationships between periods
	
	//check sufficient conditions: if they are not satisfied, exit  
  	if (U > Ulub) {printf("\n U=%lf Ulub=%lf Non schedulable Task Set", U, Ulub); return(-1);} //cond. not satisfied
  	printf("\n U=%lf Ulub=%lf Scheduable Task Set", U, Ulub); fflush(stdout);
  	sleep(5);

  	if (getuid() == 0) pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomin); /* 
	set minimum priority to the current thread: assign higher priorities to periodic threads to be soon created
	pthread_setschedparam */
  	
  	for (i =0; i < NPERIODICTASKS; i++){//set attributes of each task (including scheduling policy and priority)
      		pthread_attr_init(&(attributes[i])); //initializa the attribute structure of task i
      		pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED); /*
		set the attributes to tell the kernel that the priorities and policies are explicitly chosen,
		not inherited from the main thread (pthread_attr_setinheritsched) 
		*/
			pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);/*
			set the attributes to set the SCHED_FIFO policy (pthread_attr_setschedpolicy)
			*/

		//set parameters to assign priority inversely proportional to the period:
      		//parameters[i].sched_priority = priomin.sched_priority+NTASKS - i;
      		parameters[i].sched_priority = sched_get_priority_max(SCHED_FIFO) - i;

      		pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));/*
			//set attributes and parameters of the current thread (pthread_attr_setschedparam)
			*/
    	}
  	for (int i = NPERIODICTASKS; i < NTASKS; i++){// aperiodic tasks
      		pthread_attr_init(&(attributes[i]));
      		pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);      		
      		parameters[i].sched_priority = 0; //set minimum priority
      		pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
    	}	
  	int iret[NTASKS]; //variable to contain return values of pthread_create
	struct timespec time_1; //variables to read the current time
	clock_gettime(CLOCK_REALTIME, &time_1);
  	
  	for (i = 0; i < NPERIODICTASKS; i++){//set next arrival time for each task
	/*
	This is not the beginning of the first period, but the end of the first period and beginning of the next one. 
	*/
		long int next_arrival_nanoseconds = time_1.tv_nsec + periods[i];
	//then we compute the end of the first period and beginning of the next one
		next_arrival_time[i].tv_nsec= next_arrival_nanoseconds%1000000000;
		next_arrival_time[i].tv_sec= time_1.tv_sec + next_arrival_nanoseconds/1000000000;
		missed_deadlines[i] = 0;
    	}
	// create all threads(pthread_create)
  	iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), task1, NULL);
  	iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), task2, NULL);
  	iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task3, NULL);
   	iret[3] = pthread_create( &(thread_id[3]), &(attributes[3]), task4, NULL);

  	// join all threads (pthread_join)
  	pthread_join( thread_id[0], NULL);
  	pthread_join( thread_id[1], NULL);
  	pthread_join( thread_id[2], NULL);

  	for (i = 0; i < NTASKS; i++){// set the next arrival time for each task
		printf ("\nMissed Deadlines Task %d=%d", i, missed_deadlines[i]); fflush(stdout);
    	}
  	exit(0);
}


void task2_code(){
	write_ID_driver("[2");
	double uno;
  	for (i = 0; i < OUTERLOOP; i++){
		for (j = 0; j < INNERLOOP; j++) uno = rand()*rand()%10;		
  	}
  	
  	if (uno == 0){//execute ap. task4 when uno==0
		printf(":ex(4)");fflush(stdout);
//     	pthread_mutex_lock(&mutex_task_4);
		pthread_cond_signal(&cond_task_4);
//      pthread_mutex_unlock(&mutex_task_4);
	}
  	write_ID_driver("2]");
}
void *task2( void *ptr){
	// set thread affinity, that is the processor on which threads shall run
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

   	//execute the task one hundred times... it should be an infinite loop (too dangerous)
  	for (i=0; i < 100; i++){
		task2_code();
		// it would be nice to check if we missed a deadline here... why don't you try by yourself?

		// sleep until the end of the current period (which is also the start of the new one)
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[1], NULL);
		// the thread is ready and can compute the end of the current period for the next iteration
		long int next_arrival_nanoseconds = next_arrival_time[1].tv_nsec + periods[1];
		next_arrival_time[1].tv_nsec= next_arrival_nanoseconds%1000000000;
		next_arrival_time[1].tv_sec= next_arrival_time[1].tv_sec + next_arrival_nanoseconds/1000000000;
	}
}
void task1_code()
{
	write_ID_driver("[1");
	double uno;
  	for (i = 0; i < OUTERLOOP; i++){
		for (j = 0; j < INNERLOOP; j++) uno = rand()*rand()%10;
	}
	write_ID_driver("1]");
}
void *task1( void *ptr )
{
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

  	for (i=0; i < 100; i++){
		task1_code();
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[0], NULL);
		long int next_arrival_nanoseconds = next_arrival_time[0].tv_nsec + periods[0];
		next_arrival_time[0].tv_nsec= next_arrival_nanoseconds%1000000000;
		next_arrival_time[0].tv_sec= next_arrival_time[0].tv_sec + next_arrival_nanoseconds/1000000000;
	}
}
void task3_code()
{
	write_ID_driver("[3");
	double uno;
  	for (i = 0; i < OUTERLOOP; i++){
		for (j = 0; j < INNERLOOP; j++) uno = rand()*rand()%10;
	}
  	write_ID_driver("3]");
}
void *task3( void *ptr)
{
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

  	for (i=0; i < 100; i++){
		task3_code();
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[2], NULL);
		long int next_arrival_nanoseconds = next_arrival_time[2].tv_nsec + periods[2];
		next_arrival_time[2].tv_nsec= next_arrival_nanoseconds%1000000000;
		next_arrival_time[2].tv_sec= next_arrival_time[2].tv_sec + next_arrival_nanoseconds/1000000000;
    }
}
void task4_code()
{
  	write_ID_driver("[4");
	for (int i = 0; i < OUTERLOOP; i++){
		for (int j = 0; j < INNERLOOP; j++) double uno = rand()*rand();	
	}
  	write_ID_driver("4]");
}
void *task4( void *)
{
	cpu_set_t cset;
	CPU_ZERO (&cset);
	CPU_SET(0, &cset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset); 
	while (1)
    	{
		// wait for the proper condition to be signalled
		// See below why mutexes have been commented
//		pthread_mutex_lock(&mutex_task_4);
		pthread_cond_wait(&cond_task_4, &mutex_task_4);
//		pthread_mutex_unlock(&mutex_task_4);
		// execute the task code
 		task4_code();
	}
}