# include <ctype.h>

/* macro definitions */
# define MAX_LINE_LEN 82 /* 81 characters as per instructions, +1 for \0 */
# define srcFiletype ".am" /* filetypes */
# define outFiletype ".obj"
# define commandCount 16 /* amount of commands known */
# define dataToken ".data" /* tokens needed for noticing instructions */
# define stringToken ".string"
# define entryToken ".entry"
# define externToken ".extern"


/* typedefs */
typedef enum {IMMEDIATE=0, DIRECT=1, INDEX=2, REGISTER=3} AddressingMode;
typedef enum {REG, LABEL} State;

typedef struct command {
    char *name;
    int opcode;
    int funct;
    int srcMtds;
    int dstMtds;
    int argCount;
} Command;

/* function prototypes */
int isLabel(char *);
int isDataInstruction(char *);
int isString(char *);
int checkLabel(Label *, char *, int, Command *, int);
int addLabelToLL(Label **, char *, int, Attribute);
Word *constructDataWord(Attribute, char *, int);
Word *constructCodeWord(Command *, int);
Word *constructEmptyWord();
int getRegValue(char *);
signed int getCommandIndex(Command *, char *);
AddressingMode getAddressingMode(char *);
int checkAddressingMode(char *, AddressingMode, Command *, int, int);
int firstCheck(char *);
