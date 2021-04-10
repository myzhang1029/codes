/* Internet issue finder for macOS by Zhang Maiyun */
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void die(const char *message)
{
    perror(message);
    getchar();
    exit(1);
}

int setup_logger(int argc, const char * const argv[])
{
    int fd;
    if (argc > 1)
        fd = creat(argv[1], 0644);
    else
    {
        /* Create in the same directory as the executable if possible
           for easier location.
           If run as a command in PATH, log is created in CWD.
         */
        char *pathname = malloc(strlen(argv[0]) + 5);
        strcpy(pathname, argv[0]);
        strcat(pathname, ".log");
        fd = creat(pathname, 0644);
        free(pathname);
    }
    if (fd < 0)
        die("Cannot open log file");
    return fd;
}

void log_command(int log_fd, char *const argv[])
{
    size_t count = 0;
    write(log_fd, "\n+\tRunning ", 11);
    for (; argv[count]; ++count)
    {
        write(log_fd, argv[count], strlen(argv[count]));
        write(log_fd, " ", 1);
    }
    write(log_fd, "\n", 1);
}

/* Spawn process, setup log and execute */
int execute_vp(int log_fd, const char *file, char *const argv[])
{
    pid_t pid = fork();
    int return_value = 0;
    if (pid < 0)
        die("Cannot fork");
    if (pid == 0)
    {
        /* Child process */
        close(0);
        close(1);
        close(2);
        log_command(log_fd, argv);
        if (dup2(log_fd, 1) < 0)
            die("Cannot duplicate log fd to stdout");
        if (dup2(log_fd, 2) < 0)
            die("Cannot duplicate log fd to stderr");
        close(log_fd);
        execvp(file, argv);
    }
    if (pid > 0)
    {
        /* Parent process */
        waitpid(pid, &return_value, 0);
        dprintf(log_fd, "-\tCommand exited with status %d\n", return_value);
    }
    return return_value;
}

/* va version of execute_vp */
int execute_lp(int log_fd, const char *file, const char *arg, ...)
{
    va_list ap;
    int return_value;
    /* Used size includes ARG and NULL */
    size_t have_size = 4, used_size = 2;
    char **args = malloc(sizeof(char *) * have_size), *argn;
    if (args == NULL)
        die("Cannot allocate memory");
    args[0] = (char *)arg;
    va_start(ap, arg);
    while ((argn = va_arg(ap, char *)) != NULL)
    {
        if (used_size == have_size)
        {
            char **new_args = realloc(args, sizeof(char *) * (have_size <<= 1));
            if (new_args == NULL)
                die("Cannot allocate memory");
            args = new_args;
        }
        args[used_size - 1] = argn;
        ++used_size;
    }
    args[used_size - 1] = NULL;
    return_value = execute_vp(log_fd, file, args);
    free(args);
    return return_value;
}

char *get_ifaces()
{
    int pipe_fd[2];
    pid_t pid;
    size_t have_size = 32, used_size = 1;
    char *result = malloc(sizeof(char) * have_size);
    if (result == NULL)
        die("Cannot allocate memory");
    pipe(pipe_fd);
    pid = fork();
    if (pid < 0)
        die("Cannot fork");
    if (pid == 0)
    {
        /* Child process */
        close(pipe_fd[0]);
        close(1);
        if (dup2(pipe_fd[1], 1) < 0)
            die("Cannot duplicate pipe fd");
        close(pipe_fd[1]);
        execlp("ifconfig", "ifconfig", "-l", NULL);
    }
    if (pid > 0)
    {
        /* Parent process */
        int exit_stat;
        ssize_t size_read;
        /* close input side */
        close(pipe_fd[1]);
        while (1)
        {
            if (used_size >= have_size)
            {
                char *new_result =
                    realloc(result, sizeof(char) * (have_size <<= 1));
                if (new_result == NULL)
                    die("Cannot allocate memory");
                result = new_result;
            }
            size_read =
                read(pipe_fd[0], result + used_size - 1, have_size - used_size);
            if (size_read < 0)
                die("Cannot read pipe");
            if (size_read == 0)
            {
                close(pipe_fd[0]);
                break;
            }
            used_size += size_read;
        }
        waitpid(pid, &exit_stat, 0);
        if (exit_stat != 0)
            die("Cannot read interfaces");
    }
    result[used_size - 1] = '\0';
    return result;
}

int main(int argc, const char * const argv[])
{
    int log_fd = setup_logger(argc, argv);
    int sum_return = 0;

#define run(...) sum_return += execute_lp(log_fd, __VA_ARGS__, NULL)
    puts("macOS network debugger by Zhang Maiyun, built " __DATE__ " " __TIME__);
    puts("--- Logging system information");
    run("uname", "uname", "-a");

    puts("--- Logging interface information");
    run("ifconfig", "ifconfig");

    puts("--- Checking interfaces");
    puts("*** Loopback interface check");
    run("ping", "ping", "-vc3", "127.0.0.1");
    puts("*** Ethernet interface check: ping gateways");
    run("ping", "ping", "-vc3", "10.11.200.1");
    /* ping second stage gateway. if reachable then wifi should be OK */
    run("ping", "ping", "-vc3", "10.10.10.10");
    /* try pinging an external ip */
    puts("*** Ethernet interface check: ping 1.0.0.1");
    run("ping", "ping", "-vc3", "1.0.0.1");

    /* Check ARP table of every ethernet interface */
    puts("--- Checking arp tables");
    {
        char *saveptr;
        char *ifaces = get_ifaces();
        char *iface = strtok_r(ifaces, " \n", &saveptr);
        while ((iface = strtok_r(NULL, " \n", &saveptr)))
        {
            printf("*** Interface %s\n", iface);
            run("arp", "arp", "-ai", iface);
        }
        free(ifaces);
    }

    puts("--- Checking DNS");
    puts("*** Cloudflare DNS one.one.one.one");
    run("dig", "dig", "one.one.one.one", "@1.0.0.1");
    puts("*** System DNS one.one.one.one");
    run("dig", "dig", "one.one.one.one");

    puts("--- Checking SSL connection to 1.0.0.1");
    run("openssl", "openssl", "s_client", "-connect", "1.0.0.1:443");

    puts("--- Testing HTTPS with curl");
    run("curl", "curl", "https://apple.com");

    /* Final item (may take long): route logging */
    puts("--- Tracing route to 1.0.0.1");
    run("traceroute", "traceroute", "1.0.0.1");

    return sum_return;
}
