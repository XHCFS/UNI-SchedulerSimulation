#include "headers.h"

int scheduler_pipe[2];
void clearResources(int);
int *shmaddr;
int num_processes=0;
int next_process=0;
int time=0;
pid_t scheduler_pid, clock_pid;

Process_Params processes[100];

void printProcesses() {
    printf("Processes:\n");
    printf("ID\tArrival Time\tRuntime\n");
    for (int i = 0; i < num_processes; i++) {
        printf("%d\t%d\t\t%d\n", processes[i].id, processes[i].arrival_time, processes[i].runtime);
        // Add more fields as needed
    }
}
Process_Params parse_process(char *line){
    Process_Params p;
    sscanf(line,"%d %d %d", &p.id,&p.arrival_time,&p.runtime);
    return p;
}

void read_file() {
    FILE *fp = fopen("processes.txt", "r");
    if (fp == NULL) {
        perror("Error: input file");
        return;
    }

    char line[120];
    int lineCount = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        lineCount++;
        if (lineCount == 1) {
            continue;
        }
        Process_Params p = parse_process(line);
        processes[num_processes++] = p;
    }
    fclose(fp);
    printProcesses();
}

int getUserInput() {
    int input;
    do {
        printf("Select a scheduling algorithm:\n");
        printf("0: FCFS\n1: RR\n2: SRTN\n3: SJF\n");
        printf("Enter the corresponding number: ");

        if (scanf("%d", &input) != 1) {
            printf("Invalid input. Please enter a valid number.\n");
            while (getchar() != '\n');
        } else {
            break; 
        }
    } while (1);

    return input;
}

int main(int argc, char * argv[])
{

    int algorithm = getUserInput();
    if (pipe(scheduler_pipe)==-1){
        perror("Error: creating pipe");
        return 1;
    }

    scheduler_pid= fork();
    if (scheduler_pid== 0) {
        close(scheduler_pipe[1]);
        dup2(scheduler_pipe[0], STDIN_FILENO);
        char algorithm_str[2];
        snprintf(algorithm_str, sizeof(algorithm_str), "%d", algorithm);
        execl("./scheduler.out", "./scheduler.out", algorithm_str, NULL);
    }
    close(scheduler_pipe[0]);

    read_file();
    signal(SIGINT, clearResources);
    
    clock_pid= fork();
    if (clock_pid==0){
        execl("./clk.out", "./clk.out", NULL);
        perror("Error: Did not create clk properly");
    }
    initClk();

    while(1){
        if(time==getClk()){continue;}
        time=getClk();
        printf("current time is %d\n", time);
        while (processes[next_process].arrival_time==time && num_processes>0){
            write(scheduler_pipe[1], &processes[next_process], sizeof(Process_Params));
            num_processes--;next_process++;
        }
   }
}

void clearResources(int signum)
{
    close(scheduler_pipe[1]);
    destroyClk(true);
    exit(0);
}
