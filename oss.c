
/*
CS_4760
Anthony Nguyen
04/03/2022
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include "queue.h"

#define QUANTUM 40000

struct timer
{
	int seconds;
	int ns;
};

struct PCB
{
	struct timer CPUTime;
	struct timer totalTime;
	struct timer prevTime;
	int priority;
	pid_t pid;
};

struct schedule
{
	int quantum;
	pid_t pid;
};

int errno;
char errmsg[200];
int shmidTime;
struct timer *shmTime;
int shmidPCB;
struct PCB *shmPCB;
int shmidSched;
struct schedule *shmSched;
sem_t * schedSem;
sem_t * slaveSem;



void sigIntHandler(int signum)
{
	/* Send a message*/
	snprintf(errmsg, sizeof(errmsg), "OSS: Caught SIGINT! Killing all child processes.");
	perror(errmsg);	
	
	/* Deallocate shared memory */
	errno = shmdt(shmTime);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmTime)");
		perror(errmsg);	
	}
	
	errno = shmctl(shmidTime, IPC_RMID, NULL);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidTime)");
		perror(errmsg);	
	}

	errno = shmdt(shmPCB);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmPCB)");
		perror(errmsg);	
	}
	
	errno = shmctl(shmidPCB, IPC_RMID, NULL);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidPCB)");
		perror(errmsg);	
	}
	
	errno = shmdt(shmSched);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmSched)");
		perror(errmsg);	
	}
	
	errno = shmctl(shmidSched, IPC_RMID, NULL);
	if(errno == -1)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidSched)");
		perror(errmsg);	
	}
	
	/* Close the Semaphore */
	sem_unlink("schedSem");   
    sem_close(schedSem);  
	sem_unlink("slaveSem");   
    sem_close(slaveSem);  
	/* Out the program */
	exit(signum);
}

int main (int argc, char *argv[]) {
int o;
int i;
int maxSlaves = 1;
int numSlaves = 0;
int numProc = 0;
int children[18] = {0};
int maxTime = 5;
int createNext;
char *sParam = NULL;
char *lParam = NULL;
char *tParam = NULL;
char timeArg[33];
char pcbArg[33];
char schedArg[33];
char indexArg[33];
pid_t pid = getpid();
key_t keyTime = 5309;
key_t keyPCB = 8311;
key_t keySched = 2410;
FILE *fp;
char *fileName = "./msglog.out";
signal(SIGINT, sigIntHandler);
time_t start;
time_t stop;

/* random number */
srand(pid * time(NULL));


/* Menu */
while ((o = getopt (argc, argv, "hs:l:t:")) != -1)
{
	switch (o)
	{
		case 'h':
			snprintf(errmsg, sizeof(errmsg), "Project 4 - Anthony Nguyen - Process Scheduling\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "Options:\n\n-h\tFor Help option: display how project should be run\nUsage:\t ./oss -h\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "-s\tFor Slave option: sets the number of slave process.\nUsage:\t ./oss -s 10\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "-l\tFor Log file: specify a particular name for the log file\nUsage:\t ./oss -l output.txt\n\n");
			printf(errmsg);
			snprintf(errmsg, sizeof(errmsg), "-t\tFor TimeOut option: sets the maximum seconds before the system terminates\nUsage:\t ./oss -t 5\n");
			printf(errmsg);
			exit(1);
			break;
		case 's':
			sParam = optarg;
			break;
		case 'l':
			lParam = optarg;
			break;
		case 't':
			tParam = optarg;
			break;
		case '?':
			if (optopt == 's' || optopt == 'l' || optopt == 't')
			{
				snprintf(errmsg, sizeof(errmsg), "OSS: Option -%c requires an argument.", optopt);
				perror(errmsg);
			}
			return 1;
		default:
			break;
	}	
}


if(sParam != NULL)
{
	maxSlaves = atoi(sParam);
}
if(maxSlaves <= 0)
{
	maxSlaves = 1;
}
if(maxSlaves > 18)
{
	maxSlaves = 18;
}

/* name for the log file */
if(lParam != NULL)
{
	fp = fopen(lParam, "w");
	if(fp == NULL)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: fopen(lParam).");
		perror(errmsg);
	}
}
else
{
	fp = fopen(fileName, "w");
	if(fp == NULL)
	{
		snprintf(errmsg, sizeof(errmsg), "OSS: fopen(fileName).");
		perror(errmsg);
	}
}


if(tParam != NULL)
{
	maxTime = atoi(tParam);
}

/********************MEMORY ALLOCATION********************/
/* Create shared memory segment for a struct timer */
shmidTime = shmget(keyTime, sizeof(struct timer), IPC_CREAT | 0666);
if (shmidTime < 0)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmget(keyTime...)");
	perror(errmsg);
	exit(1);
}

