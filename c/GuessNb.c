#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void)
{
    int rd = (srand(time(NULL)), rand() % 101), gs = 0, nt = time(NULL),
        rnds = 0;
    while (++rnds)
    {
        printf("Input your guess between 1 and 100: ");
        fflush(stdin);
        scanf("%d", &gs);
        if (gs == rd)
        {
            printf("\nYou won the game in %d seconds or %d rounds\n",
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
