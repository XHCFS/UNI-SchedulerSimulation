#include "headers.h"
#include <math.h>
#include <sys/select.h>
#include <errno.h>


typedef enum{
    FCFS,
    RR,
    SRTF,
    SJF,
    NUM_SCHEDULERS
} Scheduler;

typedef enum{
    RUNNING,
    READY,
    FINISHED
} ProcessState;


int time;
int scheduler_num;
int num_processes=0;
int working_process=-1;
int SCHEDULER_counter=0;
ssize_t bytesRead;
Process processes[100];
int IDLE_TIME=0;
int STOP_TIME=0;
const int Quanta=1;
int RR_Count=Quanta;
Scheduler scheduler;


void printProcesses();
void schedule_SRTF();
void schedule_RR();
void schedule_FCFS();
void schedule_SJF();
void (*schedule[])() = {&schedule_FCFS, &schedule_RR, &schedule_SRTF,&schedule_SJF};
void logProcessState();
void updateProcessStatistics();
void run_process();
void contextSwitch(int oldIndex, int newIndex);
void sigint_handler(int signo);
int isDataAvailable();
void setNonBlockingStdin();
void create_process(Process_Params);

int main(int argc, char * argv[])
{
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("Error setting up signal handler");
        exit(EXIT_FAILURE);
    }

    setNonBlockingStdin();
    
    scheduler = (argc > 1) ? atoi(argv[1]) : FCFS;
    initClk();
    Process_Params received_params;

    while(1){

        if(time==getClk()){continue;}
        time = getClk();
        schedule[scheduler]();
        run_process();
        printProcesses();

        if(working_process==-1){
            IDLE_TIME++;
        } 

        bytesRead = fread(&received_params,sizeof(Process_Params), 1, stdin);
        if(bytesRead==0){continue;}
        while (bytesRead > 0) {
            create_process(received_params);
            bytesRead = fread(&received_params, sizeof(Process_Params), 1, stdin);
        }
      
      
    }
}

void create_process(Process_Params received_params){
        Process received_process;
        received_process.id = received_params.id;
        received_process.arrival_time = received_params.arrival_time;
        received_process.runtime = received_params.runtime;

        received_process.pid = 0;
        received_process.waiting_time= 0;
        received_process.remainingtime= received_params.runtime;
        received_process.state = 1;
        received_process.turnaround_time = 0;
        received_process.weighted_turnaround_time = 0;

        pid_t pid = fork();
        if (pid == 0){
            char argv[2][20];
            sprintf(argv[0], "%d", received_process.id);
            sprintf(argv[1], "%d", received_process.runtime);
    
            if (execl("./process.out", "./process.out", argv[0], argv[1], (char*) NULL) == -1) 
            {perror("execl");exit(EXIT_FAILURE);}
        }

        received_process.pid=pid;
        received_process.remainingtime=received_process.runtime;
        processes[num_processes++]=received_process;   
}

void run_process() {
    if (working_process == -1) {
        return;
    }
    if (processes[working_process].remainingtime == 0) {
        processes[working_process].state = FINISHED;
        updateProcessStatistics(working_process);
        logProcessState(working_process);
        return;
    }
    int current_process = processes[working_process].pid;
    processes[working_process].remainingtime--;
    if (kill(current_process, SIGUSR2) == -1) {
        perror("Error sending signal");
        exit(EXIT_FAILURE);
    }
}
void contextSwitch(int oldIndex, int newIndex) {
    if (oldIndex != -1 && processes[oldIndex].state!=FINISHED) {
        processes[oldIndex].state = READY;
    }

    working_process = newIndex;

    processes[newIndex].state = RUNNING;

    logProcessState(oldIndex);
    logProcessState(newIndex);
}

void schedule_SRTF() {
    int min_index = -1;
    int min_value = 9999;
    for (int i = 0; i < num_processes; i++) {
        if (processes[i].state != FINISHED && processes[i].remainingtime < min_value) {
            min_index = i;
            min_value = processes[min_index].remainingtime;
        }
    }
    if (min_index != -1) {
        contextSwitch(working_process, min_index);
    } else {
        working_process = -1;
    }
}

void schedule_RR() {
    RR_Count--;
    if (RR_Count != 0) {
        return;
    }

    RR_Count = Quanta;

    if (++scheduler_num >= num_processes) {
        scheduler_num = 0;
    }
    for (int i = scheduler_num; i < num_processes; i++) {
        if (processes[i].state != FINISHED) {
            contextSwitch(working_process, i);
            scheduler_num = i;
            return;
        }
    }

    working_process = -1;
    printf("No available process to switch to.\n");
}

