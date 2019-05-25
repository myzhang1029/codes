/* You type in music written in numbered musical notation, then I give you how to read it(not completed)*/
/*
 *  musiccvrt.c
 *  Copyright (C) 2017 Zhang Maiyun <myzhang1029@163.com>
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

#include <ctype.h>
#include <slib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <slib/getopt.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAXLEN 262144

int getlen(char **stack)
{
	int i = 0;
	for (; (*stack)[i]; ++i)
		;
	return i;
}

int push(char **stack, char c)
{
	if (getlen(stack) == MAXLEN)
		return -1;
	(*stack)[getlen(stack)] = c;
	return 0;
}

int numeric(FILE *otpt, char *stack, int c, double hits)
{
	if (!getlen(&stack))
	{
		/* First iter */
		push(&stack, c);
	}
	switch (stack[0])
	{
		case '0':
			fprintf(otpt, "stop for ");
			break;
		case '1':
			fprintf(otpt, "do for ");
			break;
		case '2':
			fprintf(otpt, "re for ");
			break;
		case '3':
			fprintf(otpt, "mi for ");
			break;
		case '4':
			fprintf(otpt, "fa for ");
			break;
		case '5':
			fprintf(otpt, "sol for ");
			break;
		case '6':
			fprintf(otpt, "la for ");
			break;
		case '7':
			fprintf(otpt, "ti for ");
			break;
		default:
			fprintf(stderr, "Invalid number: %c\n", stack[0]);
			return 1;
	}
	for (int i = 1; i <= getlen(&stack); i++)
	{
		switch (stack[i])
		{
			case ',':
				fprintf(otpt, "Hi ");
				break;
			case '\'':
				fprintf(otpt, "Lo ");
				break;
			case '_':
				hits /= 2;
				break;
			case '-':
				hits *= 2;
				break;
			case '.':
				hits += hits / 2;
				break;
			default:
				break;
		}
	}
	fprintf(otpt, "%lf hits\n", hits);
	hits = 1;
	memset(stack, 0, sizeof(char) * MAXLEN);
	return 0;
}

void check_inpt(FILE **inpt, FILE **otpt, int argc, char **argv)
{
	char *usage = "%s [inputFile] [-o outputFile] or %s -h\n\nIf no input "
		      "file provided, read STDIN;\nif no output file provided, "
		      "write STDOUT.\n";
	if (argc != 1)
	{
		int ch;
		struct stat statinfo;
		opterrGS = 0;
		while ((ch = getoptGS(argc, argv, ":o:h")) != -1)
		{
			switch (ch)
			{
				case 'o':
					*otpt = fopen(optargGS, "w");
					if (!(*otpt))
					{
						prterr("Fopen(output) failed.");
						exit(1);
					}
					break;
				case 'h':
					printf(usage, argv[0]);
					exit(0);
				case ':':
					fprintf(stderr, "%s: '-o': Missing argument\n", argv[0]);
					exit(1);
				case '?':
					fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], ch);
					exit(1);
				default:
					break;
			}
		}
		if (optindGS != argc)
		{
			if (stat(argv[optindGS], &statinfo) != 0)
			{
				fprintf(stderr, "stat: %s: %s\n", argv[optindGS], strerror(errno));
				exit(1);
			}
			if (statinfo.st_mode & S_IFDIR)
			{
				fprintf(stderr, "%s: %s: Is a directory\n", argv[0], argv[optindGS]);
				exit(1);
			}
			*inpt = fopen(argv[optindGS], "r");
			if (!(*inpt))
			{
				fprintf(stderr, "Fopen(input) failed");
				fclose(*otpt);
				exit(1);
			}
		}
	}
	if (*inpt == NULL)
		*inpt = stdin;
	if (*otpt == NULL)
		*otpt = stdout;
}

int main(int argc, char **argv)
{
	FILE *inpt = NULL, *otpt = NULL;
	double hits = 1;
	char *stack = malloc(sizeof(char) * MAXLEN);
	char c;
	check_inpt(&inpt, &otpt, argc, argv);
	while ((c = getc(inpt)) != EOF)
	{
		if (isdigit(c))
		{
			if (numeric(otpt, stack, c, hits))
				goto out;
		}
		else
		{
			switch (c)
			{
				case ',':
				case '\'':
				case '_':
				case '-':
				case '.':
					push(&stack, c);
					break;
				case 0:
				case '\n':
				case ' ':
					continue;
				default:
					fprintf(stderr, "Unknown symble: %c\n", c);
					goto out;
			}
		}
	}
out:
	free(stack);
	if (inpt != stdin)
		fclose(inpt);
	if (otpt != stdout)
		fclose(otpt);
	return 0;
}
