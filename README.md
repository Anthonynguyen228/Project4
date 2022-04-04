/*
CS_4760
Anthony Nguyen
04/03/2022
*/

Process Scheduling
The goal of this homework is to learn about process scheduling inside an operating system. You will work on the specified scheduling algorithm and simulate its performance.

In this project, you will simulate the process scheduling part of an operating system. You will implement time-based
scheduling, ignoring almost every other aspect of the os. In particular, this project will involve a main executable managing
the execution of concurrent user processes just like a scheduler/dispatcher does in an os. You may use message queues or
semaphores for synchronization, depending on your choice.




For this project, I did the following files:
oss.c
queue.h
User.c
Makefile
README

You must first create the executable program if trying to use this code. Use the 'make' command to create the 'oss' executable.

You must implement at least the following command line arguments:

./oss -h 
For Help option: display how project should be run


./oss -s t
For Slave option: sets the number of slave process


./oss -l f
For Log file: specify a particular name for the log file

./oss -t 
For TimeOut option: sets the maximum seconds before the system terminates


