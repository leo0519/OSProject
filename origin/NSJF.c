#define _GNU_SOURCE
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sched.h>
#include<sys/wait.h>
#include<time.h>

typedef struct{
    char name[32];
    int ready;
    int term;
} Process;

//attach process to core 0
void to_specific_core(){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
}

//user-defined unit time
void unit_time(){
    volatile unsigned long i;
    for(i = 0; i < 1000000UL; i++);
}

struct timespec start;
struct timespec end;

int create_child(Process procs[], int N){
    int unit_time_clock = 0;
    for(int i = 0; i < N; i++){
        for(int j = unit_time_clock; j < procs[i].ready; j++){
            unit_time();
        }
        unit_time_clock = procs[i].ready;
        int cur_idx = i;
        for(; i < N && procs[i].ready == unit_time_clock; i++){
            pid_t pid;
            pid = fork();
            if(pid == 0){
                clock_gettime(CLOCK_REALTIME, &start);
                return i;
            }
        }
        i--;
    }
    return -1;
}

int main(){
    char sche[5];
    scanf("%s", sche);
    int N;
    scanf("%d", &N);
    Process procs[N];
    for(int i = 0; i < N; i++){
        scanf("%s%d%d", procs[i].name, &procs[i].ready, &procs[i].term);
    }
    Process temp;
    for(int i = 0;i<N-1;i++){
      for(int j = i+1;j<N;j++){
          if(procs[i].ready>procs[j].ready)
          {
            temp=procs[i];
            procs[i]=procs[j];
            procs[j]=temp;
          }
      }
    }
    int is_child = create_child(procs, N);
    if(is_child >= 0){
        struct sched_param param;
        param.sched_priority = 50 - is_child;
        sched_setscheduler(getpid(), SCHED_FIFO, &param);
        to_specific_core();
        for(int i = 0; i < procs[is_child].term; i++){
            unit_time();
        }
        struct timespec end;
        clock_gettime(CLOCK_REALTIME, &end);
        printf("%s %ld.%ld %ld.%ld\n", procs[is_child].name, start.tv_sec, start.tv_nsec, end.tv_sec, end.tv_nsec);
    }
    else{
      int status;
      wait(&status);
    }
}
