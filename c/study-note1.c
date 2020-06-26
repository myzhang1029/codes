/* Simple program to demonstrate C lists */
#include <stdio.h>
int main(void)
{
    /* Empty list with 4 slots */
    char a[4];
    /* Just fits but no room for NUL, see line 18 */
    char b[4] = "1234";
    /* c[4] for NUL */
    char c[] = "1234";
    /* Initializer too long */
    char d[4] = "12345";
    /* Under-initialized */
    char e[5] = "1234";
    char f[6] = "1234";
    printf("a:%zu , b:%zu, c:%zu, d:%zu, e:%zu, f:%zu\n", sizeof(a), sizeof(b), sizeof(c), sizeof(d), sizeof(e), sizeof(f));
    /* Example:
     * a:4 , b:4, c:5, d:4, e:5, f:6
     */
    /* We cannot print b */
    puts(b);
    /* Example:
     * 1234%?k
     */
    return 0;
}
