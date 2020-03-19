/* Automatically publish public IP to a git README.md */
/* dependencies: getpid(), myzhang1029/slib, libcurl and libgit2 */
/*
 *  autoip.c
 *  Copyright (C) 2017, 2018 Zhang Maiyun <myzhang1029@hotmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <memory.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <curl/curl.h>
#include <git2.h>
#include <slib.h>
#include <slib/getopt.h>

#define LFILE "/var/log/autoip.log"
#define SITEDDNS "/Users/zmy/autoip/siteddns"
#define SSHPUB "/Users/zmy/.ssh/id_rsa.pub"
#define SSHPRI "/Users/zmy/.ssh/id_rsa"
#define SSHUSR "git"
#define SSHPAS ""
#define UAGENT                                                                 \
    "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:58.0) Gecko/20100101 "         \
    "Firefox/58.0"

jmp_buf exit_point;

struct MemoryStruct
{
    char *memory;
    size_t size;
};

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                           void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL)
    {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static char *getctime()
{
    time_t t;
    char *str;
    time(&t);
    str = strdup(ctime(&t));
    str[strlen(str) - 1] = 0;
    return str;
}

void rlae(int sig)
{
    printf("catch signal %d, exiting\n", sig);
    longjmp(exit_point, 2);
}

void aierror(FILE *dest, char *fmt, ...)
{
    va_list ap;
    char *cutime = getctime();
    fprintf(dest, "(error)[%s] ", cutime);
    va_start(ap, fmt);
    vfprintf(dest, fmt, ap);
    va_end(ap);
    fprintf(dest, "\n");
    free(cutime);
}

void aiinfo(FILE *dest, char *fmt, ...)
{
    va_list ap;
    char *cutime = getctime();
    fprintf(dest, "(info)[%s] ", cutime);
    va_start(ap, fmt);
    vfprintf(dest, fmt, ap);
    va_end(ap);
    fprintf(dest, "\n");
    free(cutime);
}

void git_usual_error(int error_code, FILE *log, const char *funcname)
{
    const git_error *error = giterr_last();
    if (!error_code)
        return;
    aierror(log, "%s returned %d: %s", funcname, error_code,
            (error && error->message) ? error->message : "???");
}

void git_fatal_error(int error_code, FILE *log, const char *funcname)
{
    const git_error *error = giterr_last();
    if (!error_code)
        return;
    aierror(log, "%s returned %d: %s", funcname, error_code,
            (error && error->message) ? error->message : "???");
    aiinfo(log, "Exiting");
    longjmp(exit_point, 2);
}

/* Change whenever you need another source */
char *get_ip_by_curl()
{
    CURL *curl_handle;
    CURLcode res;
    struct MemoryStruct chunk;
    char *tmp, *ip;
    chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
    chunk.size = 0;           /* no data at this point */
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    /* Set URL */
    curl_easy_setopt(curl_handle, CURLOPT_URL,
                     "http://whois.pconline.com.cn/ipJson.jsp");
    /* Set memory callback */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    /* Set output data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    /* Set User-Agent */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, UAGENT);
    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
    {
        curl_easy_cleanup(curl_handle);
        curl_global_cleanup();
        return NULL;
    }
    else
    {
        /* Get the fourth field */
        tmp = strchr(chunk.memory, '"') + 1;
        tmp = strchr(tmp, '"') + 1;
        tmp = strchr(tmp, '"') + 1;
        *(strchr(tmp, '"')) = 0;
        ip = malloc(strlen(tmp) + 1);
        strcpy(ip, tmp);
    }
    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    free(chunk.memory);

    curl_global_cleanup();
    return ip;
}
void commit_changes(git_repository *repo, git_index *index, FILE *log)
{
    git_strarray strarray;
    git_tree *old_tree, *tree;
    git_commit *parent;
    git_signature *author;
    git_oid tree_id, parent_id, commit_id;
    char *pathspec = "*";
    strarray.strings = (char **)malloc(sizeof(char *));
    strarray.strings[0] = pathspec;
    strarray.count = 1;

    git_fatal_error(git_index_add_all(index, &strarray, 0, NULL, NULL), log,
                    "git_index_add_all");
    git_fatal_error(git_index_write(index), log, "git_index_write");
    git_fatal_error(git_index_write_tree(&tree_id, index), log,
                    "git_index_write_tree");
    git_index_free(index);
    git_fatal_error(git_tree_lookup(&tree, repo, &tree_id), log,
                    "git-tree_lookup");

    git_fatal_error(git_reference_name_to_id(&parent_id, repo, "HEAD"), log,
                    "git_reference_name_to_oid");
    git_fatal_error(git_commit_lookup(&parent, repo, &parent_id), log,
                    "git_commit_lookup");

    git_fatal_error(git_signature_default(&author, repo), log,
                    "git_signature_default");

    git_fatal_error(git_commit_create_v(&commit_id, repo, "HEAD", author,
                                        author, NULL, "Update IP", tree, 1,
                                        parent),
                    log, "git_commit_create_v");

    git_signature_free(author);
    git_tree_free(tree);
    git_commit_free(parent);
}

int cred_cb(git_cred **out, const char *url, const char *username_from_url,
            unsigned int allowed_types, void *payload)
{
    return git_cred_ssh_key_new(out, SSHUSR, SSHPUB, SSHPRI, SSHPAS);
}

