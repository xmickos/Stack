#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define POISON 0xC0DEDEAD
#define EPS 1e-3
#define DEBUG

#define SINGLE_BIT(bit) (1U << (unsigned) bit)

enum STACK_ERRORS {
    STK_NULLPTR = SINGLE_BIT(0),
    ZERO_CAP    = SINGLE_BIT(1),
    // STK_NULLPTR  = (1U << 0U),
    // ZERO_CAP     = (1U << 1U),
    DATA_NULLPTR = SINGLE_BIT(2),
    ZERO_SIZE    = SINGLE_BIT(3),
    WRONG_POISON = SINGLE_BIT(4),
    CAP_LESS_SIZE= SINGLE_BIT(5),
    HASH_ERROR   = SINGLE_BIT(6),
    LOGFILE_NULL = SINGLE_BIT(7)
}; // TODO: enum

// #define STK_NULLPTR 0b00000001
// #define ZERO_CAP 0b00000010
// #define DATA_NULLPTR 0b00000100
// #define ZERO_SIZE 0b00001000
// #define WRONG_POISON 0b00010000
// #define CAP_LESS_SIZE 0b01000000
// #define HASH_ERROR 0b10000000

//uint32_t

#define FIRST_CANARIE  0xDEAD2BAD
#define SECOND_CANARIE 0xDEFEC8ED

#ifdef DEBUG
#define DEBUG_ECHO(stream, message) do { fprintf(stream, "[%s, line: %d]" message "\n", __func__, __LINE__); } while(0)
#else
#define DEBUG_ECHO(stream, message) do { ; } while(0)
#endif


// сделать такой же define на случаи "pushed succesfully", который бы раскрывался в пустоту в случае не дебага.

#define VERIFICATION(condition, message, error_code)     if(condition){                                                 \
        fprintf(logfile, "[Verificator][%s, line: %d] " message , __func__, __LINE__);                                  \
        fprintf(logfile, message);                                                                                      \
        stk->errors = stk->errors | error_code;                                                                         \
    }                                                                                                                   \

#define VERIFICATION_CRITICAL(condition, message, error_code)     if(condition){                                        \
        fprintf(logfile, "[Verificator][%s, line: %d] " message , __func__, __LINE__);                                  \
        fprintf(logfile, message);                                                                                      \
        stk->errors = stk->errors | error_code;                                                                         \
        return stk->errors;                                                                                             \
    }                                                                                                                   \

#define STK_NULL_VERIFICATION(stk, logfile)     if(stk == nullptr){                                                     \
        fprintf(logfile, "[Error][%s, line: %d] stk pointer is nullptr!\n", __func__, __LINE__);                        \
        return STK_NULLPTR;                                                                                             \
    }                                                                                                                   \


typedef double Elem_t;
typedef unsigned long Canarie_t;

struct Stack{
    size_t capacity = 0;
    size_t size = 0;
    Elem_t *data = nullptr;
    unsigned long hash = 0;
    uint32_t errors = 0;
};


uint32_t Verificator(Stack *stk, FILE* logfile);

#define GENERAL_VERIFICATION(stk, logfile)     uint32_t verificator_output = Verificator(stk, logfile);                 \
    DEBUG_ECHO(stdout, "");                                                                                             \
    if(verificator_output != 0){                                                                                        \
        DEBUG_ECHO(logfile, "Verification error.\n");                                                                   \
        return verificator_output;                                                                                      \
    }                                                                                                                   \

