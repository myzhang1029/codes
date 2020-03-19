/* One-liner to print out a line.
 * Requires only syscall write and exit, even no crt0.o
 * c() is standard putchar;
 * s() is standard puts.
 * e() is standard exit.
 */
/*
 *  oa3.c
 *  Copyright (C) 2018 Zhang Maiyun <myzhang1029@hotmail.com>
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

#define w 0x2000004
#define x 0x2000001
#define n int
#define t "Hello, World!"
n _ = 0xa;
void e(n r)
{
    asm("movl %0,%%edi\n\tmovq %1,%%rax\n\tsyscall" ::"r"(r), "i"(x));
}
n c(n c)
{
    n r;
    asm("movq %1,%%rsi\n\tmovq %2,%%rax\n\tmovq $1,%%rdi\n\tmovq "
        "$1,%%rdx\n\tsyscall"
        : "=a"(r)
        : "r"(&c), "i"(w));
    return (r >= 0 ? c : -1);
}
n s(char *j) { return (*j ? c(*j), s(++j) : c(_)); }
n main() { e((s(t), 0)); }
