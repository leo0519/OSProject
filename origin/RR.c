
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

int run_process(int priority, Process p, int N){
	struct sched_param param;
    param.sched_priority = priority;
    sched_setscheduler(getpid(), SCHED_FIFO, &param);
    to_specific_core();
	
	if(p.term >= 500){
		for(int i = 0; i < 500; i++)
			unit_time();
		p.term -= 500;
		run_process(priority - N, p, N);
		return 0;
	}
    for(int i = 0; i < p.term; i++)
        unit_time();
    struct timespec end;
    clock_gettime(CLOCK_REALTIME, &end);
    printf("%s %ld.%ld %ld.%ld\n", p.name, start.tv_sec, start.tv_nsec, end.tv_sec, end.tv_nsec);
}

int main(){
    char sche[5];
    scanf("%s", sche);
    int N;
    scanf("%d", &N);
    Process procs[N];
    for(int i = 0; i < N; i++)
        scanf("%s%d%d", procs[i].name, &procs[i].ready, &procs[i].term);
    
	
    int is_child = create_child(procs, N);
	
	if(is_child < 0) return 0;
	
	Process p = procs[is_child];
	run_process(99 - is_child, p, N);
	
	return 0;	
}

