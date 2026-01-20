#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char *DECRYPT_COMMAND[] = {
    "gpg-sq",
    "--decrypt",
    "PLACEHOLDER",
    NULL
};
const size_t DECRYPT_PLACEHOLDER_IDX = 2;

char *SSHPASS = "sshpass";

int main(int argc, char **argv) {
    int pipefd[2];
    pid_t pid;
    char fdline[] = "-dXXXXXX";

    if (argc < 2 || strcmp(argv[1], "-h") == 0) {
        fprintf(stderr, "Usage: %s <PGP_PW_FILENAME> [...]\n", argv[0]);
        fprintf(stderr, "\nTrailing arguments passed to sshpass(1)\n");
        return 1;
    }

    /* Pass the PGP-encrypted filename */
    DECRYPT_COMMAND[DECRYPT_PLACEHOLDER_IDX] = argv[1];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 2;
    }
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 3;
    }
    if (pid == 0) {
        /* Child */
        /* Close read half */
        if (close(pipefd[0]) == -1) {
            perror("child read close");
            _exit(4);
        }
        /* Close original stdout explicitly */
        if (close(1) == -1) {
            perror("child stdout close");
            _exit(5);
        }
        if (dup2(pipefd[1], 1) == -1) {
            perror("child dup2");
            _exit(6);
        }
        /* Close original pipe write half */
        if (close(pipefd[1]) == -1) {
            perror("child write close");
            _exit(7);
        }
        execvp(DECRYPT_COMMAND[0], DECRYPT_COMMAND);
        perror("child execvp");
        _exit(8);
    }
    /* Parent */
    /* Close write half */
    if (close(pipefd[1]) == -1) {
        perror("parent write close");
        return 9;
    }
    /* Prepare the read half fd */
    if ((size_t) snprintf(fdline, sizeof(fdline), "-d%d", pipefd[0]) >= sizeof(fdline)) {
        fprintf(stderr, "pipe is unhappy today. Try again.\n");
        return 11;
    }
    /* Prepare the new argv by replacing [0] and [1] in place */
    argv[0] = SSHPASS;
    argv[1] = fdline;
    /* ISO/IEC 9899:1990 5.1.2.2.1, paragraph 2, but to make sure */
    argv[argc] = NULL;
    execvp(argv[0], argv);
    perror("parent execvp");
    close(pipefd[0]);
    return 12;
}
