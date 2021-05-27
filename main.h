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
int hyperSpace;//              }
pthread_mutex_t quequeMutex;//     }
int *queque;//                     }
bool *valid;//                     }
int sended_ts;
int groupedProcess_id;//for X and Y

//Initialize numbers of masters, set hyperspace full and assign master type to process
void init();
void incrementTimestamp(int income);
void sendMessage(int receiver, int type);
void sendToGroup(int messageType, masters master);
struct Message receiveMessage();

typedef enum{quequeing, waitingForY, farming} stateX;
void runningX();
void *sendingThreadX();
void *listeningThreadX();

int main(int argc, char **argv);
#endif
