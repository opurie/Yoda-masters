#ifndef MAIN_H
#define MAIN_H

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>
#include <time.h>
#include <string.h>

#define ROOT 0

#define REQ 10
#define ACK 11
#define RELEASE 12
#define JOINED 13
#define GROUP_ME 14
#define FULL 15
#define EMPTY 16

//percentage of each masters type
#define X_MASTERS 0.35
#define Y_MASTERS 0.35
#define Z_MASTERS 0.3

#define MAX_ENERGY 50

typedef enum {X=100,Y,Z} masters;

int size, rank;
MPI_Status status;

masters master;
int *queque;
int timestamp=0;
int hyperSpace;
int countOfX, countOfY, countOfZ;


void init();
void X_stuff();
void Y_stuff();
void Z_stuff();
int main(int argc, char **argv);
#endif
