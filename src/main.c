#include "alloc.h"
#include <stdio.h>

int main(void) {
    int *a = (int *)tumalloc(5 * sizeof(int));
    if (a == NULL) {
        printf("tumalloc failed\n");
        return 1;
    }

    for (int i = 0; i < 5; i++) {
        a[i] = (i + 1) * 10;
    }

    for (int i = 0; i < 5; i++) {
        printf("%d\n", a[i]);
    }

    int *b = (int *)tucalloc(10, sizeof(int));
    if (b == NULL) {
        printf("tucalloc failed\n");
        tufree(a);
        return 1;
    }

    b[0] = 111;
    b[9] = 999;

    int *bigger = (int *)turealloc(b, 20 * sizeof(int));
    if (bigger == NULL) {
        printf("turealloc failed\n");
        tufree(a);
        tufree(b);
        return 1;
    }

    for (int i = 10; i < 20; i++) {
        bigger[i] = i;
    }

    tufree(a);
    tufree(bigger);

    return 0;
}
