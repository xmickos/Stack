#include "stack_headers.hpp"

unsigned long djb2hash(void *_data, unsigned int len){
#ifndef NOHASH
    unsigned char *data = (unsigned char*)_data, *c = 0;
    unsigned long hash = 5381;
    size_t i = 0;

    while((c = data++) && i < len){
        hash = ((hash << 5) + hash) + (unsigned long)c;
        i++;
    }

    return hash;
#else
    return 0;
#endif
}

bool IsEqual(Elem_t a, Elem_t b){
    return fabs(a - b) < EPS;
}

static inline unsigned long djb2hash_safety(Stack* stk){
#ifndef NOHASH
    unsigned long hash = 5381;
    unsigned long c = 0;

    c = (unsigned long)stk->capacity;
    hash = ((hash << 5) + hash) + c;
    c = (unsigned long)stk->size;
    hash = ((hash << 5) + hash) + c;
    c = (unsigned long)stk->data;
    hash = ((hash << 5) + hash) + c;
    c = (unsigned long)stk->hash;
    hash = ((hash << 5) + hash) + c;
    c = (unsigned long)stk->errors;
    hash = ((hash << 5) + hash) + c;

    return hash;
#else
    return 0;
#endif
}

static inline uint32_t StackResize(Stack *stk, FILE* logfile, int Direction){
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
    stk->data--;
    stk->data = (Elem_t*)realloc(stk->data, new_size * sizeof(Elem_t) + 2 * sizeof(Canary_t));
    stk->data++;

    if(stk->data == nullptr){
        DEBUG_ECHO(logfile, "Failed to reallocate memory – not enought free space!\n");
        return DATA_NULLPTR;
    }
    stk->capacity = new_size;

    for(size_t i = stk->size; i < stk->capacity; i++){
        stk->data[i] = POISON;
    }
    stk->data[stk->capacity] = SECOND_CANARY;

    stk->hash = djb2hash_safety(stk);
    return 0;
}

static inline uint32_t NeedToResize(Stack *stk, int direction, FILE* logfile){
    GENERAL_VERIFICATION(stk, logfile)

    if(direction){
        return ( (((stk->capacity - stk->size) * 100) / stk->capacity) < 20 ) * RESIZE_YES;
    }
    else{
        return ( ((4 * stk->size - stk->capacity) * 100 / stk->capacity) < 20 ) * RESIZE_YES;
    }
}

static inline uint32_t Verificator(Stack *stk, FILE* logfile){      // Сделать это define`ом
    unsigned long prev_hash = 0, new_hash = 0;

    // За один неверный верификатор программа ложится дойдя ошибками до мейна

    // Предполагается что stk->size и stk->capacity не должны быть равны нулю.

    STK_NULL_VERIFICATION(stk, logfile);
    prev_hash = stk->hash;
    stk->hash = 0;
    new_hash = djb2hash_safety(stk);
    // printf("prev: %lu, new: %lu\n", prev_hash, new_hash);
    VERIFICATION_CRITICAL(new_hash != prev_hash, "Hash error.\n", HASH_ERROR, logfile);
    stk->hash = new_hash;
    VERIFICATION_CRITICAL(logfile == nullptr, "logfile is nullptr!\n", LOGFILE_NULL, stdout);
    VERIFICATION_CRITICAL(stk->data == nullptr, "stk->data is nullptr!\n", DATA_NULLPTR, logfile);
    VERIFICATION(!IsEqual(stk->data[-1], FIRST_CANARY), "First canary is dead!\n", FIRST_CAN_BAD);
    VERIFICATION(!IsEqual(stk->data[stk->capacity], SECOND_CANARY), "Second canary is dead!\n", SECOND_CAN_BAD);
    VERIFICATION(stk->capacity == 0, "stk->capacity is 0!\n", ZERO_CAP);
    VERIFICATION(stk->size == 0 && !IsEqual(stk->data[0], POISON), "stk->size is 0!\n", ZERO_SIZE);
    VERIFICATION(stk->capacity < stk->size, "stk->capacity is less than stk->size!\n", CAP_LESS_SIZE);

    size_t real_size = stk->size;
    for(; real_size < stk->capacity; real_size++){
        if(!IsEqual(stk->data[real_size],POISON)){
            fprintf(logfile, "[Verificator][%s, line: %d] stk->data[%zu] is not POISON!\n", __func__, __LINE__, real_size);
            stk->errors = stk->errors | WRONG_POISON;
            break;
        }
    }

    if(stk->errors == 0){
        #ifndef SILENT_DEBUG
            DEBUG_ECHO(logfile, "No errors found!\n");
        #endif
        return stk->errors;
    }

    StackDump(stk, logfile);

    return stk->errors;
}

