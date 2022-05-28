# include "utils.h"

int testRuns() {
    return 0;
}

/* WORD STRUCT FUNCTIONS =================================================================== */

void writeBlockToWord(Word *word, int data) {
    /* writeBlockToWord - words are divided into fields for ease of access, but whenever we want to write either a value or an opcode into the word, we
     * must use this function.
     * word - the word to write to
     * data - the data to write
     * returns nothing */
    int mask = 0, i;
    for (i = 0; i < WORD_SIZE - 4; i++) {
        /* making bitmask to mask the block we want to write to */
        mask <<= 1;
        mask += 1;
    }
    data = data & mask; /* masking data stay within the 16 byte limit */

    *((unsigned int *) word) &= ~0xFFFF;

    *((unsigned int *) word) += data;

}


void addWordToLL(Word **head, Word **tail, Word *word) {
    /* addWordToLL - adds word to tail of linked list
     * head - pointer to variable pointing to the head of the linked list
     * tail - pointer to variable pointing to the tail of the linked list
     * word - pointer to the word/s to add (the word might be the head of a LL to add to the tail)
     * returns nothing*/
    if (*tail) /* if there is a tail, make it point to the word. else, make the head point to the word*/
        (*tail)->next = word;
    else
        *head = word;


    while (word->next != NULL) /* move the tail to the end of the given linked list */
        word = word->next;
    *tail = word;
    
}

void addWordToLLByHead(Word *head, Word *w) {
    /* addWordToLLByHead - add a word to a linked list by head
     * head - the head of the linked list
     * w - the word to add
     * returns nothing*/
    while (head->next != NULL) /* get to the tail of the linked list */
        head = head->next;
    
    head->next = w;
}


void printWord(Word *w, FILE *fd, int start) {
    /* printWord - print a word to a given output, from a given start address
     * w - the word to print
     * fd - the output to print to
     * start - the starting address
     * returns nothing */

    int i, mask = 0xF; /* i - counter, mask - bitmask */
    fprintf(fd, "%04d\t", start); /* print the address with leading 0 */
    for (i = 4; i >= 0; i--) { /* print all 4-bit sections of the word, all tagged by letters A-E */
        /* bitwise magic (getting the i'th nibble and shifting it to be the first nibble, so it gets printed in its single hex value) */
        fprintf(fd, "%c%x%s", 'E' - i, ((*((unsigned int *) w)) & (mask << 4 * i)) >> (4 * i), i == 0 ? "\n" : "-");
    }
}

void printWordLL(Word *head, FILE *fd, int start) {
    /* printWordLL - print an entire word linked list to a given output, from a start address
     * head - the head of the linked list to print
     * fd - the output to print to
     * start - the starting address
     * returns nothing*/

    while (head != NULL) { /* go over all nodes in the list */
        printWord(head, fd, start++); /* print each node */
        head = head->next;
    }
}



/* END OF WORD STRUCT FUNCTIONS ============================================================ */

/* START OF LABEL STRUCT FUNCTIONS ======================================================== */

void freeLabelLL(Label *head) {
    /* freeLabelLL - free a label linked list
     * head - the head of the Label linked list
     * returns nothing */

    Label *p; /* p - pointer to previous node */
    while (head != NULL) { /* going over all nodes, freeing them and moving on */
        p = head;
        head = head->next;
        free(p);
    }
}


/* END OF LABEL STRUCT FUNCTIONS ========================================================== */


/* UTILITY FUNCTIONS ======================================================================= */

char *getNextToken(char **line, int isArg) {
    /* getNextToken - allocate memory and get next token in input, whilst ignoring spaces and tabs
     * isArg - says whether or not the token should be an argument or a command (1 for arg, 0 for command)
     * returns a pointer to the allocated memory containing the token*/
    int c, i = 1, j = 0; /* c - char to hold the next char in input, i - temp (starting at 1) */
    char *buf; /* buffer to hold the chars */
    while ((c = (*(line))[j++]) == ' ' || c == '\t') /* skip through all tabs and spaces */
        ; /* print everything out */
    if (c == '\0')
        return NULL;
    buf = (char *)malloc(sizeof(char)); /* allocate memory for one char and move the char read into it */
    if (!buf) {
        printf("ERROR WHILE ALLOCATING MEMORY\n");
        exit(0);
    }
    buf[0] = c;
    if (c == ',' || c == 0) { /* if char is ',' or 0, there is no need to keep reading */
        buf = (char *)realloc(buf, 2 * sizeof(char));
        if (!buf) {
            printf("ERROR WHILE ALLOCATING MEMORY\n");
            exit(0);
        }
        buf[1] = 0; /* add last character of command */
        return buf;
    }

    if (c == '\n') /* in case of newline, return the buffer */
        return buf;
    while ((c = (*(line))[j++]) != ' ' && c != '\t' && c != '\n' && c != ',' && c != 0) { /* get chars until char is space, tab, new(*(line)), comma, or 0 */
        buf = (char *)realloc(buf, ++i * sizeof(char));
        if (!buf) {
            printf("ERROR WHILE ALLOCATING MEMORY\n");
            exit(0);
        }
        buf[i - 1] = c; /* reallocate buffer memory to add room for one more char */
    }
    if (c != 0)
        ;
    if ((c == ' ' || c == '\t')) { /* in case token is arg and char is space or tab, keep reading  */
        while ((c = (*(line))[j++]) == ' ' || c == '\t') /* skip through all tabs and spaces */
            ;
        if (c != 0)
            ; /* print out last char */
        if (!isArg && c != '\n' && c != ',') { /* handle lines like 'stop' - there can be trailing spaces, which we don't want. */
            c = ' ';
            j -= 2;
        }
    }
    buf = (char *)realloc(buf, (++i + 1) * sizeof(char));
    if (!buf) {
        printf("ERROR WHILE ALLOCATING MEMORY\n");
        exit(0);
    }
    buf[i - 1] = c;
    buf[i] = 0; /* allocating more memory for last char and '\0'*/
    *line += j;
    return buf;
}

char *concat(char *str1, char *str2) {
    /* concat - concatenate two strings
     * str1 - the string to be at the start
     * str2 - the string to be added to str1
     * returns a pointer to the new string */

    int i, j; /* i - counter, j - counter */
    char *out; /* out - pointer to new string */
    /* copying str1 */
    out = (char *) malloc(strlen(str1) + strlen(str2) + 1);
    if (out == NULL) { /* in case of memory allocation error */
        printf("ERROR WHILST ALLOCATING MEMORY\n");
        exit(0);
    }
    for (i = 0; str1[i] != 0; i++) {
        out[i] = str1[i];
    }
    /* copying str2 */
    for (j = 0; str2[j] != 0; j++) {
        out[i + j] = str2[j];
    }
    /* adding string terminator */
    out[i + j] = 0;

    return out;
}
/* END OF UTILITY FUNCTIONS =============================================================================== */
