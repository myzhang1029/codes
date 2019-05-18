/* One man has n children, his children have n-1 children each, they have n-2 children each, how many people are there
 * in the family after n generations?
 * the mother side not included */
/* n specified in argv[1], defaults to 1 */
/* UPDATE: this program actually calculates OEIS:A000522 */
/*
 * generation.c
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

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	unsigned long long men = atoi(argc > 1 ? argv[1] : "1");
	unsigned count = men;	 /* count for generations */
	unsigned long long total = 1; /* People in total */
	total += men;
	while (--count)
	{
		men *= count;
		total += men;
	}
	printf("%lld\n", total);
	return 0;
}
