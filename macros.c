# include <stdio.h>
# include <stdlib.h>
# include <ctype.h>
# include <string.h>

/* macros */
# define srcFiletype ".as"
# define mFiletype ".am"
# define MAX_LINE_LEN 81
# define MACRO_START "macro"
# define MACRO_END "endm"

/* extern function prototypes */
extern char *getNextToken(char **, int);
extern char *concat(char *, char *);

/* typedefs */
typedef enum {REG, MACRO} Type;

typedef struct Macro {
    char *name;
    char *content;
    struct Macro *next;
} Macro;

/* function prototypes */
int addMacroToLL(Macro **, char *);
Macro *isMacroName(Macro *, char *);
void addLineToMacro(Macro *, char *);
char *getMacroName(char *);
Type checkLineType(char *);
void freeLL(Macro *);

int spread(char *filename) {
    /* spread - spreads macros
     * filename - the name of the file to spread (without filetype)*/

    FILE *fdIn, *fdOut; /* fdIn - file descriptor for filename.as, fdOut - file descriptor for filename.am */
    char *filenameIn = concat(filename, srcFiletype), *filenameOut = concat(filename, mFiletype), *temp;
    /* filenameIn - the filename of the input file (with .as)
     * filenameOut - the fileanme of the ouput file (with .am)
     * temp - temporary pointer */
    char *line, lineStart[MAX_LINE_LEN]; /* line - pointer to current position in line, lineStart - array containing the current line */
    Type state = REG; /* state - current state (macro or regular) */
    Macro *macros = NULL, *tempMacro; /* macros - head of macros linked list, tempMacro - temporary macro pointer */

    /* opening files - if there is an error, raise error */
    if (!(fdIn = fopen(filenameIn, "rt"))) {
        printf("UNABLE TO OPEN FILE %s\n", filenameIn);
        return -1;
    }
    if (!(fdOut = fopen(filenameOut, "wt"))) {
        printf("UNABLE TO CREATE FILE %s\n", filenameOut);
        return -1;
    }

    /* reading line by line, determining if it is a macro */
    while (!feof(fdIn)) {
        if (fgets(lineStart, MAX_LINE_LEN, fdIn) == NULL) { /* get the next line in input */
            if (feof(fdIn))
                break;
            printf("ERROR WHILE READING FROM FILE %s\n", filenameIn); /* if NULL was returned but we haven't reached the end of the file, reaise error */
            return -1;
        }

        line = lineStart;  /* set line to point to the start of the line */


        if (state == REG) { /* if we aren't in a macro */
            if (checkLineType(lineStart) == MACRO) { /* if the line initiates a macro, create a new macro node and give it the macro name */
                state = MACRO;
                temp = getMacroName(lineStart);
                addMacroToLL(&macros, temp); /* TODO - try to make generic method for getting macro name whilst using getNextToken */
            }
            else {
                temp = getNextToken(&line, 0);
                if ((tempMacro = isMacroName(macros, temp)) != NULL) { /* if the line is a call to a macro, then paste the macro's content to the output file */
                    fputs(tempMacro->content, fdOut);
                }
                else { /* we aren't calling a macro - therefore we just print the line to the output file */
                    fputs(lineStart, fdOut);
                }
                free(temp); /* freeing allocated memory */
            }
        }
        else { /* if we are in a macro */
            if (checkLineType(lineStart) == MACRO) { /* in case the macro has ended */
                state = REG;
            }
            else
                addLineToMacro(macros, lineStart); /* add the line to the macro's text */
        }

    }


    /* freeing memory */
    free(filenameIn);
    free(filenameOut);

    freeLL(macros);

    /* closing files */
    fclose(fdIn);
    fclose(fdOut);

    return 0;
}

int addMacroToLL(Macro **head, char *name) {
    /* addMacroToLL - add a new macro to the macro linked list
     * head - a pointer to the variable containing the head of the macro linked list
     * name - a string of the macro's name
     * returns failure or success (-1 or 0) */
    Macro *new = (Macro *) malloc(sizeof(Macro)); /* allocating memory */
    if (new == NULL) { /* if memory error, raise error */
        printf("ERROR WHILST ALLOCATING MEMORY\n");
        exit(0);
    }
    /* fill in the macro's details */
    new->name = name;
    new->next = *head;
    new->content = (char *) malloc(sizeof(char));
    if (!(new->content)) { /* handle memory allocation error */
        printf("ERROR WHILST ALLOCATING MEMORY\n");
        exit(0);
    }
    *(new->content) = '\0'; /* making the length 0 */
    *head = new; /* make the head be the new macro */
    return 0;
}

Macro *isMacroName(Macro *macros, char *name) {
    /*  isMacroName - checks whether or not a name is a macro's name
     *  macros - the head of the linked list containing all macros
     *  name - the name to check
     *  returns a pointer to the macro with the same name, or NULL if none found */
    while (macros != NULL) { /* go over all macros and compare the name */
        if (strcmp(macros->name, name) == 0) {
            return macros;
        }
        macros = macros->next;
    }
    return NULL;
}

void addLineToMacro(Macro *macro, char *line) {
    /* addLineToMacro - add a line to a macro's content
     * macro - the macro to add to
     * line - a string containing the line to add
     * returns nothing */
    int len = strlen(macro->content), i; /* len - the length of the existing content in the macro, i - temp counter */
    
    macro->content = (char *) realloc(macro->content, len + strlen(line) + 1); /* reallocating memory to fit the current size */
    if (!(macro->content)) {
        printf("ERROR WHILE ALLOCATING MEMORY\n");
        exit(0);
    }
    
    for (i = 0; (macro->content[len + i] = line[i]) != 0; i++) /* copying line's values into the end of content + the string terminator '\0' */
        ;
}

char *getMacroName(char *line) {
    /* getMacroName - gets the name of the macro in a given line
     * line - the line from which the name should be taken
     * returns a pointer to the name */

    /* getting past macro */
    free(getNextToken(&line, 0));
    /* getting macro name */
    return getNextToken(&line, 0);
}

Type checkLineType(char *line) {
    /* TODO - check this function. WHY 6??? */
    /* checkLineType - checks whether or not the line is the start of a macro or if it is regular
     * line - the line to check
     * returns the type of the line (macro line or regular line) */
    int i; /* i - counter */
    
    for(i = 0; isspace(line[i]); i++) /* skipping over spaces */
        ;
    if (i < strlen(line) - 4) { /* if there are at least 4 other characters left in the line (3 for the macro string + 1 for newline)*/
        /* if there's a match, return that the line is a macro line */
        if (strncmp((line + i), MACRO_START, sizeof(MACRO_START) - 1) == 0 || strncmp((line + i), MACRO_END, sizeof(MACRO_END) - 1) == 0)
            return MACRO;
    }

    return REG; /* else, return that it is a macro */
}

void freeLL(Macro *macros) {
    /* freeLL - free the macros linked list
     * macros - the head of the linked list to free
     * returns nothing*/
    Macro *temp;
    while (macros != NULL) { /* go over all macros and free all allocated memory */
        temp = macros;
        free(macros->name);
        free(macros->content);
        macros = macros->next;
        free(temp);
    }
}
