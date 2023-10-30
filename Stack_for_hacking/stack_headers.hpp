#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define POISON 0xC0DEDEAD
#define BASIC_POISON 0xC67178F2
#define MAX_STACK_SIZE 8092
#define EPS 1e-3
#define DEBUG
#define SILENT_DEBUG

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
  FIRST_CAN_BAD  = SINGLE_BIT(9),
  SECOND_CAN_BAD = SINGLE_BIT(10)
}; // TODO: enum

//uint32_t

#define FIRST_CANARY  0xDEAD2BAD
#define SECOND_CANARY 0xDEFEC8ED

#ifdef DEBUG
#define DEBUG_ECHO(stream, message) do { fprintf(stream, "[%s, line: %d]" message "\n", __func__, __LINE__); } while(0)
#else
#define DEBUG_ECHO(stream, message) do { ; } while(0)
#endif

#define DECODE_ERROR(errorscode, BITMASK, message, logfile) do{                                                         \
    if((errorscode & BITMASK) != 0){                                                                                    \
        printf("\033[1;31mError\033[0m: " message "\n");                                                                \
        fprintf(logfile, "Error:" message "\n");                                                                        \
    }                                                                                                                   \
}while(0)                                                                                                               \

#define ERRORS_DECODING(errorscode, logfile) do{                                                                        \
    DECODE_ERROR(errorscode, STK_NULLPTR, "Stack pointer is nullptr!", logfile);                                        \
    DECODE_ERROR(errorscode, ZERO_CAP, "Stack capacity is zero!", logfile);                                             \
    DECODE_ERROR(errorscode, DATA_NULLPTR, "Stack data pointer is nullptr!", logfile);                                  \
    DECODE_ERROR(errorscode, ZERO_SIZE, "Stack size is zero!", logfile);                                                \
    DECODE_ERROR(errorscode, WRONG_POISON, "Wrong poison!", logfile);                                                   \
    DECODE_ERROR(errorscode, CAP_LESS_SIZE, "Stack capacity is smaller than stack size!", logfile);                     \
    DECODE_ERROR(errorscode, HASH_ERROR, "Stack hash is lost!", logfile);                                               \
    DECODE_ERROR(errorscode, LOGFILE_NULL, "Stack logfile pointer is nullptr!", logfile);                               \
    DECODE_ERROR(errorscode, FIRST_CAN_BAD, "Stack first canary is dead!", logfile);                                    \
    DECODE_ERROR(errorscode, SECOND_CAN_BAD, "Stack second canary is dead!", logfile);                                  \
}while(0)


#define ELEM_PRINT(symbol)         if(IsEqual(stk->data[i - 1], symbol)){                                               \
            fprintf(logfile, "\tstk->data[%zu] = " #symbol "\n", i - 1);                                                \
            continue;                                                                                                   \
        }                                                                                                               \


// сделать такой же define на случаи "pushed succesfully", который бы раскрывался в пустоту в случае не дебага.

#define VERIFICATION(condition, message, error_code)     if(condition){                                                 \
        fprintf(logfile, "[Verificator][%s, line: %d] " message , __func__, __LINE__);                                  \
        stk->errors = stk->errors | error_code;                                                                         \
    }                                                                                                                   \

#define VERIFICATION_CRITICAL(condition, message, error_code, logfile)     if(condition){                               \
        fprintf(logfile, "[FATAL ERROR][%s, line: %d] " message , __func__, __LINE__);                                  \
        return error_code;                                                                                              \
    }                                                                                                                   \
        // stk->errors = stk->errors | error_code;

#define STK_NULL_VERIFICATION(stk, logfile)     if(stk == nullptr){                                                     \
        fprintf(logfile, "[FATAL ERROR][%s, line: %d] stk pointer is nullptr!\n", __func__, __LINE__);                  \
        return STK_NULLPTR;                                                                                             \
    }                                                                                                                   \

#define UPDATE_HASH(stk) do{                                                                                            \
    stk->hash = 0;                                                                                                      \
    stk->hash = djb2hash_safety(stk);                                                                                   \
}while(0)


typedef double Elem_t;
typedef unsigned long  Canary_t;

struct Stack{
    size_t capacity = 0;
    size_t size = 0;
    Elem_t *data = nullptr;
    unsigned long hash = 0;
    uint32_t errors = 0;
};



#define GENERAL_VERIFICATION(stk, logfile)     uint32_t verificator_output = Verificator(stk, logfile);                 \
    if(verificator_output != 0){                                                                                        \
        if((verificator_output & LOGFILE_NULL) == 0){                                                                   \
            DEBUG_ECHO(logfile, " Verification error. Function failed.\n");                                             \
        }                                                                                                               \
        DEBUG_ECHO(stdout, " Verification error. Function failed.");                                                    \
        if((verificator_output & LOGFILE_NULL) != 0){                                                                   \
            ERRORS_DECODING(verificator_output, stdout);                                                                \
        }                                                                                                               \
        else{                                                                                                           \
            ERRORS_DECODING(verificator_output, logfile);                                                               \
        }                                                                                                               \
        return verificator_output;                                                                                      \
    }                                                                                                                   \

unsigned long djb2hash(void *data_, unsigned int len);

bool IsEqual(Elem_t a, Elem_t b);

static inline uint32_t StackResize(Stack *stk, FILE* logfile, int Direction);

static inline uint32_t NeedToResize(Stack *stk, int direction, FILE* logfile);

static inline uint32_t Verificator(Stack *stk, FILE* logfile);

uint32_t StackPop(Stack *stk, FILE *logfile);

static inline uint32_t StackDump(Stack *stk, FILE *logfile);

uint32_t StackDtor(Stack *stk, FILE* logfile);

uint32_t StackCtor(Stack *stk, size_t capacity, FILE* logfile);

uint32_t StackPush(Stack *stk, FILE *logfile, Elem_t value);

static inline unsigned long djb2hash_safety(Stack* stk);
