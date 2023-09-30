#include "stack_headers.hpp"

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

uint32_t NeedToResize(Stack *stk, int direction, FILE* logfile){
    GENERAL_VERIFICATION(stk, logfile)

    if(direction){
        return ( (((stk->capacity - stk->size) * 100) / stk->capacity) < 20 ) * RESIZE_YES;
    }
    else{
        return ( ((4 * stk->size - stk->capacity) * 100 / stk->capacity) < 20 ) * RESIZE_YES;
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
        return stk->errors;
    }

    StackDump(stk, logfile);

    return stk->errors;
}

uint32_t StackPop(Stack *stk, FILE *logfile){
    GENERAL_VERIFICATION(stk, logfile);

    if(NeedToResize(stk, 0, logfile) == RESIZE_YES){
        StackResize(stk, logfile, 0);
    }

    fprintf(logfile, "Popping %f\n", stk->data[stk->size - 1]);
    stk->data[stk->size - 1] = POISON;
    stk->size--;

    DEBUG_ECHO(logfile, "Popped succesfully!\n");

    return 0;
}

static uint32_t StackDump(Stack *stk, FILE *logfile){
    GENERAL_VERIFICATION(stk, logfile);

    // "Мы верим в логфайл", поэтому проверки на logfile == nullptr нет
    // если logfile == nullptr то exit(-1); ладно не надо exit(-1);

    fprintf(logfile, "[DUMP][file: %s, func: %s, line: %d]\nstk->size: %zu,\nstk->capacity: %zu.\n",
            __FILE__, __func__, __LINE__, stk->size, stk->capacity);


    for(size_t i = 0; i < stk->capacity; i++){
        if(IsEqual(stk->data[i],POISON)){
            fprintf(logfile, "\tstk->data[%zu] = POISON\n", i);
        }
        else fprintf(logfile, "\tstk->data[%zu] = %f\n", i, stk->data[i]);
    }




    return 0;
}

uint32_t StackDtor(Stack *stk, FILE* logfile){
    GENERAL_VERIFICATION(stk, logfile);
    StackDump(stk, logfile);

    free(stk->data);
    return 0;           // Проверки на stk == nullptr посредством макроса и stk->errors бессмысленны: stk->errors при
}                       // stk == nullptr не имеет смысла, нужно делать отдельный макрос и возвращать отдельно зарезирвированное
                        // значение.

uint32_t StackCtor(Stack *stk, size_t capacity, FILE* logfile){
    DEBUG_ECHO(stdout, "");
    STK_NULL_VERIFICATION(stk, logfile);
    VERIFICATION_CRITICAL(logfile == nullptr, "logfile is nullptr!\n", LOGFILE_NULL);

    stk->capacity = capacity;
    stk->size = 0;

    stk->data = (Elem_t*)calloc(capacity, sizeof(Elem_t));

    if(stk->data == nullptr){
        DEBUG_ECHO(logfile, "Failed to allocate memory – not enought free space!\n");
        return STK_NULLPTR;
    }

    // *(stk->data - sizeof(Canarie_t)) = FIRST_CANARY;
    // *(stk->data + stk->capacity -1 + sizeof(Canarie_t)) = SECOND_CANARY;

    for(size_t i = 0; i < stk->capacity; i++){
        stk->data[i] = POISON;
    }

    stk->hash = djb2hash((unsigned long*)stk);

    return 0;
}

uint32_t StackPush(Stack *stk, FILE *logfile, Elem_t value){
    GENERAL_VERIFICATION(stk, logfile);

    if(NeedToResize(stk, 1, logfile) == RESIZE_YES){
        if(!StackResize(stk, logfile, 1)) DEBUG_ECHO(logfile, "Resized successfully!\n");
    }

    stk->data[stk->size] = value;
    stk->size++;

    fprintf(logfile, "[%s, line: %d] Pushed %f successfully!\n", __func__, __LINE__, value);

    return 0;
}
