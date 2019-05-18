/* One-liner to print out a line.
 * Requires only syscall write and exit, even no crt0.o
 * c() is standard putchar;
 * s() is standard puts.
 * e() is standard exit.
 */
#define w 0x2000004
#define x 0x2000001
#define n int
#define t "Hello, World!"
n _ = 0xa;void e(n r){asm("movl %0,%%edi\n\tmovq %1,%%rax\n\tsyscall"::"r"(r),"i"(x));}n c(n c){n r;asm("movq %1,%%rsi\n\tmovq %2,%%rax\n\tmovq $1,%%rdi\n\tmovq $1,%%rdx\n\tsyscall":"=a"(r):"r"(&c),"i"(w));return(r>=0?c:-1);}n s(char*j){return(*j?c(*j),s(++j):c(_));}n main(){e((s(t),0));}
