#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>
#include<unistd.h>
#include<sched.h>
#include<time.h>
#include<sys/types.h>
#include<signal.h>

typedef struct{
    char name[32];
    int ready;
    int term;
    int pid;
    struct timespec start, end;
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

void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void print_log(char *msg, int size){
    char cmd[size + 3];
    cmd[0] = cmd[size + 1] = '"';
    cmd[size + 2] = '\0';
    memcpy(cmd + 1, msg, size);
    char sys[size + 64];
    sprintf(sys, "insmod kernel_files/logger.ko message='%s'", cmd);
    system(sys);
    system("rmmod logger");
}

int procs_cmp(const void *p1, const void *p2){
    return (*(Process **)p1)->ready > (*(Process **)p2)->ready;
}

int FIFO(Process *procs[], int N){
    int ts = 0;
    int wait = 0;
    int exec = 0;
    while(exec < N){
        for(;wait < N && procs[wait]->ready == ts; wait++){
            int pid = fork();
            clock_gettime(CLOCK_REALTIME, &procs[wait]->start);
            if(pid == 0){
                printf("%s %d\n", procs[wait]->name, getpid());
                return 0;
            }
            else procs[wait]->pid = pid;
        }
        unit_time();
        ts++;
        if(wait > exec){
            if(procs[exec]->term == 0){
                clock_gettime(CLOCK_REALTIME, &procs[exec]->end);
                char buf[128];
                sprintf(buf, "[Project1] %d %ld.%09ld %ld.%09ld", procs[exec]->pid, procs[exec]->start.tv_sec, procs[exec]->start.tv_nsec, procs[exec]->end.tv_sec, procs[exec]->end.tv_nsec);
                print_log(buf, strlen(buf));
                exec++;
            }
            if(wait > exec)procs[exec]->term--;
        }
    }
    return 1;
}

int RR(Process *procs[], int N){
    int status[N];
    memset(status, 0, N*sizeof(int)); //1=running, 2=done
    
    int exec = 0;
    int ts = 0;
    int pre_running_id = -1;
    int running_id = -1;
    int pre_ts = -1;
    while(exec < N){
        for(int i=0; i<N; i++){ //fork
            if(status[i]==0 && procs[i]->ready==ts){ 
                int pid = fork();
                clock_gettime(CLOCK_REALTIME, &procs[i]->start);
                if(pid == 0){
                    printf("%s %d\n", procs[i]->name, getpid());
                    struct sched_param param = {.sched_priority = 1};
                    sched_setscheduler(0, SCHED_FIFO, &param);
                    return 0;
                }
                else{ 
                    procs[i]->pid = pid;
                    status[i] = 1;
                }
            }               
        }
        
        unit_time();
        ts++;
        
        if(running_id == -1){ //no running process
            for(int i=0; i<N; i++){
                int check_id = (pre_running_id==-1) ? i : (pre_running_id+1+i)%N;
                if(status[check_id] == 1 ){ //i is ready
                    running_id = check_id;
                    break;
                }
            }
        }
        if(running_id != -1){
            if(pre_ts == -1) pre_ts = ts;
            procs[running_id]->term--;
            if(procs[running_id]->term == 0){
                struct sched_param param = {.sched_priority = 99};
                sched_setscheduler(procs[running_id]->pid, SCHED_FIFO, &param);
                clock_gettime(CLOCK_REALTIME, &procs[running_id]->end);
                char buf[128];
                sprintf(buf, "[Project1] %d %ld.%09ld %ld.%09ld", procs[running_id]->pid, procs[running_id]->start.tv_sec, procs[running_id]->start.tv_nsec, procs[running_id]->end.tv_sec, procs[running_id]->end.tv_nsec);
                print_log(buf, strlen(buf));
                exec++;
                status[running_id] = 2;
                pre_ts = -1;
                pre_running_id = running_id;
                running_id = -1;
            }
                
            if((ts-pre_ts) == 500){
                pre_ts = -1;
                pre_running_id = running_id;
                
                int special_check = 0;
                for(int i=0; i<N; i++){
                    if(i!=running_id && status[i]!=2){
                        special_check = 1;
                        break;
                    }
                }
                if(special_check)
                    running_id = -1;
            }
        }
    }
}

int minOfReady(Process *procs[], int ready[], int k) {
    int ind = -1, mini = INT_MAX;
    for(int i = 0; i < k; i++) {
        if(procs[ready[i]]->term < mini) {
            ind = i;
            mini = procs[ready[i]]->term;
        }
    }
    return ind;
}

int SJF(Process *procs[], int N) {
    int ready[N], k = 0;
    int ts = 0;
    int wait = 0;
    int exec = 0;
    int now = -1;
    while(exec < N) {
        for(; wait < N && procs[wait]->ready == ts; wait++) {
            ready[k] = wait;
            k++;
            int pid = fork();
            clock_gettime(CLOCK_REALTIME, &procs[wait]->start);
            if(pid == 0) {
                printf("%s %d\n", procs[wait]->name, getpid());
                struct sched_param param = {.sched_priority = 1};
                sched_setscheduler(0, SCHED_FIFO, &param);
                return 0;
            }
            else procs[wait]->pid = pid;
        }
        if(now == -1) now = minOfReady(procs, ready, k);
        //check for the execution time = 0 process
        while(now != -1 && procs[ready[now]]->term == 0) {
            struct sched_param param = {.sched_priority = 99};
            sched_setscheduler(procs[ready[now]]->pid, SCHED_FIFO, &param);
            clock_gettime(CLOCK_REALTIME, &procs[ready[now]]->end);
            char buf[128];
            sprintf(buf, "[Project1] %d %ld.%09ld %ld.%09ld", procs[ready[now]]->pid, procs[ready[now]]->start.tv_sec, procs[ready[now]]->start.tv_nsec, procs[ready[now]]->end.tv_sec, procs[ready[now]]->end.tv_nsec);
            print_log(buf, strlen(buf));
            swap(&ready[now], &ready[k-1]);
            k--;
            now = minOfReady(procs, ready, k);
            exec++;
        }
        unit_time();
        ts++;
        if(now != -1) {
            procs[ready[now]]->term--;
            if(procs[ready[now]]->term == 0) {
                struct sched_param param = {.sched_priority = 99};
                sched_setscheduler(procs[ready[now]]->pid, SCHED_FIFO, &param);
                clock_gettime(CLOCK_REALTIME, &procs[ready[now]]->end);
                char buf[128];
                sprintf(buf, "[Project1] %d %ld.%09ld %ld.%09ld", procs[ready[now]]->pid, procs[ready[now]]->start.tv_sec, procs[ready[now]]->start.tv_nsec, procs[ready[now]]->end.tv_sec, procs[ready[now]]->end.tv_nsec);
                print_log(buf, strlen(buf));
                swap(&ready[now], &ready[k-1]);
                k--;
                now = -1;
                exec++;
            }
        }
    }
    return 1;
}

int PSJF(Process *procs[], int N){
    int ts = 0;
    int wait = 0;
    int finish = 0;
    while(finish < N){
        for(; wait < N && procs[wait]->ready == ts; wait++){
            int pid = fork();
            clock_gettime(CLOCK_REALTIME, &procs[wait]->start);
            if(pid == 0){
                printf("%s %d\n", procs[wait]->name, getpid());
                return 0;
            }
            else procs[wait]->pid = pid;
        }
        int short_idx;
        for(int i = 0, short_len = INT_MAX; i < wait; i++){
            if(procs[i]->term != 0 && procs[i]->term < short_len) short_idx = i, short_len = procs[i]->term;
        }
        unit_time();
        ts++;
        if(wait > 0) {
            procs[short_idx]->term--;
            if(procs[short_idx]->term == 0){
                char buf[128];
                clock_gettime(CLOCK_REALTIME, &procs[short_idx]->end);
                sprintf(buf, "[Project1] %d %ld.%09ld %ld.%09ld", procs[short_idx]->pid, procs[short_idx]->start.tv_sec, procs[short_idx]->start.tv_nsec, procs[short_idx]->end.tv_sec, procs[short_idx]->end.tv_nsec);
                print_log(buf, strlen(buf));
                finish++;
            }
        }
    }
    return 1;
}

int main(){
    char sche[5];
    scanf("%s", sche);
    int N;
    scanf("%d", &N);
    Process *procs[N];
    for(int i = 0; i < N; i++){
        procs[i] = (Process *)malloc(sizeof(Process));
        scanf("%s%d%d", procs[i]->name, &procs[i]->ready, &procs[i]->term);
    }
    struct sched_param param = {.sched_priority = 99};
    sched_setscheduler(0, SCHED_FIFO, &param);
    qsort(procs, N, sizeof(Process *), procs_cmp);
    to_specific_core();
    int is_parent;
    if(!strcmp(sche, "FIFO")) is_parent = FIFO(procs, N);
    else if(!strcmp(sche, "RR")) is_parent = RR(procs, N);
    else if(!strcmp(sche, "SJF")) is_parent = SJF(procs, N);
    else if(!strcmp(sche, "PSJF")) is_parent = PSJF(procs, N);
    if(is_parent)print_log("------------------------------", 30);
    return 0;
}
