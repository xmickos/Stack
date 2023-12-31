#include <stdio.h>
#include "stack_headers.hpp"

int main(){
    Stack stk = {};
    size_t capacity = 8;
    FILE* logfile = fopen("logfile.txt", "w");

    StackCtor(&stk, capacity, logfile);

    for(int i = 0; i < 64; i++){
        StackPush(&stk, logfile, i);
    }

    for(int i = 0; i < 64; i++){
        StackPop(&stk, logfile);
    }

    StackDtor(&stk, logfile);

    fclose(logfile);

    return 0;
}

/**
 * Пусть в main`e могут применяться только функции CTOR, PUSH, POP, DTOR, и только они могут положить программу с exit(-1) и
 * соответствующим сообщением в логфайл. StackDump вызывается только в случае возникновения ошибки.
 */


/**
 *  Пусть задача – ни при каких условиях (кроме совсем плачевных) не положить программу с ошибкой и спасти стек.
 *
 *
 *
 *
 */