unsigned long djb2hash(void *data_){
#ifndef NOHASH
    unsigned char *data = (unsigned char*)data_;
    unsigned long c = 0, hash = 5381;

    while((c = *data++)){
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
#else
    return 0;
#endif
}

#ifdef NOHASH
unsigned long duplicated_hash(void* data_){
    unsigned char *data = (unsigned char*)data_;
    unsigned long c, hash = 5381;

    while((c = *data++)){
        hash = ((hash << 5) * hash) + c;
    }

    return hash;
}
#endif

bool IsEqual(Elem_t a, Elem_t b){
    return fabs(a - b) < EPS;
}
// static int sdelay
static uint32_t StackResize(Stack *stk, FILE* logfile, int Direction){
    GENERAL_VERIFICATION(stk, logfile);

    size_t new_size = 0;
    // Direction: 1 <=> SizeUp, 0 <=> SizeDown

    if(Direction){
        if(stk->capacity < 8){
            new_size = 8;
        }
        else{
            new_size = stk->capacity * 2;
        }
    }
    else{
        if(stk->capacity > 16){
            new_size = stk->capacity / 2;
        }
        else{
            new_size = 16;
        }
    }

    fprintf(logfile, "[%s, line: %d] Resizing from %zu to %zu capacity\n", __func__, __LINE__, stk->capacity, new_size);

    stk->data = (Elem_t*)realloc(stk->data, new_size * sizeof(Elem_t));

    if(stk->data == nullptr){
        DEBUG_ECHO(logfile, "Failed to reallocate memory – not enought free space!\n");
        return DATA_NULLPTR;
    }
    stk->capacity = new_size;

    for(size_t i = stk->size; i < stk->capacity; i++){
        stk->data[i] = POISON;
    }

    stk->hash = djb2hash(stk);
    return 0;
}

bool NeedToResize(Stack *stk, int direction, FILE* logfile){
    GENERAL_VERIFICATION(stk, logfile)

    if(direction){
        return (((stk->capacity - stk->size) * 100) / stk->capacity) < 20;
    }
    else{
        return ((4 * stk->size - stk->capacity) * 100 / stk->capacity) < 20;
    }
}

uint32_t Verificator(Stack *stk, FILE* logfile){      // Сделать это define`ом

    // За один неверный верификатор программа ложится дойдя ошибками до мейна

    // Предполагается что stk->size и stk->capacity не должны быть равны нулю.

    // VERIFICATION_CRITICAL(stk == nullptr, "stk is nullptr!\n", STK_NULLPTR);
    STK_NULL_VERIFICATION(stk, logfile);
    VERIFICATION_CRITICAL(logfile == nullptr, "logfile is nullptr!\n", LOGFILE_NULL);
    VERIFICATION(stk->data == nullptr, "stk->data is nullptr!\n", DATA_NULLPTR);
    VERIFICATION(stk->capacity == 0, "stk->capacity is 0!\n", ZERO_CAP);
    VERIFICATION(stk->capacity < stk->size, "stk->capacity is less than stk->size!\n", CAP_LESS_SIZE);
    VERIFICATION(stk->size == 0 && !IsEqual(stk->data[0], POISON), "stk->size is 0!\n", ZERO_SIZE);

    unsigned long prev_hash = stk->hash, new_hash = 0;
    stk->hash = 0;
    new_hash = djb2hash((unsigned long*)stk);
    VERIFICATION(new_hash != prev_hash, "Hash error.\n", HASH_ERROR);
    stk->hash = new_hash;

    for(size_t i = stk->size; i < stk->capacity; i++){
        if(!IsEqual(stk->data[i],POISON)){
            fprintf(logfile, "[Verificator][%s, line: %d] stk->data[%zu] is not POISON!\n", __func__, __LINE__, i);
            stk->errors = stk->errors | WRONG_POISON;
        }
    }

    if(stk->errors == 0){
        DEBUG_ECHO(logfile, "No errors found!\n");
    }

    return stk->errors;
}

int StackPop(Stack *stk, FILE *logfile){
    GENERAL_VERIFICATION(stk, logfile);

    if(NeedToResize(stk, 0, logfile)){
        StackResize(stk, logfile, 0);
    }

    fprintf(logfile, "Popping %f\n", stk->data[stk->size - 1]);
    stk->data[stk->size - 1] = POISON;
    stk->size--;

    DEBUG_ECHO(logfile, "Popped succesfully!\n");

    return 0;
}

int StackDtor(Stack *stk, FILE* logfile){
    GENERAL_VERIFICATION(stk, logfile);

    free(stk->data);
    return 0;           // Проверки на stk == nullptr посредством макроса и stk->errors бессмысленны: stk->errors при
}                       // stk == nullptr не имеет смысла, нужно делать отдельный макрос и возвращать отдельно зарезирвированное
                        // значение.

int StackDump(Stack *stk, FILE *logfile){
    // DEBUG_ECHO(stdout, "");
    // Verificator(stk, logfile);

    // "Мы верим в логфайл", поэтому проверки на logfile == nullptr нет
    // если logfile == nullptr то exit(-1); ладно не надо exit(-1);

    fprintf(logfile, "[DUMP][file: %s, func: %s, line: %d]\nstk->size: %zu,\nstk->capacity: %zu.\n",
            __FILE__, __func__, __LINE__, stk->size, stk->capacity);

    if(stk == nullptr){
        fprintf(logfile, "[Error][%s, line: %d] stk pointer is nullptr!\n", __func__, __LINE__);
        return -1;
    }

    for(size_t i = 0; i < stk->capacity; i++){
        if(IsEqual(stk->data[i],POISON)){
            fprintf(logfile, "\tstk->data[%zu] = POISON\n", i);
        }
        else fprintf(logfile, "\tstk->data[%zu] = %f\n", i, stk->data[i]);
    }

    return 0;
}

int StackCtor(Stack *stk, size_t capacity, FILE* logfile){
    DEBUG_ECHO(stdout, "");
    STK_NULL_VERIFICATION(stk, logfile);
    VERIFICATION_CRITICAL(logfile == nullptr, "logfile is nullptr!\n", LOGFILE_NULL);

    stk->capacity = capacity;
    stk->size = 0;

    stk->data = (Elem_t*)calloc(capacity, sizeof(Elem_t));

    if(stk->data == nullptr){
        DEBUG_ECHO(logfile, "Failed to allocate memory – not enought free space!\n");
        return -1;
    }

    // *(stk->data - sizeof(Canarie_t)) = FIRST_CANARY;
    // *(stk->data + stk->capacity -1 + sizeof(Canarie_t)) = SECOND_CANARY;

    for(size_t i = 0; i < stk->capacity; i++){
        stk->data[i] = POISON;
    }

    stk->hash = djb2hash((unsigned long*)stk);

    return 0;
}

int StackPush(Stack *stk, FILE *logfile, Elem_t value){
    GENERAL_VERIFICATION(stk, logfile);

    if(NeedToResize(stk, 1, logfile)){
        if(!StackResize(stk, logfile, 1)) fprintf(logfile, "[%s, line: %d] Resized succesfully!\n", __func__, __LINE__);
    }

    stk->data[stk->size] = value;
    stk->size++;

    fprintf(logfile, "[%s, line: %d] Pushed %f succesfully!\n", __func__, __LINE__, value);

    return 0;
}

int main(){
    Stack stk = {};
    size_t capacity = 8;              //TBD
    FILE* logfile = fopen("logfile.txt", "w");      // Может, сделать переменную logfile глобальной ?

    //User input for capacity ?

    StackCtor(&stk, capacity, logfile);

    StackDump(&stk, logfile);

    for(int i = 0; i < 8; i++){
        StackPush(&stk, logfile, i);
    }

    StackDump(&stk, logfile);

    for(int i = 0; i < 6; i++){
        StackPop(&stk, logfile);
    }

    StackDump(&stk, logfile);

    printf("Hash: %lu\n", stk.hash);

    StackDtor(&stk, logfile);
    fclose(logfile);

    return 0;
}
