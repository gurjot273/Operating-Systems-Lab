#include <bits/stdc++.h>
#include <pthread.h>
#include <signal.h>
#include <ctime>
#include <unistd.h> 

#define NUM_THREADS 10
#define M 3000
#define delta 10

// status constants 
#define TERMINATED 0
#define RUNNING 1
#define SUSPENDED 2
#define CREATED 3

using namespace std;

typedef struct status {
    bool isProducer;
    int runningStatus;
} STATUS;

STATUS status[NUM_THREADS];

pthread_mutex_t mut;

// shared buffer
queue <int> buffer;

// shows current thread in execution
int reporterCurrentThread = -1;

// signal handler for user signals
void signalHandler(int id) {
    if(id == SIGUSR1) {
        pause();
    }
}

// handler for worker threads
void *workerHandler(void *param) {
    int id = *((int *)param);
    pause(); // pausing then resuming from scheduler thread

    if(status[id].isProducer) { // if thread is a producer
        int numLeft = 1000;
        while(1) {
            pthread_mutex_lock(&mut); // mutex lock
            if(buffer.size() < M) {
                int num = rand() % 1000 + 1; // generate random number
                buffer.push(num); // insert in buffer
                numLeft--; // decreasing numleft
            }
            if(numLeft<=0){
                status[id].runningStatus = TERMINATED; // updating status
                pthread_mutex_unlock(&mut); // unlock mutex
                pthread_exit(NULL); // exiting thread
            }
            pthread_mutex_unlock(&mut); // mutex unlock
        }
        
    } else {
        while(1) {
            pthread_mutex_lock(&mut); // mutex lock
            if(buffer.size() > 0) { 
                buffer.pop();           // remove element 
            }
            bool flag=false;
            for(int i=0;i<NUM_THREADS;i++){ // checking if any producer is left to generate number
                if(status[i].isProducer && status[i].runningStatus!=TERMINATED) {
                    flag=true;
                    break;
                }
            }
            if(buffer.size()>0) // check buffer size
                flag=true;
            if(!flag){ 
                status[id].runningStatus = TERMINATED; // updating status
                pthread_mutex_unlock(&mut); // unlock mutex
                pthread_exit(NULL); // exiting thread
            }
            pthread_mutex_unlock(&mut); // mutex unlock
        }
    }
}

// handler for scheduler thread
void *schedulerHandler(void *param) {
    queue <int> scheduleQ; // ready queue
    pthread_t *threads = (pthread_t *)param; // threads
    for(int i=0;i<NUM_THREADS;i++) {
        scheduleQ.push(i); // pushing all threads in the ready queue
    }
    while(1){
        if(scheduleQ.empty()) {
            pthread_exit(NULL); // if all threads are finished exit
        }

        int currentThread = scheduleQ.front();
        scheduleQ.pop(); // poping from the front of queue

        pthread_mutex_lock(&mut); 

        if(status[currentThread].runningStatus == TERMINATED) { // check if currentThread is terminated
            pthread_mutex_unlock(&mut);
            continue;
        }
        
        // updating current thread status
        status[currentThread].runningStatus = RUNNING;
        reporterCurrentThread=currentThread;
        pthread_kill(threads[currentThread],SIGUSR2);
        pthread_mutex_unlock(&mut);

        usleep(delta); // waiting for current thread

        pthread_mutex_lock(&mut);

        if(status[currentThread].runningStatus != TERMINATED) {
            // updating current thread status
            status[currentThread].runningStatus = SUSPENDED;
            pthread_kill(threads[currentThread],SIGUSR1);
            scheduleQ.push(currentThread);
        }
        pthread_mutex_unlock(&mut);
    }
} 

// handler for reporter thread
void *reporterHandler(void *param) {
    int previousThread = -1; // store previous thread

    while(1) {
        bool flag=false;
        // checking if all threads are terminated
        for(int i=0;i<NUM_THREADS;i++){
            if((status[i].runningStatus!=TERMINATED) || (reporterCurrentThread != previousThread)) {
                flag=true;
                break;
            }
        }

        if(!flag) {
            cout<<"Thread "<<reporterCurrentThread<<" has terminated, ";
            cout<<"Buffer Size = "<<buffer.size()<<"\n";
            pthread_exit(0);
        }

        if(reporterCurrentThread == previousThread){ // no change of thread
            continue;
        }

        if(previousThread == -1) { // starting condition
            cout<<"Thread "<<reporterCurrentThread<<" has started executing, ";
        } else {
            if(status[previousThread].runningStatus == TERMINATED) { // thread termination
                cout<<"Thread "<<previousThread<<" has terminated\n";
            }
            // context switch
            cout<<"Execution switched from "<<previousThread<<" to "<<reporterCurrentThread<<", ";
        }
        cout<<"Buffer Size = "<<buffer.size()<<"\n";
        previousThread=reporterCurrentThread; // updating previous thread
        //pthread_mutex_unlock(&mut);
    }
}

int main() {
    pthread_t threads[NUM_THREADS];
    srand(time(NULL));

    signal(SIGUSR1,signalHandler); // creating signal handler for SIGUSR1
    signal(SIGUSR2,signalHandler); // creating signal handler for SIGUSR2

    pthread_mutex_init(&mut,NULL); // initializing mutex

    for(int i=0;i<NUM_THREADS;i++) {
        int p = rand()%2;
        int *id = new int;
        *id=i;
        if(p) { // producer
            status[i].isProducer = true;
            status[i].runningStatus = CREATED;
            cout<<"Producer thread "<<i<<" is created\n";
        } else { // consumer
            status[i].isProducer = false;
            status[i].runningStatus = CREATED;
            cout<<"Consumer thread "<<i<<" is created\n";
        }
        status[i].runningStatus = SUSPENDED;
        pthread_create(&threads[i],NULL,workerHandler,(void *)id); // creating worker thread
    }

    pthread_t scheduler, reporter;
    // creating scheduler thread
    pthread_create(&scheduler,NULL,schedulerHandler,threads);
    // creating reporter thread
    pthread_create(&reporter,NULL,reporterHandler,NULL);

    // waiting for worker threads to finish
    for(int i=0;i<NUM_THREADS;i++) {
        pthread_join(threads[i],NULL);
    }

    // waiting for scheduler threads to finish
    pthread_join(scheduler,NULL);
    cout<<"Scheduler thread has terminated\n";

    // waiting for reporter threads to finish
    pthread_join(reporter,NULL);
    cout<<"Reporter thread has terminated\n";
    
    cout<<"Program terminated\n";

    pthread_mutex_destroy(&mut); // destroying mutex
    return 0;
}