/* Point shmTime to shared memory */
shmTime = shmat(shmidTime, NULL, 0);
if ((void *)shmTime == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmat(shmidTime)");
	perror(errmsg);
    exit(1);
}

/* Create shared memory segment for a struct Process Control Block */
shmidPCB = shmget(keyPCB, sizeof(struct PCB)*18, IPC_CREAT | 0666);
if (shmidPCB < 0)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmget(keyPCB...)");
	perror(errmsg);
	exit(1);
}


shmPCB = shmat(shmidPCB, NULL, 0);
if ((void *)shmPCB == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmat(shmidPCB)");
	perror(errmsg);
    exit(1);
}

/* Create shared memory segment for a struct Schedule */
shmidSched = shmget(keySched, sizeof(struct schedule), IPC_CREAT | 0666);
if (shmidSched < 0)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmget(keySched...)");
	perror(errmsg);
	exit(1);
}


shmSched = shmat(shmidSched, NULL, 0);
if ((void *)shmSched == (void *)-1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmat(shmidSched)");
	perror(errmsg);
    exit(1);
}

sprintf(timeArg, "%d", shmidTime);
sprintf(pcbArg, "%d", shmidPCB);
sprintf(schedArg, "%d", shmidSched);


/* Set the time to 00.00 */
shmTime->seconds = 0;
shmTime->ns = 0;


for(i =0; i < 18; i++)
{
	shmPCB[i].CPUTime.seconds = 0;
	shmPCB[i].CPUTime.ns = 0;
	shmPCB[i].totalTime.seconds = 0;
	shmPCB[i].totalTime.ns = 0;
	shmPCB[i].prevTime.seconds = 0;
	shmPCB[i].prevTime.ns = 0;
	shmPCB[i].priority = 0;
	shmPCB[i].pid = 0;
}

shmSched->quantum = 0;
shmSched->pid = 0;

schedSem=sem_open("schedSem", O_CREAT | O_EXCL, 0644, 1);
if(schedSem == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "OSS: sem_open(schedSem)...");
	perror(errmsg);
    exit(1);
}

slaveSem=sem_open("slaveSem", O_CREAT | O_EXCL, 0644, maxSlaves);
if(slaveSem == SEM_FAILED) {
	snprintf(errmsg, sizeof(errmsg), "OSS sem_open(slaveSem)...");
	perror(errmsg);
	exit(1);
}

sem_wait(schedSem);



/* timer */
start = time(NULL);
do
{
	
	for(i = 0; i < maxSlaves-1; ++i)
	{
		if(shmPCB[i].pid == 0)
		{
			sprintf(indexArg, "%d", i);
			break;
		}
	}
	
	/* Create a new child process  */
	if(numSlaves < maxSlaves)
	{
		numSlaves += 1;
		numProc += 1;
		pid = fork();
		if(pid == 0)
		{
			pid = getpid();
			shmPCB[i].pid = pid;
			execl("./user", "user", timeArg, pcbArg, schedArg, indexArg, (char*)0);
		}
	}
	
	/* Schedule next process */
	shmSched->quantum = QUANTUM;
	shmSched->pid = shmPCB[i].pid;
	
	sem_post(schedSem);
	
	shmTime->ns += (rand()%1000) + 1;
	if(shmTime->ns >= 1000000000)
	{
		shmTime->ns -= 1000000000;
		shmTime->seconds += 1;
	}
	shmTime->seconds += 1;
	stop = time(NULL);

}while(stop-start < maxTime && numProc < 100);

for(i = 0; i < maxSlaves; i++)
{
	printf("shmPID[%d] = %d\n", i, shmPCB[i].pid);
}




/* Kill all slave processes */
for(i = 0; i < maxSlaves; i++)
{
	kill(shmPCB[i].pid, SIGINT);
}
/********************DEALLOCATE MEMORY********************/
errno = shmdt(shmTime);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmTime)");
	perror(errmsg);	
}

errno = shmctl(shmidTime, IPC_RMID, NULL);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidTime)");
	perror(errmsg);	
}

errno = shmdt(shmPCB);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmPCB)");
	perror(errmsg);	
}

errno = shmctl(shmidPCB, IPC_RMID, NULL);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidPCB)");
	perror(errmsg);	
}

errno = shmdt(shmSched);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmdt(shmSched)");
	perror(errmsg);	
}

errno = shmctl(shmidSched, IPC_RMID, NULL);
if(errno == -1)
{
	snprintf(errmsg, sizeof(errmsg), "OSS: shmctl(shmidSched)");
	perror(errmsg);	
}


/* Close */
sem_unlink("schedSem");   
sem_close(schedSem);
sem_unlink("slaveSem");   
sem_close(slaveSem);  

printf("OSS: Program completed successfully!\n");
return 0;
}
