#ifndef CONSTSH
#define CONSTSH

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>
#include <time.h>
#include <string.h>

#define ROOT 0

//Messages
#define REQ 10
#define ACK 11
#define RELEASE 12
#define JOINED 13
#define GROUP_ME 14
#define FULL 15
#define EMPTY 16

//percentage of each masters type, sum must be 1.0
#define X_MASTERS 0.35
#define Y_MASTERS 0.35
#define Z_MASTERS 0.3

#define MAX_ENERGY 50
#define MIN_ENERGY 0


struct Message{
    int sender;
    int timestamp;
    int type;
};

typedef enum {X=100,Y,Z} masters;
#endif