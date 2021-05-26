#include "main.h"

void init(){
    //init number of Yodas
    countOfX = size*X_MASTERS;
    countOfY = size*Y_MASTERS;
    countOfZ = size*Z_MASTERS;
    int suma = countOfZ + countOfY + countOfX;
    if(suma < size){
        countOfZ += size - suma;}
    else if(suma > size){
        if((suma-size)%2==0){
            countOfX -= (suma-size)/2;
            countOfY -= (suma-size)/2;
        }else{
            countOfX -= (suma-size)/2;
            countOfY -= ((suma-size)/2 + 1);
        }
    }
    //assign master type
    if(rank < countOfX){
        master = X;}
    else if(rank >= countOfX && rank < countOfX+countOfY){
        master = Y;}
    else{
        master = Z;}
    //set hyperspace full
    hyperSpace = MAX_ENERGY;    
}
void sendMessage(int sender, int receiver, int type){
    struct Message message;
    message.sender= sender;
    message.timestamp = timestamp;
    message.type = type;
    MPI_Send(&message, 1, mpi_message_type, receiver, 100, MPI_COMM_WORLD);
}
void sendToGroup(int id, masters master, int messageType){
    if(master == X){
        timestamp++;
        for(int i = 0; i<countOfX;i++){
            if(id == i)
                continue;
            sendMessage(id, i, messageType);
        }
    }else if(master == Y){
        timestamp++;
        for(int i = 0; i < countOfY; i++){
            if(id == i+countOfX)
                continue;
            sendMessage(id, i+countOfX, messageType);
        }
    }else if(master == Z){
        timestamp++;
        for(int i = 0; i < countOfZ; i++){
            if(id == i+countOfX+countOfY)
                continue;
            sendMessage(id, i+countOfX+countOfY, messageType);
        }
    }
}
int main(int argc, char **argv){
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    init();
    MPI_Finalize();
    return 0;
}