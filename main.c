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
    if(id < countOfX){
        master = X;}
    else if(id >= countOfX && id < countOfX+countOfY){
        master = Y;}
    else{
        master = Z;}
    //set hyperspace full
    hyperSpace = MAX_ENERGY;    
}
void initCustomMessage(){
    int block[4] = {1,1,1,1};
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint offsets[4];
    offsets[0] = offsetof(struct Message, sender);
    offsets[1] = offsetof(struct Message, timestamp);
    offsets[2] = offsetof(struct Message, type);
    offsets[3] = offsetof(struct Message, inQue);
    MPI_Type_create_struct(4, block, offsets, types, &mpi_message_type);
    MPI_Type_commit(&mpi_message_type);
}
void changeState(State s){
    state = s;
}
void sendMessage(int receiver, int type, int in){
    struct Message message;
    message.sender= id;
    message.timestamp = timestamp;
    message.type = type;
    message.inQue = in;
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
void sendToGroup(int messageType, masters master, int n){
    if(master == X){
        for(int i = 0; i<countOfX;i++){
            if(id == i)
                continue;
            sendMessage(i, messageType, n);
        }
    }else if(master == Y){
        for(int i = 0; i < countOfY; i++){
            if(id == i+countOfX)
                continue;
            sendMessage(i+countOfX, messageType, n);
        }
    }else if(master == Z){
        for(int i = 0; i < countOfZ; i++){
            if(id == i+countOfX+countOfY)
                continue;
            sendMessage(i+countOfX+countOfY, messageType, n);
        }
    }
}
//k = {1..countOf(XYZ)}
int queuePlace(masters master, int *queue, int *inQue, int offset){
    int k = 1;
    int ys = countOfX, zs = countOfX+countOfY;
    if(master == X){
        for(int i = 0; i < countOfX; i++){
            if(i == id) continue;
            if(queue[i] < queue[id] && inQue[i]==1)
                k++;
            else if(queue[i] == queue[id] && i < id && inQue[i]==1)
                k++;
        }
    }
    if(master == Y){
        for(int i = 0; i < countOfY; i++){
            if(id - ys == i) continue;
            if(queue[i] < queue[id - ys] && inQue[i]==1)
                k++;
            else if(queue[i] == queue[id - ys] && i < id - ys && inQue[i]==1)
                k++;
        }
    }
    if(master == Z){
        for(int i = 0; i<countOfZ;i++){
            if(id - zs == i) continue;
            if(queue[i] < queue[id - zs] && inQue[i]==1)
                k++;
            else if(queue[i] == queue[id - zs] && i < id-zs && inQue[i]==1)
                k++;
        }
    }
    return k + offset;
}
/*
queueing - kolejkowanie się, czekanie na wszystkie ACKi
waitingForY - odebraliśmy wszystkie ACKi i czekamy na kolej wysłania prośby do Y
readyToFarm - wysłaliśmy prośbę do Y
farming - Yi odesłał nam wiadomość i jesteśmy z nim w parze
*/
void runningX(){
    int *queue= malloc(countOfX * sizeof(int));
    int *inQue= malloc(countOfX * sizeof(int)); 
    memset(inQue, 1, countOfX);
    memset(queue, 0, countOfX);
    struct Message message;
    int k, offset=0;
    int minimum = countOfX;
    if(countOfX > countOfY) minimum = countOfY;

    //początek, proces rozsyła żądanie do Xs aby otrzymać Y
start:
    changeState(queueing);
    incrementTimestamp(0);
    sendToGroup(REQ, X, 0);
    queue[id]=timestamp;
    groupedProcess_id = -1;
    int sendedToY=0, receivedACKs = 0;
    k=0;
    //pętla zarządzająca odbiorem wiadomości
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        switch (message.type)
        {
        case REQ:
            queue[message.sender] = message.timestamp;
            inQue[message.sender] = 1;
            sendMessage(message.sender, ACK, 0);
            break;
        case ACK:
            if(message.timestamp > queue[id])
                receivedACKs++;
            //otrzymano wszystkie ACKi
            if(receivedACKs==countOfX-1){
                k = queuePlace(master, queue, inQue, offset);
                changeState(waitingForY);    
                incrementTimestamp(0);
                printf("[X - %d] readyToFarm, kolejka - %d\n", id, k);
                sendToGroup(GROUP_ME, Y, k);
                changeState(readyToFarm);
            }
            break;
        case RELEASE_X:
            inQue[message.sender] = 0;
            break;
        case JOINED:
            changeState(farming);
            if(groupedProcess_id > 0)
                printf("[ERROR X - %d] grouped - %d, want to group - %d\n", id, groupedProcess_id, message.sender);
            else{
                groupedProcess_id = message.sender;
                printf("[X - %d] FARMING - %d\n", id, groupedProcess_id);
            }
            break;
        case RELEASE_Y:
            incrementTimestamp(0);
            printf("[X - %d] RELEASED\n", id);
            offset += countOfX;
            sendToGroup(RELEASE_X, X, 0);
            goto start;
            break;
        default:
            break;
        }
    }

}
int findX(int k, int *xtab){
    for(int i=0;  i<countOfX; i++){
        if(k == xtab[i])
            return i;
    }
    return -1;
}

