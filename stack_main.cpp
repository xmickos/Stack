#include <stdio.h>
#include "stack_headers.hpp"

int main(){
    Stack stk = {};
    size_t capacity = 8;
    FILE* logfile = fopen("logfile.txt", "w");

    //User input for capacity ?

    StackCtor(&stk, capacity, logfile);

    for(int i = 0; i < 8; i++){
        StackPush(&stk, logfile, i);
    }

    // stk.data = nullptr;

    // if(stk.data == nullptr)  fprintf(stdout, "\n\t\t\t\tHEEEERE WE ARE\n");

    for(int i = 0; i < 6; i++){
        StackPop(&stk, logfile);
    }

    printf("Hash: %lu\n", stk.hash);

    StackDtor(&stk, logfile);
    fclose(logfile);

    return 0;
}

/**
 * Пусть в main`e могут применяться только функции CTOR, PUSH, POP, DTOR, и только они могут положить программу с exit(-1) и
 * соответствующим сообщением в логфайл. StackDump вызывается только в случае возникновения ошибки.
 */
