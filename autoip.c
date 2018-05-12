/* Automatically publish public IP to a git README.md */
/* Requires UNIX API, myzhang1029/slib, libcurl and working git binary */
/*
 * autoip.c
 * Copyright (C) 2017 Zhang Maiyun <myzhang1029@163.com>
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <errno.h>
#include <memory.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <slib.h>
#include <curl/curl.h>
#include <slib/getopt.h>

#define GIT "/usr/local/bin/git"
#define LFILE "/Users/zmy/autoip/autoip.log"
#define SITEDDNS "/Users/zmy/autoip/siteddns"
#define UAGENT "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:58.0) Gecko/20100101 Firefox/58.0"

jmp_buf exit_point;

struct MemoryStruct
{
	char *memory;
	size_t size;
};

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
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

/* Change whenever you need another source */
char *get_ip_by_curl()
{
	CURL *curl_handle;
	CURLcode res;
	struct MemoryStruct chunk;
	char *tmp, *ip;
	chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
	chunk.size = 0;		  /* no data at this point */
	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	/* Set URL */
	curl_easy_setopt(curl_handle, CURLOPT_URL, "http://whois.pconline.com.cn/ipJson.jsp");
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

int main(int argc, char **argv)
{
	FILE *log = NULL;
	char *current_ip, *old_ip = NULL;
	int opt;
	int check = 1;
	int pinfo = 1, sleeptime = 60;
	char *siteddnsdir = NULL;
	const char *opts = ":ovqrht:d:";
	signal(SIGINT, rlae);
	signal(SIGTERM, rlae);
	signal(SIGQUIT, rlae);
	signal(SIGSEGV, rlae);
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
				printf("autoip [options]\n"
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
			FILE *siteddns;
			pid_t pid;
			int stat;
			if (pinfo)
				aiinfo(log, "Publish");
			if ((siteddns = fopen(SITEDDNS "/README.md", "w")) == NULL)
			{
				aierror(log, "fopen failed for README.md: %s", strerror(errno));

				goto exithere;
			}
			fputs(current_ip, siteddns);
			fclose(siteddns);
			if (check)
				chdir(SITEDDNS);
			if ((pid = fork()) == 0)
			{
				execl(GIT, "git", "commit", "-am", "Update", NULL);
			}
			else if (pid > 0)
			{
				waitpid(pid, &stat, 0);
			}
			else
				aierror(log, "fork failed: %s", strerror(errno));
			if (stat != 0)
				aierror(log, "git commit failed with status %d", stat);
			if ((pid = fork()) == 0)
			{
				execl(GIT, "git", "push", NULL);
			}
			else if (pid > 0)
			{
				waitpid(pid, &stat, 0);
			}
			else
				aierror(log, "fork failed: %s", strerror(errno));
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
	if (siteddnsdir)
		free(siteddnsdir);
	remove(LFILE);
}