void schedule_SJF() {

    if (working_process == -1 || processes[working_process].state == FINISHED) {
        int min_index = -1;
        int min_value = 9999;
        for (int i = 0; i < num_processes; i++) {
            if (processes[i].state != FINISHED && processes[i].remainingtime < min_value) {
                min_index = i;
                min_value = processes[min_index].remainingtime;
            }
        }
        if (min_index != -1) {
            contextSwitch(working_process, min_index);
        } else {
            working_process = -1;
        }
    }
}



void schedule_FCFS() {
    if (processes[working_process].state == FINISHED || working_process == -1) {
        for (int i = 0; i < num_processes; i++) {
            if (processes[i].state == READY) {
                contextSwitch(working_process, i);
                return;
            }
        }
        working_process = -1;
    }
}

void logProcessState(int p)
{
    FILE *logFile = fopen("scheduler.log", "a");

    if (processes[p].state == RUNNING && processes[p].remainingtime < processes[p].runtime)
    {
        fprintf(logFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",
                time, processes[p].id, processes[p].arrival_time,
                processes[p].runtime, processes[p].remainingtime,
                processes[p].waiting_time);
    }
    else
    {
        switch (processes[p].state)
        {
        case RUNNING:
            fprintf(logFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",
                    time, processes[p].id, processes[p].arrival_time,
                    processes[p].runtime, processes[p].remainingtime,
                    processes[p].waiting_time);
            break;
        case READY:
            fprintf(logFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",
                    time, processes[p].id, processes[p].arrival_time,
                    processes[p].runtime, processes[p].remainingtime,
                    processes[p].waiting_time);
            break;
        case FINISHED:
            fprintf(logFile, "At time %d process %d finished arr %d total %d remain %d wait %d\n",
                    time, processes[p].id, processes[p].arrival_time,
                    processes[p].runtime, processes[p].remainingtime,
                    processes[p].waiting_time);
            break;
        }
    }

    fclose(logFile);
}

void updateProcessStatistics(int i)
{
    processes[i].waiting_time = time - processes[i].arrival_time - processes[i].runtime + processes[i].remainingtime;
    processes[i].turnaround_time = time - processes[i].arrival_time;
    processes[i].weighted_turnaround_time = (float)processes[i].turnaround_time / processes[i].runtime;
}
void calculatePerformance()
{
    float totalWTA = 0, totalWaiting = 0, stdWTA = 0;
    int finishedProcesses = 0;

    for (int i = 0; i < num_processes; i++)
    {
        if (processes[i].state == FINISHED)
        {
            totalWTA += processes[i].weighted_turnaround_time;
            totalWaiting += processes[i].waiting_time;
            finishedProcesses++;
        }
    }

    if (finishedProcesses == 0) return;

    float avgWTA = totalWTA / finishedProcesses;
    float avgWaiting = totalWaiting / finishedProcesses;

    for (int i = 0; i < num_processes; i++)
    {
        if (processes[i].state == FINISHED)
        {
            stdWTA += pow(processes[i].weighted_turnaround_time - avgWTA, 2);
        }
    }

    stdWTA = sqrt(stdWTA / finishedProcesses);
    float cpuUtilization = 100.0*((float)STOP_TIME - (float)IDLE_TIME) / (float)STOP_TIME;
    FILE *perfFile = fopen("scheduler.perf", "w");
    fprintf(perfFile, "CPU utilization = %0.2f%%\n",cpuUtilization);
    fprintf(perfFile, "Avg WTA = %.2f\n", avgWTA);
    fprintf(perfFile, "Avg Waiting = %.2f\n", avgWaiting);
    fprintf(perfFile, "Std WTA = %.2f\n", stdWTA);
    fclose(perfFile);
}
void printProcesses() {
    printf("Printing Process Information:\n");
    for (int i = 0; i < num_processes; ++i) {
        printf("Process %d:\n", processes[i].id);
        printf("  PID: %d\n", processes[i].pid);
        printf("  Arrival Time: %d\n", processes[i].arrival_time);
        printf("  Runtime: %d\n", processes[i].runtime);
        printf("  Remaining Time: %d\n", processes[i].remainingtime);
        printf("  Waiting Time: %d\n", processes[i].waiting_time);
        printf("  State: %d\n", processes[i].state);
        printf("  Turnaround Time: %d\n", processes[i].turnaround_time);
        printf("  Weighted Turnaround Time: %d\n", processes[i].weighted_turnaround_time);
        printf("\n");
    }
}
void sigint_handler(int signo) {
    if (signo == SIGINT) {
        STOP_TIME=time;
        calculatePerformance();
        exit(EXIT_SUCCESS);
    }
}
void setNonBlockingStdin() {
    int flags = fcntl(fileno(stdin), F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    if (fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}
