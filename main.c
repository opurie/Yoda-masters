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

/*
queueing - kolejkowanie się, czekanie na wszystkie ACKi
waitingForY - odebraliśmy wszystkie ACKi i czekamy na kolej wysłania prośby do Y
readyToFarm - wysłaliśmy prośbę do Y
farming - Yi odesłał nam wiadomość i jesteśmy z nim w parze
*/
void runningX(){
    int *queue= malloc(countOfX * sizeof(int));
    for(int i = 0; i< countOfX; i++)
        queue[i]=0;
    struct Message message;
    int k, testk;
    //początek, proces rozsyła żądanie do Xs aby otrzymać Y
start:
    changeState(queueing);
    incrementTimestamp(0);
    sendToGroup(REQ, X, 0);
    queue[id]=timestamp;

    groupedProcess_id = -1;
    int receivedACKs = 0;
    k=-1; countReqs++; testk=0;
    //pętla zarządzająca odbiorem wiadomości
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        switch (message.type)
        {
        case REQ:
            countReqs++;
            queue[message.sender] = message.timestamp;
            sendMessage(message.sender, ACK, 0);
            if(message.timestamp > queue[id] || (message.timestamp == queue[id] && id< message.sender))
                testk++;
            break;
        case ACK:
            if(message.timestamp > queue[id])
                receivedACKs++;
            //otrzymano wszystkie ACKi
            if(receivedACKs==countOfX-1){
                k = countReqs - testk;
                incrementTimestamp(0);
                printf("[X - %d] WAITINGFORY - k: %d\n",id, k);
                sendToGroup(GROUP_ME, Y, k);
                changeState(waitingForY);
            }
            break;
        case JOINED:
            changeState(farming);
            if(groupedProcess_id > 0){
                printf("[ERROR X - %d] grouped - %d, want to group - %d======================================\n", id, groupedProcess_id, message.sender);
            }else{
                groupedProcess_id = message.sender;
                printf("[X - %d] FARMING - Y: %d\n", id, groupedProcess_id);
            }
            break;
        case RELEASE_Y:
            //printf("[X - %d] RELEASED\n", id);
            if(groupedProcess_id == message.sender)
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

int farmingY(int k, int* queue, int* xtab){
    if(state == waitingForX){
        groupedProcess_id = findX(k, xtab);
        if(groupedProcess_id != -1){
            printf("[Y - %d] READYTOFARM, k: %d, X: %d\n",id, k, groupedProcess_id);
            incrementTimestamp(0);
            changeState(readyToFarm);
            sendMessage(groupedProcess_id, JOINED, 0);
        }
    } 
    int tmp = wholeReceivedEnergy%MAX_ENERGY;
    if((k>=wholeReceivedEnergy-tmp && k <= wholeReceivedEnergy + MAX_ENERGY - tmp))
        if(state == readyToFarm && hyperSpace > 0){
            printf("[Y - %d] FARMING, hyperspace - %d, k:%d\n", id, hyperSpace,k);
            changeState(farming);
            hyperSpace--;
            wholeReceivedEnergy++;
            sleep(TIME_IN);
            incrementTimestamp(0);
            sendMessage(groupedProcess_id, RELEASE_Y, 0);
            sendToGroup(RELEASE_Y, Y, groupedProcess_id);
            if(hyperSpace==0){
                incrementTimestamp(0);
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
    for(int i = 0; i< countOfX; i++)
        xtab[i]=0;
    for(int i = 0; i< countOfY; i++)
        queue[i]=0;
    struct Message message;

    int receivedFULLs = 0, sendedEMPTY = 0;
    int k, sendedToX, testk;
    int resY = 0;
    int ys = countOfX;
    
start:
    changeState(queueing);
    incrementTimestamp(0);
    sendToGroup(REQ, Y, 0);
    queue[id-ys]=timestamp;

    groupedProcess_id = -1, resY=0;
    int receivedACKs = 0;
    k=-1, countReqs++; testk=0;
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        switch (message.type)
        {
        case REQ:
            countReqs++;
            queue[message.sender - ys] = message.timestamp;
            sendMessage(message.sender, ACK, 0);
            if(message.timestamp > queue[id - ys])
                testk++;
            if((message.timestamp == queue[id - ys] && id < message.sender))
                testk++;
            break;
        case ACK:
            if(message.timestamp > queue[id - ys])
                receivedACKs++;
            if(receivedACKs == countOfY-1){
                k = countReqs - testk;
                printf("[Y - %d] WAITINGFORX - k: %d\n",id, k);
                changeState(waitingForX);
                resY = farmingY(k, queue, xtab); 
                if(resY == 1){
                    goto start;
                }else if(resY == 2){
                    sendedEMPTY = 1;
                    goto start;
                }            
            }
            break;
        case GROUP_ME:
            xtab[message.sender] = message.inQue;
            resY = farmingY(k, queue, xtab); 
            if(resY == 1){
                goto start;
            }else if(resY == 2){
                sendedEMPTY = 1;
                goto start;
            }            
            break;
        case RELEASE_Y:
            hyperSpace -= 1;
            wholeReceivedEnergy++;
            if(hyperSpace==0 && sendedEMPTY == 0){
                sendedEMPTY=1;
                incrementTimestamp(0);
                sendToGroup(EMPTY, Z, 0);
            }
            break;
        case FULL:
            receivedFULLs++;
            if(receivedFULLs==countOfZ){
                hyperSpace = MAX_ENERGY;
                sendedEMPTY = 0; 
                receivedFULLs = 0;
                resY = farmingY(k, queue, xtab); 
                if(resY == 1){
                    goto start;
                }else if(resY == 2){
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
int farmingZ(){
    changeState(farming);
    hyperSpace++;
    wholeReceivedEnergy++;
    printf("\t\t\t\t\t[Z - %d] FARMING, hyperspace: %d\n",id,hyperSpace);
    sleep(TIME_IN);
    incrementTimestamp(0);
    sendToGroup(RELEASE_Z, Z, 0); 
    if(hyperSpace == MAX_ENERGY){
        sendToGroup(FULL, Y, 0);
        return 1;
    }
    return 0;
}
void runningZ(){
    int *queue= malloc(countOfZ * sizeof(int));
    for(int i = 0; i< countOfZ; i++)
        queue[i]=0;
    struct Message message;
    
    int k=0, receivedACKs = 0, testk = 0;
    int zs = countOfY+countOfX;
    int receivedEMPTYs = 0, sendedFULL = 0, tmp;
    countReqs = 0;
    //początek, proces rozsyła żądanie do Xs aby otrzymać Y
start:
    printf("\t\t\t\t\t[Z - %d] QUEUEING\n", id);
    changeState(queueing);
    incrementTimestamp(0);
    sendToGroup(REQ, Z, 0);
    queue[id - zs] = timestamp;
    receivedACKs = 0, k = -1, testk = 0;
    countReqs++;
    //pętla zarządzająca odbiorem wiadomości
    while(1){
        message = receiveMessage();
        incrementTimestamp(message.timestamp);
        switch (message.type)
        {
        case REQ:
            countReqs++;
            queue[message.sender - zs] = message.timestamp;
            sendMessage(message.sender, ACK, 0);
            if(message.timestamp > queue[id-zs]||(message.timestamp == queue[id-zs] && id< message.sender))
                testk++;    
            break;
        case ACK:
            if(message.timestamp > queue[id - zs])
                receivedACKs++;
            if(receivedACKs == countOfZ - 1){
                k = countReqs - testk;
                changeState(readyToFarm);
                printf("\t\t\t\t\t[Z - %d] READYTOFARM, k: %d\n", id, k);
            }
            tmp = wholeReceivedEnergy%MAX_ENERGY;
            if((k > wholeReceivedEnergy - tmp && k <= wholeReceivedEnergy + MAX_ENERGY - tmp)||k==MAX_ENERGY)
                if(hyperSpace < MAX_ENERGY){
                    sendedFULL = farmingZ();
                    goto start;
                }
            break;
        case RELEASE_Z:
            hyperSpace++;
            wholeReceivedEnergy++;
            if(hyperSpace == MAX_ENERGY && sendedFULL == 0){
                sendToGroup(FULL, Y, 0);
                changeState(chilling);
                sendedFULL = 1;
            }
            tmp = wholeReceivedEnergy%MAX_ENERGY;
            if((k > wholeReceivedEnergy - tmp && k <= wholeReceivedEnergy + MAX_ENERGY - tmp)||k==MAX_ENERGY)
                if(hyperSpace < MAX_ENERGY){
                    sendedFULL = farmingZ();
                    goto start;
                }
            break;
        case EMPTY:
            receivedEMPTYs++;
            if(receivedEMPTYs==countOfY){
                sendedFULL = 0;
                receivedEMPTYs = 0;
                hyperSpace = 0;
                if(wholeReceivedEnergy==0)
                    goto start;
                tmp = wholeReceivedEnergy%MAX_ENERGY;
                if((k > wholeReceivedEnergy - tmp && k <= wholeReceivedEnergy + MAX_ENERGY - tmp))
                    if(hyperSpace < MAX_ENERGY){
                        sendedFULL = farmingZ();
                        goto start;
                    }
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