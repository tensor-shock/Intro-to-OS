#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
// #include <semaphore.h>
#include "zemaphore.h"


pthread_mutex_t rng_mutex;


int thread_safe_rng(int min, int max) {
    pthread_mutex_lock(&rng_mutex);
    int r = rand();
    pthread_mutex_unlock(&rng_mutex);
    return min + r % max;
}

/* TODO : can add global vars, structs, functions etc */
char* getDir(char *ch)
{
    if(ch[0]=='N')
    return "North\0";
    else if(ch[0]=='W')
    return "West\0";
    else if(ch[0]=='E')
    return "East\0";
    return "South\0";
}

sem_t inter1,inter2,inter3,inter4,interdead1;
int a1=0,a2=0,a3=0,a4=0;
int done=0;
int deadlock_handle=0;
int deadlock_bypassE=0,deadlock_bypassN=0,deadlock_bypassW=0,deadlock_bypassS=0;

void arriveLane(char *arg) {
    /* TODO: add code here */
    if(arg[0]=='N')
    {
        sem_wait(&inter1);
        a1=1;
    }
    else if(arg[0]=='E')
    {
        sem_wait(&inter4);
        a4=1;
    }
    else if(arg[0]=='W')
    {
        sem_wait(&inter2);
        a2=1;
    }
    else
    {
        sem_wait(&inter3);
        a3=1;
    }
}

void crossLane(char *arg) {
    /* TODO: add code here */
    if(arg[0]=='N')
    {
        sem_wait(&inter2);

        if(deadlock_bypassN)
        {
            sem_post(&inter2);
            return;
        }
        a1=0;
    }
    else if(arg[0]=='E')
    {
        sem_wait(&inter1);
        
        if(deadlock_bypassE)
        {
            sem_post(&inter1);
            return;
        }
        a4=0;
    }
    else if(arg[0]=='W')
    {
        sem_wait(&inter3);

        if(deadlock_bypassW)
        {
            sem_post(&inter3);
            return;
        }
        a2=0;
    }
    else
    {
        sem_wait(&inter4);
        if(deadlock_bypassS)
        {
            sem_post(&inter4);
            return;
        }
        a3=0;
    }
    usleep(1000 * thread_safe_rng(500, 1000)); // take 500-1000 ms to cross the lane
}

void exitLane(char *arg) {
    /* TODO: add code here */
    if(arg[0]=='N')
    {
        sem_post(&inter1);
        sem_post(&inter2);
    }
    else if(arg[0]=='E')
    {
        
        sem_post(&inter1);
        sem_post(&inter4);
    }
    else if(arg[0]=='W')
    {
        sem_post(&inter2);
        sem_post(&inter3);
    }
    else 
    {
        sem_post(&inter3);
        sem_post(&inter4);
    }
}



void* trainThreadFunction(void* arg)
{
    /* TODO extract arguments from the `void* arg` */
    usleep(thread_safe_rng(0, 10000)); // start at random time
    
    char* trainDir = getDir((char*)(arg)); // TODO set the direction of the train: North/South/East/West.
    
    arriveLane((char*)(arg));
    printf("Train Arrived at the lane from the %s direction\n", trainDir);

    
    crossLane((char*)(arg));

    if(deadlock_bypassE>0 && ((char*)(arg))[0]=='E')
    {
        deadlock_bypassE--;
        return NULL;
    }
    if(deadlock_bypassN>0 && ((char*)(arg))[0]=='N')
    {
        deadlock_bypassN--;
        return NULL;
    }
    if(deadlock_bypassS>0 && ((char*)(arg))[0]=='S')
    {
        deadlock_bypassS--;
        return NULL;
    }
    if(deadlock_bypassW>0 && ((char*)(arg))[0]=='W')
    {
        deadlock_bypassW--;
        return NULL;
    }
    printf("Train Exited the lane from the %s direction\n", trainDir);
    
    exitLane((char*)(arg));

    return NULL;
}

void* deadLockResolverThreadFunction(void * arg) {
    /* TODO extract arguments from the `void* arg` */
    while (1) {
        /* TODO add code to detect deadlock and resolve if any */

        int deadLockDetected = 0; // TODO set to 1 if deadlock is detected

        if(a1 & a2 & a3 & a4)
        {
            deadLockDetected=1;
        }

        if (deadLockDetected) {
            printf("Deadlock detected. Resolving deadlock...\n");
            /* TODO add code to resolve deadlock */

            int ran = rand();
            ran = ran%4;

            usleep(1000 * thread_safe_rng(500, 1000));

            if(ran==0)
            {
                printf("Train Exited the lane from the East direction\n");
                a4=0;
                deadlock_bypassE++;
                sem_post(&inter4);
            }
            else if(ran==1)
            {
                printf("Train Exited the lane from the West direction\n");
                a2=0;
                deadlock_bypassW++;
                sem_post(&inter2);
            }
            else if(ran==2)
            {
                printf("Train Exited the lane from the South direction\n");
                a3=0;
                deadlock_bypassS++;
                sem_post(&inter3);
            }
            else{
                printf("Train Exited the lane from the North direction\n");
                a1=0;
                deadlock_bypassN++;
                sem_post(&inter1);
            }
            


        }
        if(done)
        return NULL;
        
        usleep(1000 * 500); // sleep for 500 ms
    }
}




int main(int argc, char *argv[]) {


    srand(time(NULL));

    if (argc != 2) {
        printf("Usage: ./main <train dirs: [NSWE]+>\n");
        return 1;
    }
    pthread_mutex_init(&rng_mutex, NULL);


    /* TODO create a thread for deadLockResolverThreadFunction */
    pthread_t deadlock;
    pthread_create(&deadlock, NULL, deadLockResolverThreadFunction, NULL);

    char* train = argv[1];
    
    int num_trains = 0;
    int inputsize=strlen(train);
    pthread_t array[inputsize];

    sem_init(&inter1,0,1);
    sem_init(&inter2,0,1);
    sem_init(&inter3,0,1);
    sem_init(&inter4,0,1);
    sem_init(&interdead1,0,1);
    

    while (train[num_trains] != '\0') {
        char trainDir = train[num_trains];

        if (trainDir != 'N' && trainDir != 'S' && trainDir != 'E' && trainDir != 'W') {
            printf("Invalid train direction: %c\n", trainDir);
            printf("Usage: ./main <train dirs: [NSEW]+>\n");
            return 1;
        }
        pthread_t temp;
        array[num_trains]=temp;
        if(trainDir=='N')
        {
            pthread_create(&array[num_trains], NULL, trainThreadFunction, "N\0");
        }
        else if(trainDir=='E')
        {
            pthread_create(&array[num_trains], NULL, trainThreadFunction, "E\0");
        }
        else if(trainDir=='W')
        {
            pthread_create(&array[num_trains], NULL, trainThreadFunction, "W\0");
        }
        else 
        {
            pthread_create(&array[num_trains], NULL, trainThreadFunction, "S\0");
        }


        /* TODO create a thread for the train using trainThreadFunction */

        num_trains++;
    }

    /* TODO: join with all train threads*/
    for(int i=0;i<inputsize;i++)
    {
        pthread_join(array[i], NULL);
    }
    done=1;
    pthread_join(deadlock, NULL);
    
    return 0;
}