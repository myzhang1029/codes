int _(char*j){return(*j?putchar(*j),_(++j):putchar(10));}int main(int i,char**j){return(i?_(*j),main(--i,++j):i);}
