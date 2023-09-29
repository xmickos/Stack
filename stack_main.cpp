#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define POISON -43125
#define EPS 1e-3
#define STK_NULLPTR 0b00000001
#define ZERO_CAP 0b00000010
#define DATA_NULLPTR 0b00000100
#define ZERO_SIZE 0b00001000
#define WRONG_POISON 0b00010000
#define CAP_LESS_SIZE 0b01000000
#define HASH_ERROR 0b10000000
#define FIRST_CANARIE 0xDEAD2BAD
#define SECOND_CANARIE 0xDEFEC8ED

#define DEBUG_ECHO(); fprintf(stdout, "[%s, line: %d]\n", __func__, __LINE__);

#define VERIFICATION(condition, message, error_code);     if(condition){                                                \
        fprintf(logfile, "[Verificator][%s, line: %d] ", __func__, __LINE__);                                           \
        fprintf(logfile, message);                                                                                      \
        errors = errors | error_code;                                                                                   \
    }                                                                                                                   \
// Не нашёл пока способа сделать лучше :(


typedef double Elem_t;
typedef unsigned long Canarie_t;
char errors = (char)0;         // Чем лучше инициализировать?

struct Stack{
    size_t capacity = 0;
    size_t size = 0;
    Elem_t *data = nullptr;
    unsigned long hash = 0;
};

unsigned long djb2hash(void *data_){
#ifndef NOHASH
    unsigned char *data = (unsigned char*)data_;
    unsigned long c, hash = 5381;

    while((c = *data++)){
        hash = ((hash << 5) * hash) + c;
    }

    return hash;
#else
    return 0;
#endif
}

