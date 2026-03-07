#include "alloc.h"
#include <stdio.h>

static void print_array(const char *label, int *arr, int count) {
    printf("%s\n", label);
    for (int i = 0; i < count; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int main(void) {
    printf("=== tumalloc basic test ===\n");
    int *a = (int *)tumalloc(5 * sizeof(int));
    if (a == NULL) {
        printf("tumalloc failed\n");
        return 1;
    }

    for (int i = 0; i < 5; i++) {
        a[i] = (i + 1) * 10;
    }
    print_array("a contents:", a, 5);

    printf("\n=== tucalloc zero-initialization test ===\n");
    int *b = (int *)tucalloc(10, sizeof(int));
    if (b == NULL) {
        printf("tucalloc failed\n");
        tufree(a);
        return 1;
    }

    print_array("b after tucalloc (should be all zeros):", b, 10);

    b[0] = 111;
    b[9] = 999;
    print_array("b after writes:", b, 10);

    printf("\n=== turealloc growth test ===\n");
    int *bigger = (int *)turealloc(b, 20 * sizeof(int));
    if (bigger == NULL) {
        printf("turealloc failed\n");
        tufree(a);
        tufree(b);
        return 1;
    }

    /* Make sure the old values are still there after resizing */
    printf("bigger[0] = %d (should be 111)\n", bigger[0]);
    printf("bigger[9] = %d (should be 999)\n", bigger[9]);

    for (int i = 10; i < 20; i++) {
        bigger[i] = i;
    }
    print_array("bigger after expansion:", bigger, 20);

    printf("\n=== free and reuse test ===\n");
    tufree(a);

    int *c = (int *)tumalloc(5 * sizeof(int));
    if (c == NULL) {
        printf("reuse allocation failed\n");
        tufree(bigger);
        return 1;
    }

    for (int i = 0; i < 5; i++) {
        c[i] = 500 + i;
    }
    print_array("c contents:", c, 5);

    printf("\n=== coalescing setup test ===\n");
    int *x = (int *)tumalloc(4 * sizeof(int));
    int *y = (int *)tumalloc(4 * sizeof(int));
    int *z = (int *)tumalloc(4 * sizeof(int));

    if (x == NULL || y == NULL || z == NULL) {
        printf("coalescing setup alloc failed\n");
        tufree(c);
        tufree(bigger);
        if (x) tufree(x);
        if (y) tufree(y);
        if (z) tufree(z);
        return 1;
    }

    tufree(y);
    tufree(x);
    tufree(z);

    printf("Freed x, y, and z (neighboring free blocks should merge if coalescing works)\n");

    printf("\n=== double-free protection test ===\n");
    tufree(c);
    printf("First free of c done.\n");
    tufree(c);   /* This should trigger the double-free protection */
    printf("Second free of c attempted.\n");

    printf("\n=== invalid free handling test ===\n");
    int stack_value = 123;
    tufree(&stack_value);   /* Invalid free: this pointer was not returned by tumalloc */
    printf("Invalid free attempted on stack variable.\n");

    printf("\n=== cleanup ===\n");
    tufree(bigger);

    printf("Done.\n");
    return 0;
}
