#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

extern int goCallbackHandler(int, int);

static int doAdd(int a, int b) {
    printf("doAdd in c\n");
    goCallbackHandler(a, b);
    //Sleep(5000);
    printf("doAdd in c\n");
    return goCallbackHandler(a, b);
}
