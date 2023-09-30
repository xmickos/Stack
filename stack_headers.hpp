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
    LOGFILE_NULL = SINGLE_BIT(7),
    RESIZE_YES   = SINGLE_BIT(8),
}; // TODO: enum

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
        stk->errors = stk->errors | error_code;                                                                         \
    }                                                                                                                   \

#define VERIFICATION_CRITICAL(condition, message, error_code)     if(condition){                                        \
        fprintf(stdout, "[Verificator][%s, line: %d] " message , __func__, __LINE__);                                   \
        exit(-1);                                                                                                       \
    }                                                                                                                   \
        // stk->errors = stk->errors | error_code;

#define STK_NULL_VERIFICATION(stk, logfile)     if(stk == nullptr){                                                     \
        fprintf(logfile, "[Error][%s, line: %d] stk pointer is nullptr!\n", __func__, __LINE__);                        \
        exit(-1);                                                                                                       \
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

unsigned long djb2hash(void *data_);

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


bool IsEqual(Elem_t a, Elem_t b);

static uint32_t StackResize(Stack *stk, FILE* logfile, int Direction);

uint32_t NeedToResize(Stack *stk, int direction, FILE* logfile);

uint32_t Verificator(Stack *stk, FILE* logfile);

uint32_t StackPop(Stack *stk, FILE *logfile);

static uint32_t StackDump(Stack *stk, FILE *logfile);

uint32_t StackDtor(Stack *stk, FILE* logfile);

uint32_t StackCtor(Stack *stk, size_t capacity, FILE* logfile);

uint32_t StackPush(Stack *stk, FILE *logfile, Elem_t value);


