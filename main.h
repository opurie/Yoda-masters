#ifndef MAIN_H
#define MAIN_H

#include "consts.h"

int size, id;
MPI_Datatype mpi_message_type;

int countOfX, countOfY, countOfZ;

masters master;
pthread_mutex_t tsMutex;// }
int timestamp=0;//         }
pthread_mutex_t hsMutex;//     }
int hyperSpace;
int sended_ts;
int groupedProcess_id;//for X and Y

//Initialize numbers of masters, set hyperspace full and assign master type to process
void init();
void incrementTimestamp(int income);
void sendMessage(int receiver, int type, int in);
void sendToGroup(int messageType, masters master, int n);
struct Message receiveMessage();

typedef enum{queueing, waitingForXs, waitingForY, farming} stateX;
stateX state;
int receivedACKs = 0;
void initCustomMessage();
void *listeningX();
int queuePlace(int acks, masters master, int *queue, int *inQue);
void runningX();


int main(int argc, char **argv);
#endif