uint32_t  StackPop(Stack *stk, FILE *logfile){
    GENERAL_VERIFICATION(stk, logfile);

    if(NeedToResize(stk, 0, logfile) == RESIZE_YES){
        StackResize(stk, logfile, 0);
    }

    fprintf(logfile, "Popping %f\n", stk->data[stk->size - 1]);
    fprintf(stdout, "Popping %f\n", stk->data[stk->size - 1]);
    stk->data[stk->size - 1] = POISON;
    stk->size--;

    DEBUG_ECHO(logfile, "Popped succesfully!\n");

    UPDATE_HASH(stk);

    return 0;
}

static inline uint32_t StackDump(Stack *stk, FILE *logfile){
    // GENERAL_VERIFICATION(stk, logfile);
    DEBUG_ECHO(stdout, "");

    // "Мы верим в логфайл", поэтому проверки на logfile == nullptr нет
    // если logfile == nullptr то exit(-1); ладно не надо exit(-1);

    fprintf(logfile, "[DUMP][file: %s, func: %s, line: %d]\nstk->size: %zu,\nstk->capacity: %zu.\n",
            __FILE__, __func__, __LINE__, stk->size, stk->capacity);


    for(size_t i = 0; i < stk->capacity + 2; i++){            // !! Предполагается, что канарейка имеет размер элемента стэка.
        ELEM_PRINT(FIRST_CANARY);
        ELEM_PRINT(SECOND_CANARY);
        ELEM_PRINT(POISON)
        else fprintf(logfile, "\tstk->data[%zu] = %f\n", i - 1, stk->data[i - 1]);
    }

    return 0;
}

uint32_t StackDtor(Stack *stk, FILE* logfile){
    GENERAL_VERIFICATION(stk, logfile);
    StackDump(stk, logfile);

    free(stk->data - 1);
    return 0;
}

    // Проверки на stk == nullptr посредством макроса и stk->errors бессмысленны: stk->errors при
    // stk == nullptr не имеет смысла, нужно делать отдельный макрос и возвращать отдельно зарезирвированное значение.

uint32_t StackCtor(Stack *stk, size_t capacity, FILE* logfile){
    DEBUG_ECHO(stdout, "");
    VERIFICATION_CRITICAL(logfile == nullptr, "logfile is nullptr!\n", LOGFILE_NULL, logfile);

    if(capacity > MAX_STACK_SIZE){
        printf("Provided capacity is too large!\nExiting...\n");
        return ZERO_CAP;
    }

    stk->capacity = capacity;
    stk->size = 0;

    // stk->data = (Elem_t*)calloc(capacity, sizeof(Elem_t));              // No canary


    stk->data = (Elem_t*)calloc(capacity * sizeof(Elem_t) + 2 * sizeof(Canary_t), sizeof(char));
    printf("allocated %lu bytes of memory\n", capacity * sizeof(Elem_t) + 2 * sizeof(Canary_t));
    stk->data++;

    /**
     * Пока вторая канарейка хэшируется, потом это можно исправить, добавив в функцию хэша длину хэшируемого блока памяти.
     */

    if(stk->data == nullptr){
        DEBUG_ECHO(logfile, "Failed to allocate memory – not enought free space!\n");
        return STK_NULLPTR;
    }

    for(size_t i = 0; i < stk->capacity; i++){
        stk->data[i] = POISON;
    }
    stk->data[-1] = FIRST_CANARY;
    stk->data[stk->capacity] = SECOND_CANARY;

    stk->hash = djb2hash_safety(stk);

    printf("CTOR: Initial hash is = %lu\n", stk->hash);

    StackDump(stk, logfile);

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

    UPDATE_HASH(stk);

    return 0;
}
