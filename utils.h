# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# define WORD_SIZE 20

typedef enum {CODE, DATA, ENTRY, EXTERN, STRING} Attribute;

typedef struct Word {
    /* Word struct represents word in memory (has the fields of a 2nd word of a command, but can get filled in with data) */
    unsigned int dstMtd:2;
    unsigned int dstReg:4;
    unsigned int srcMtd:2;
    unsigned int srcReg:4;
    unsigned int funct:4;
    unsigned int E:1;
    unsigned int R:1;
    unsigned int A:1;
    unsigned int unused:1;
    unsigned int :0; /* end of unsigned int used to represent word */
    struct Word *next;
} Word;

typedef struct Label {
    /* Label struct represents a label */
    char *name;
    int value;
    int base;
    int offset;
    Attribute attribute;
    int isEntry;
    struct Label *next;
} Label;

/* function prototypes */
char *getNextToken(char **, int);
void writeBlockToWord(Word *, int);
void addWordToLL(Word **, Word **, Word *);
void addWordToLLByHead(Word *, Word *);
void printWord(Word *, FILE *fd, int);
void printWordLL(Word *, FILE *fd, int);
void freeLabelLL(Label *);
char *getNextToken(char **, int);
char *concat(char *, char *);
