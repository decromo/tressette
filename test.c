#include <stdio.h>
#include <unistd.h>
#include <assert.h>

// #define assert(some) some


int main() {
    printf("ci sono\n");
    sleep(1);
    printf("asdddd.... ");
    printf("aaaaaaaaaaaaaaaaaa ");
    fflush(stdout);
    assert(fprintf(stdin, "madonnaaaaa l'uomo al pestoooo\n") >= 0);
    char linea[1024] = { 0 };
    scanf("%s", linea);
    fprintf(stderr, "%s\n", linea);
    sleep(2);
    printf("che palle\n");
    fflush(stdin);
}