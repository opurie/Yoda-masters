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
void sendMessage(int receiver, int type){
    struct Message message;
    message.sender= id;
    message.timestamp = timestamp;
    message.type = type;
    MPI_Send(&message, 1, mpi_message_type, receiver, 100, MPI_COMM_WORLD);
}
struct Message receiveMessage(){
    struct Message message;
    MPI_Status status;
    MPI_Recv(&message, 1, mpi_message_type, MPI_ANY_SOURCE, 100, MPI_COMM_WORLD, &status);
    return message;
}
void incrementTimestamp(int income){
    if(income > timestamp)
        timestamp = income+1;
    else
        timestamp++;
}
void sendToGroup(int messageType, masters master){
    if(master == X){
        for(int i = 0; i<countOfX;i++){
            if(id == i)
                continue;
            sendMessage(id, i, messageType);
        }
    }else if(master == Y){
        for(int i = 0; i < countOfY; i++){
            if(id == i+countOfX)
                continue;
            sendMessage(id, i+countOfX, messageType);
        }
    }else if(master == Z){
        for(int i = 0; i < countOfZ; i++){
            if(id == i+countOfX+countOfY)
                continue;
            sendMessage(id, i+countOfX+countOfY, messageType);
        }
    }
}
void *listeningThreadX(){
    struct Message message;
    while(1){
        message = receiveMessage();
        if(message.sender < countOfX){
            //from X
            pthread_mutex_lock(&tsMutex);
            incrementTimestamp(message.timestamp);
            queque[message.sender] = message.timestamp;
            pthread_mutex_unlock(&tsMutex);
        }else{
            //from Y
        }

    }
}
void runningX(){
    *queque = malloc(countOfX * sizeof(int));
    *valid = malloc(countOfX * sizeof(bool));
    memset(valid, 1, countOfX);
    memset(queque, 0, countOfX);
    
    pthread_t thread, thread2;
    pthread_create(&thread, NULL, sendingThreadX);
    pthread_create(&thread2, NULL, listeningThreadX);

}
int main(int argc, char **argv){
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    init();

    if(master == X)
        runningX();


    MPI_Finalize();
    return 0;
}