#ifdef NOHASH
unsigned long dublicated_hash(void* data_){
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

int StackResize(Stack *stk, FILE* logfile, int Direction){
    DEBUG_ECHO();

    // Direction: 1 <=> SizeUp, 0 <=> SizeDown
    // Верификатор не нужен? В эту функцию можно попасть только сперва пройдя верификатор.
    // А если direction == NULL?

    if(Direction){
        if(stk->capacity < 8){
            fprintf(logfile, "[%s, line: %d] Resizing up from %zu to 8 capacity\n", __func__, __LINE__, stk->capacity);

            if((stk->data = (Elem_t*)realloc(stk->data, 8 * sizeof(Elem_t))) == nullptr){
                fprintf(logfile, "[Error][%s, line: %d] Failed to reallocate memory – not enought free memory!\n", __func__, __LINE__);
                return -1;
            }                   // Эти if`ы можно засунуть в define, но там, похоже, запись сократится процентов на 20 максимум...
        }                       // Даже fprintf -> REALLOC_ERROR выглядит рудиментом, не?
        else{
            fprintf(logfile, "[%s, line: %d] Resizing up from %zu to %zu capacity\n", __func__,
                __LINE__, stk->capacity, 2 * stk->capacity);

            if((stk->data = (Elem_t*)realloc(stk->data, stk->capacity * 2 * sizeof(Elem_t))) == nullptr){
                fprintf(logfile, "[Error][%s, line: %d] Failed to reallocate memory – not enought free memory!\n", __func__, __LINE__);
                return -1;
            }
            stk->capacity = stk->capacity * 2;

            for(size_t i = stk->size; i < stk->capacity; i++){
                stk->data[i] = POISON;
            }
        }
    }
    else{
        if(stk->capacity > 16){
            if((stk->data = (Elem_t*)realloc(stk->data, (stk->capacity / 2) * sizeof(Elem_t))) == nullptr){
                printf("[Error][%s, line: %d] Failed to reallocate memory!\n", __func__, __LINE__);
                return -1;
            }

            stk->capacity = stk->capacity / 2;

            fprintf(logfile, "[%s, line: %d] Resized down from %zu to %zu capacity.\n", __func__,
                __LINE__, stk->capacity, stk->capacity / 2);
        }
    }
    stk->hash = djb2hash(stk);
    return 0;
}

bool NeedToResize(Stack *stk, int direction){
    DEBUG_ECHO();

    if(direction){
        return (((stk->capacity - stk->size) * 100) / stk->capacity) < 20;
    }
    else{
        return ((4 * stk->size - stk->capacity) * 100 / stk->capacity) < 20;
    }
}

char Verificator(Stack *stk, FILE* logfile){      // Сделать это define`ом
    DEBUG_ECHO();
    unsigned long prev_hash = stk->hash, new_hash;

    // Какие ещё могут быть ошибки? –Канарейки и несовпадение хэша
    // А что потом с ошибками делать-то? Ну, собрали ошибки в переменную ошибок, и?..

    // Предполагается что stk->size и stk->capacity не должны быть равны нулю.

    VERIFICATION(stk == nullptr, "stk is nullptr!\n", STK_NULLPTR);
    VERIFICATION(stk->capacity < stk->size, "stk->capacity is less than stk->size!\n", CAP_LESS_SIZE);
    VERIFICATION(stk->capacity == 0, "stk->capacity is 0!\n", ZERO_CAP);
    VERIFICATION(stk->size == 0 && !IsEqual(stk->data[0], POISON), "stk->size is 0!\n", ZERO_SIZE);
    VERIFICATION(stk->data == nullptr, "stk->data is nullptr!\n", DATA_NULLPTR);

    stk->hash = 0;
    new_hash = djb2hash((unsigned long*)stk);
    VERIFICATION(new_hash != prev_hash, "Hash error.\n", (char)HASH_ERROR);
    stk->hash = new_hash;

    // stk->capacity и stk->size не могут быть < 0 в силу определения типом size_t

    for(size_t i = stk->size; i < stk->capacity; i++){
        if(!IsEqual(stk->data[i],POISON)){
            fprintf(logfile, "[Verificator][%s, line: %d] stk->data[%zu] is not POISON!\n", __func__, __LINE__, i);
            errors = errors | WRONG_POISON;
        }
    }
    if(errors == (char)0){
        fprintf(logfile, "[Verificator][%s, line: %d] No errors finded!\n", __func__, __LINE__);
    }

    return errors;
}

int StackPop(Stack *stk, FILE *logfile){
    DEBUG_ECHO();

    if(Verificator(stk, logfile) == (char)0){

        if(NeedToResize(stk, 0)){
            StackResize(stk, logfile, 0);
        }

        fprintf(logfile, "Popping %f\n", stk->data[stk->size - 1]);
        stk->data[stk->size - 1] = POISON;
        stk->size--;
    }
    else{
        fprintf(logfile, "[%s, line: %d] Verification error.\n", __func__, __LINE__);
        return -1;
    }

    fprintf(logfile, "[%s, line:%d] Popped succesfully!\n", __func__, __LINE__);

    return 0;
}

int StackDtor(Stack *stk, FILE* logfile){
    if(stk == nullptr || stk->data == nullptr){
        fprintf(logfile, "[%s, line: %d] Verification error.\n", __func__, __LINE__);
        return -1;
    }
    free(stk->data);
    return 0;
}

int StackDump(Stack *stk, FILE *logfile){
    DEBUG_ECHO();
    Verificator(stk, logfile);

    // "Мы верим в логфайл", поэтому проверки на logfile == nullptr нет

    fprintf(logfile, "[DUMP][file: %s, func: %s, line: %d]\nstk->size: %zu,\nstk->capacity: %zu.\n",
            __FILE__, __func__, __LINE__, stk->size, stk->capacity);

    if(stk == nullptr){
        fprintf(logfile, "Stack pointer is nullptr!\n");
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
    DEBUG_ECHO();

    // проверка stk != nullptr
    if(stk == nullptr){
        fprintf(logfile, "[Error][%s, line: %d] stk pointer is nullptr!\n", __func__, __LINE__);
        return -1;
    }

    stk->capacity = capacity;
    if((stk->data = (Elem_t*)calloc(capacity, sizeof(Elem_t))) == nullptr){             // No Canarees yet.
        fprintf(logfile, "[Error][%s, line: %d]: Allocation error!\n", __func__, __LINE__);
        return -2;
    }

    for(size_t i = 0; i < stk->capacity; i++){
        stk->data[i] = POISON;
    }

    stk->hash = djb2hash((unsigned long*)stk);

    return 0;
}

int StackPush(Stack *stk, FILE *logfile, Elem_t value){
    DEBUG_ECHO();

    if(Verificator(stk, logfile) == (char)0){
        // Здесь сделать switch case с различными исходами в зав-ти от ошибки?

        if(NeedToResize(stk, 1)){
            if(!StackResize(stk, logfile, 1)) fprintf(logfile, "[%s, line: %d] Resized succesfully!\n", __func__, __LINE__);
        }

        stk->data[stk->size] = value;
        stk->size++;
    }

    fprintf(logfile, "[%s, line: %d] Pushed %f succesfully!\n", __func__, __LINE__, value);

    return 0;
}

int main(){
    Stack stk = {};
    size_t capacity = 8;              //TBD
    FILE* logfile = fopen("logfile.txt", "w");      // Может, сделать переменную logfile глобальной ?

    //User input for capacity ?

    StackCtor(&stk, capacity, logfile);        // Как лучше обрабатывать возвращаемые значения ?..

    printf("[HASH] %zu\n", stk.hash);

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

    fclose(logfile);
    StackDtor(&stk, logfile);

    return 0;
}
