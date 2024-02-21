#include "headers.h"

int remainingtime;

void sigusr2_handler(int signo);

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <id> <runtime>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int id = atoi(argv[1]);
    int runtime = atoi(argv[2]);

    remainingtime = runtime;

    if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR)
    {
        perror("Error setting up signal handler");
        exit(EXIT_FAILURE);
    }

    while (remainingtime > 0)
    {
        pause();
    }
    return 0;
}

void sigusr2_handler(int signo)
{
    if (signal(SIGUSR2, sigusr2_handler) == SIG_ERR)
    {
        perror("Error setting up signal handler");
        exit(EXIT_FAILURE);
    }
    if (signo == SIGUSR2)
    {
        remainingtime--;
    }
}