int farmingY(int k, int* queue, int *inQue, int* xtab, int offset){
    if(state == waitingForX){
        groupedProcess_id = findX(k, xtab);
        if(groupedProcess_id != -1){
            printf("[Y - %d] readyToFarm - %d, kolejka - %d\n",id, groupedProcess_id, k);
            incrementTimestamp(0);
            changeState(readyToFarm);
            sendMessage(groupedProcess_id, JOINED, 0);
        }
    }
    if(state == readyToFarm && k - offset <= hyperSpace){
        printf("[Y - %d] farming, x - %d, hyperspace - %d\n", id, groupedProcess_id, hyperSpace);
        changeState(farming);
        hyperSpace--;
        sleep(TIME_IN);
        incrementTimestamp(0);
        sendMessage(groupedProcess_id, RELEASE_Y, 0);
        sendToGroup(RELEASE_Y, Y, groupedProcess_id);
        if(hyperSpace - (k - offset) == -1){
            incrementTimestamp(0);
            printf("[Y - %d] EMPTY\n",id);
            sendToGroup(EMPTY, Z, 0);
            return 2;
        }
        return 1;
    }
    return 0;
}

void runningY(){
    int *queue= malloc(countOfY * sizeof(int));
    int *xtab = malloc(countOfX * sizeof(int));
    int *inQue= malloc(countOfY * sizeof(int)); 
    memset(inQue, 1, countOfY);
    memset(xtab,-1, countOfX);
    memset(queue, 0, countOfY);
    int receivedFULLs, sendedEMPTY = 0, receivedACKs;
    int k, sendedToX, offset = 0;

    struct Message message;
    int resY = 0;
    int ys = countOfX;
start:
    changeState(queueing);
    incrementTimestamp(0);
    sendToGroup(REQ, Y, 0);
    receivedACKs = 0;
    queue[id-ys]=timestamp;
    groupedProcess_id = -1;
    k=0, resY=0;
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        switch (message.type)
        {
        case REQ:
            queue[message.sender - ys] = message.timestamp;
            inQue[message.sender - ys] = 1;
            sendMessage(message.sender, ACK, 0);
            break;
        case ACK:
            if(message.timestamp > queue[id - ys])
                receivedACKs++;
            if(receivedACKs == countOfY-1){   
                changeState(waitingForX);
                printf("[Y - %d] waitingForX\n",id);
                if(k==0)
                    k = queuePlace(Y, queue, inQue, offset);
                resY = farmingY(k, queue, inQue, xtab, offset); 
                if(resY == 1){
                    offset += countOfY;
                    goto start;
                }else if(resY == 2){
                    offset += countOfY;
                    sendedEMPTY = 1;
                    goto start;
                }
            }
            break;
        case GROUP_ME:
            xtab[message.sender] = message.inQue;
            if(state == waitingForX)
                resY = farmingY(k, queue, inQue, xtab, offset); 
            if(resY == 1){
                offset += countOfY;
                goto start;
            }else if(resY == 2){
                offset += countOfY;
                sendedEMPTY = 1;
                goto start;
            }
            break;
        case RELEASE_Y:
            inQue[message.sender - ys] = 0;
            hyperSpace -= 1;
            if(hyperSpace==0 && sendedEMPTY == 0){
                sendedEMPTY=1;
                printf("[Y - %d] EMPTY\n",id);
                incrementTimestamp(0);
                sendToGroup(EMPTY, Z, 0);
            }
            resY = farmingY(k, queue, inQue, xtab, offset); 
            if(resY == 1){
                goto start;
            }else if(resY == 2){
                sendedEMPTY = 1;
                goto start;
            }
            break;
        case FULL:
            receivedFULLs++;
            if(receivedFULLs==countOfZ){
                hyperSpace = MAX_ENERGY;
                sendedEMPTY = 0; 
                receivedFULLs = 0;
                resY = farmingY(k, queue, inQue, xtab, offset); 
                if(resY == 1){
                    offset += countOfY;
                    goto start;
                }else if(resY == 2){
                    offset += countOfY;
                    sendedEMPTY = 1;
                    goto start;
                }
            }
            break;
        
        default:
            break;
        }
    }
}
void runningZ(){
    int *queue= malloc(countOfZ * sizeof(int));
    int *inQue= malloc(countOfZ * sizeof(int)); 
    memset(inQue, 1, countOfZ);
    memset(queue, 0, countOfZ);
    struct Message message;
    state = chilling;
    int k=0, receivedACKs = 0, offset = 0;
    int zs = countOfY+countOfX;
    int receivedEMPTYs = 0, sendedFULL = 0;
    goto secondStart;
    //początek, proces rozsyła żądanie do Xs aby otrzymać Y
start:
    changeState(queueing);
    incrementTimestamp(0);
    sendToGroup(REQ, Z, 0);
    receivedACKs = 0, k = 0;
    queue[id - zs] = timestamp;
secondStart:
    //pętla zarządzająca odbiorem wiadomości
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        switch (message.type)
        {
        case REQ:
            queue[message.sender - zs] = message.timestamp;
            inQue[message.sender - zs] = 1;
            sendMessage(message.sender, ACK, 0);
            break;
        case ACK:
            if(message.timestamp > queue[id - zs])
                receivedACKs++;
            if(receivedACKs == countOfZ - 1){
                k = queuePlace(Z, queue, inQue, offset);
                changeState(readyToFarm);
            }
            if(k>0 && (k - offset) + hyperSpace <= MAX_ENERGY){
               changeState(farming);
               hyperSpace++;
               sleep(TIME_IN);
               incrementTimestamp(0);
               sendToGroup(RELEASE_Z, Z, 0); 
               if(hyperSpace + k == MAX_ENERGY - 1){
                    sendToGroup(FULL, Y, 0);
                    changeState(chilling);
                    sendedFULL = 1;
                    offset += countOfZ;
                    goto secondStart;
                }
               goto start;
            }
            break;
        case RELEASE_Z:
            hyperSpace++;
            inQue[message.sender - zs] = 0;
            if(hyperSpace == MAX_ENERGY && sendedFULL == 0){
                sendToGroup(FULL, Y, 0);
                sendedFULL = 1;
                changeState(chilling);
                offset += countOfZ;
            }
            break;
        case EMPTY:
            receivedEMPTYs++;
            if(receivedEMPTYs==countOfY){
                sendedFULL = 0;
                receivedEMPTYs = 0;
                hyperSpace = 0;
                goto start;
            }
            break;
        
        default:
            break;
        }
    }
}
int main(int argc, char **argv){
    MPI_Init(&argc, &argv);
    initCustomMessage();
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    init();
    if(master == X)
        runningX();
    if(master == Y)
        runningY();
    if(master == Z)
        runningZ();

    MPI_Finalize();
    return 0;
}