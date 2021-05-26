#ifndef MAIN_H
#define MAIN_H

#include "consts.h"

int size, rank;
MPI_Status status;
MPI_Datatype mpi_message_type;

masters master;
int timestamp=0;
int hyperSpace;
int countOfX, countOfY, countOfZ;

//Initialize numbers of masters, set hyperspace full and assign master type to process
void init();
//included incrementing timestamp
void sendMessage(int sender, int receiver, int type);
struct Message receiveMessage();
void *listeningThread(int id);
void sendToGroup(int id, masters master, int messageType);
int main(int argc, char **argv);
#endif
