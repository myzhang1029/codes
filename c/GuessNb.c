#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void)
{
    int rd = (srand(time(NULL)), rand() % 101), gs = 0, rnds = 0;
    time_t nt = time(NULL);
    char input[10];

    while (++rnds)
    {
        printf("Input your guess between 0 and 100: ");
        fflush(stdout);
        fgets(input, 10, stdin);
        gs = atoi(input);
        if (gs == rd)
        {
            printf("\nYou won the game in %ld seconds or %d rounds\n",
                   time(NULL) - nt, rnds);
            break;
        }
        else if (rd < gs)
            printf("Too big.\n");
        else
            printf("Too small.\n");
    }
    return 0;
}
