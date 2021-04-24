#include <bits/stdc++.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <ctype.h>

using namespace std;

#define MAXSIZE 100

void sem_lock(int sem_set_id)
{
    struct sembuf sem_op;

    /* wait on the semaphore, unless it's value is non-negative. */
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(sem_set_id, &sem_op, 1);
}

void sem_unlock(int sem_set_id)
{
    struct sembuf sem_op;

    /* signal the semaphore - increase its value by one. */
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;   /* <-- Comment 3 */
    sem_op.sem_flg = 0;
    semop(sem_set_id, &sem_op, 1);
}

typedef struct _job {
    pid_t pid;
    int producerNum;
    int priority;
    float computeTime;
    int jobId;
} job;

typedef struct _priorityQueue {
    job heap[MAXSIZE];
    int job_created=0;
    int job_completed=0;
    int size=0;
    int ct=0;
} priorityQueue;

bool isEmpty(priorityQueue* pq){
    return pq->size<=0;
}

bool isFull(priorityQueue* pq){
    return pq->size>=MAXSIZE;
}

void printPQ(priorityQueue* pq){
    for(int i=0;i<pq->size;i++){
        cout<<pq->heap[i].priority<<','<<pq->heap[i].jobId<<' ';
    }
    cout<<'\n';
}

void insert(priorityQueue* pq,job newJob){
    if(!isFull(pq)){
        pq->heap[pq->size]=newJob;
        int i=pq->size;
        while(i>0){
            if(pq->heap[(i-1)/2].priority < pq->heap[i].priority)
                swap(pq->heap[(i-1)/2],pq->heap[i]);
            else
                break;
            i=(i-1)/2;
        }
        pq->size++;
    }
}

job extractMax(priorityQueue* pq){
    job topJob;
    if(!isEmpty(pq)){
        topJob=pq->heap[0];
        int sz=pq->size-1;
        swap(pq->heap[0],pq->heap[sz]);
        int i=0;
        while((2*i+1)<sz){
            int largest=i;
            if((2*i+1)<sz && pq->heap[largest].priority < pq->heap[2*i+1].priority)
                largest=2*i+1;
            if((2*i+2)<sz && pq->heap[largest].priority < pq->heap[2*i+2].priority)
                largest=2*i+2;
            if(largest==i)
                break;
            swap(pq->heap[largest],pq->heap[i]);
            i=largest;
        }
        pq->size--;
    }
    return topJob;
}
     

int main(){
    int NP, NC, NJ;
    cout<<"Enter the number of producers\n";
    cin>>NP;
    cout<<"Enter the number of consumers\n";
    cin>>NC;
    cout<<"Enter the number of jobs\n";
    cin>>NJ;
 
    //shared memory permission; can be read and written by anybody
    const int perm = 0666;

    int semId = semget(IPC_PRIVATE,1,IPC_CREAT|perm);
    if(semId==-1){
        cout<<"Semget failed\n";
        exit(EXIT_FAILURE);
    }
    union semun { 
            int val;
            struct semid_ds *buf;
            ushort * array;
    } sem_val;

    sem_val.val = 1;

    int rc=semctl(semId,0,SETVAL,sem_val);

    if(rc==-1){
        cout<<"Semctl failed\n";
        exit(EXIT_FAILURE);
    }

    //shared memory segment size
    size_t shmSize = 30000;
    //Create shared memory if not already created with specified permission
    int shmId = shmget(IPC_PRIVATE,shmSize,IPC_CREAT|perm);
    if (shmId ==-1) {
        cout<<"Shared memory creation failed\n";
        exit(EXIT_FAILURE);
    }   
    //Attach the shared memory segment
    void* shmPtr = shmat(shmId,NULL,0);
    if(shmPtr==NULL){
        cout<<"Shmat failed\n";
        exit(EXIT_FAILURE);
    }
    
    //struct commonData* dp = (struct commonData*)shmPtr;
    priorityQueue *pq = (priorityQueue *)shmPtr;

    clock_t start, end;
    double cpu_time_used;
    start = clock();

    for(int i=0;i<NP;i++){
        pid_t pid = fork();
        if(pid==0){ // producer i
            srand(i+1);
            while(pq->job_created<NJ){
                float waitTime=(rand()%3001)/1000.0;
                sleep(waitTime); //sleeping for 0 to 3 seconds
                job newJob;
                newJob.pid=getpid();
                newJob.producerNum=i+1;
                newJob.computeTime=(rand()%3001)/1000.0+1;
                newJob.jobId=rand()%100000+1;
                newJob.priority=(rand()%10)+1;

                while(1){
                    sem_lock(semId);
                    if(pq->job_created>=NJ){
                        sem_unlock(semId);
                        break;
                    }
                    if(isFull(pq)){
                        sem_unlock(semId);
                        continue;
                    }
                    insert(pq,newJob);
                    cout<<"producer: "<<(i+1)<<", producer pid: "<<newJob.pid<<", priority: "<<newJob.priority<<", compute time: "<<newJob.computeTime<<", jobId: "<<newJob.jobId<<'\n';
                    pq->job_created++;
                    //printPQ(pq);
                    sem_unlock(semId);
                    break;
                }
            }
            sem_lock(semId);
            pq->ct++;
            sem_unlock(semId);
            exit(0);
        }
    }

    for(int i=0;i<NC;i++){
        pid_t pid = fork();
        if(pid==0){ // consumer i
            srand(NP+i+1);
            while(pq->job_completed<NJ){
                float waitTime=(rand()%3001)/1000.0;
                sleep(waitTime); //sleeping for 0 to 3 seconds
                while(1){
                    sem_lock(semId);
                    if(pq->job_completed>=NJ){
                        sem_unlock(semId);
                        break;
                    }
                    if(isEmpty(pq)){
                        sem_unlock(semId);
                        continue;
                    }
                    job topJob = extractMax(pq);
                    cout<<"consumer: "<<(i+1)<<", consumer pid: "<<getpid()<<", producer: "<<topJob.producerNum<<", producer pid: "<<topJob.pid<<", priority: "<<topJob.priority<<", compute time: "<<topJob.computeTime<<", jobId: "<<topJob.jobId<<'\n';
                    pq->job_completed++;
                    //printPQ(pq);
                    sem_unlock(semId);
                    sleep(topJob.computeTime);
                }
            }
            sem_lock(semId);
            pq->ct++;
            sem_unlock(semId);
            exit(0);
        }
    }

    while(pq->ct < (NC+NP));

    end = clock();

    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

    cout<<"Time taken to run "<<NJ<<" jobs: "<<cpu_time_used<<'\n';

    //detach shared memory
    shmdt(shmPtr);

    shmctl(shmId,IPC_RMID,NULL); //mark to destroy
}