void push_refs(git_repository *repo, FILE *log)
{
    git_strarray strarray;
    git_remote *remote;
    git_push_options opts;
    char *ref = "refs/heads/master:refs/heads/master";
    strarray.strings = (char **)malloc(sizeof(char *));
    strarray.strings[0] = ref;
    strarray.count = 1;

    git_fatal_error(git_remote_lookup(&remote, repo, "origin"), log,
                    "git_remote_lookup");
    git_fatal_error(git_push_init_options(&opts, GIT_PUSH_OPTIONS_VERSION), log,
                    "git_push_init_options");
    opts.callbacks.credentials = cred_cb;
    git_fatal_error(git_remote_push(remote, &strarray, &opts), log,
                    "git_remote_push");

    git_remote_free(remote);
}

int main(int argc, char **argv)
{
    FILE *log = NULL;
    char *current_ip, *old_ip = NULL;
    int opt;
    int check = 1;
    int pinfo = 1, sleeptime = 60;
    char *siteddnsdir = NULL;
    const char *opts = ":ovqrht:d:";
    int error;
    git_repository *siteddns = NULL;
    git_index *index = NULL;

    signal(SIGINT, rlae);
    signal(SIGTERM, rlae);
    signal(SIGQUIT, rlae);
    signal(SIGHUP, SIG_IGN);
    opterrGS = 0;

    while ((opt = getoptGS(argc, argv, opts)) != -1)
    {
        switch (opt)
        {
            case 'o':
                log = stdout;
                break;
            case 'v':
                printf("c-autoip build %s\n", __DATE__);
                goto exithere;
            case 'q':
                pinfo = 0;
                break;
            case 'h':
                printf(
                    "autoip [options]\n"
                    "Options:\n"
                    "-o: log to stdin\n"
                    "-v: print version\n"
                    "-q: disable infomations\n"
                    "-h: print this\n"
                    "-t SEC: set waiting time in seconds after each check[60]\n"
                    "-d DIR: set siteddns dir\n");
                goto exithere;
            case 'r':
                check = 0;
                break;
            case 't':
                sleeptime = atoi(optargGS);
                if (sleeptime < 10)
                {
                    fprintf(stderr, "-t: cannot be lower than 10\n");
                    goto exithere;
                }
                break;
            case 'd':
                siteddnsdir = malloc(strlen(optargGS) + 1);
                strcpy(siteddnsdir, optargGS);
                break;
            case ':':
                fprintf(stderr, "-%c: argument expected\n", optoptGS);
                goto exithere;
            default:
                fprintf(stderr, "-%c: no such option\n", optoptGS);
                goto exithere;
        }
    }

    if (siteddnsdir == NULL)
    {
        siteddnsdir = malloc(strlen(SITEDDNS) + 1);
        if (!siteddnsdir)
        {
            fprintf(stderr, "malloc failed\n");
            goto exithere;
        }
        strcpy(siteddnsdir, SITEDDNS);
    }

    if (log == NULL)
    {
        if (check)
        {
            FILE *test = fopen(LFILE, "r");
            if (test)
            {
                char pid[16];
                fseek(test, 0, SEEK_SET);
                fgets(pid, 16, test);
                fprintf(stderr, "AutoIP with pid %s already running\n", pid);
                fclose(test);
                exit(1);
            }
        }
        log = fopen(LFILE, "w");
        if (!log)
        {
            fprintf(stderr, "log failed\n");
            log = stdout;
        }
    }

    git_libgit2_init();
    error = git_repository_open(&siteddns, siteddnsdir);
    git_fatal_error(error, log, "git_repository_open");
    error = git_repository_index(&index, siteddns);
    git_fatal_error(error, log, "git_repository_index");

    fprintf(log, "%d\n", getpid());

    if (setjmp(exit_point) == 2)
        goto exithere;
    for (;;)
    {
        if (pinfo)
            aiinfo(log, "Fetching");
        if ((current_ip = get_ip_by_curl()) == NULL)
        {
            aierror(log, "curl_easy_perform() failed\n");
            sleepS(20);
            continue;
        }
        if (pinfo)
        {
            aiinfo(log, "current ip is %s", current_ip);
            aiinfo(log, "Diffing");
        }
        if (!old_ip || strcmp(current_ip, old_ip) != 0)
        {
            FILE *siteddnsfil;
            pid_t pid;
            int stat;
            if (pinfo)
                aiinfo(log, "Publish");
            if ((siteddnsfil = fopen(SITEDDNS "/README", "w")) == NULL)
            {
                aierror(log, "fopen failed for README: %s", strerror(errno));

                goto exithere;
            }
            fputs(current_ip, siteddnsfil);
            fclose(siteddnsfil);
            commit_changes(siteddns, index, log);
            push_refs(siteddns, log);
        }
        if (old_ip)
            free(old_ip);
        old_ip = malloc(strlen(current_ip) + 1);
        strcpy(old_ip, current_ip);
        free(current_ip);
        if (pinfo)
            aiinfo(log, "Suspend");
        sleepS(60);
    }
exithere:
    if (log && log != stdout)
        fclose(log);
    if (index)
        git_index_free(index);
    if (siteddns)
        git_repository_free(siteddns);
    if (siteddnsdir)
        free(siteddnsdir);
    git_libgit2_shutdown();
    remove(LFILE);
    return 1;
}
