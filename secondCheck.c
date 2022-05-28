# include "utils.h"
# include "firstCheck.h"
# include "secondCheck.h"



int secondCheck(Word *codeWordsHead, Word *dataWordsHead, Label *labels, char *filename, int ICF, int DCF) {
    /* secondCheck - preforms the second stage of the assembler process
     * codeWordsHead - pointer to the head of the code words LL built in first stage
     * dataWordsHead - pointer to the head of the data words LL built in first stage
     * labels - pointer the head of the labels LL built in first stage
     * filename - string containing the name of the file to read from and generate (without filetype)
     * ICF - final instruction counter value
     * DCF - final data counter value
     * returns whether or not the program failed */
    FILE *fdIn, *fd, *fdExt; /* fdIn - file descriptor for input file, fd - file descriptor to be used to write to files, fdExt - file descriptor for ext file */
    char *fname, *fnameExtern, *line, *token, *tempChar;
    /* fname - the name of a file, fnameExtern - the name of the ext file, line - pointer to the current position in the current line, tempChar - temp pointer */
    char lineStart[MAX_LINE_LEN]; /* lineStart - array by the size of the max amount of characters in a line */
    int command, argCount, i, IC = 100, errors = 1, errorsFlag = 1, lineNum = 0, writtenToExt = 0, writtenToEntry = 0;
    /* command - index of the current command in the commands array, argCount - amount of arguments in line, i - counter
     * IC - instruction counter (starts at 100)
     * errors - whether or not the current line has an error
     * errorsFlag - whether or not the file has an error
     * lineNum - number of read line
     * writtenToExt - flag showing whether or not the ext file was written to
     * writtenToEntry - flag showing whether or not the ent file was written to*/
    AddressingMode mtd; /* mtd - indexing method of current line */
    Word *w = codeWordsHead; /* w - pointer to the head of the code words LL (will point to current word later) */
    Label *temp; /* temp - temporary label */
    Command commands[] = { /* commands - list of all commands */
        {"mov", 0, 0, 0xF, 0xE, 2},
        {"cmp", 1, 0, 0xF, 0xF, 2},
        {"add", 2, 10, 0xF, 0xE, 2},
        {"sub", 2, 11, 0xF, 0xE, 2},
        {"lea", 4, 0, 0x6, 0xE, 2},
        {"clr", 5, 10, 0x0, 0xE, 1},
        {"not", 5, 11, 0x0, 0xE, 1},
        {"inc", 5, 12, 0x0, 0xE, 1},
        {"dec", 5, 13, 0x0, 0xE, 1},
        {"jmp", 9, 10, 0x0, 0x6, 1},
        {"bne", 9, 11, 0x0, 0x6, 1},
        {"jsr", 9, 12, 0x0, 0x6, 1},
        {"red", 12, 0, 0x0, 0xE, 1},
        {"prn", 13, 0, 0x0, 0xF, 1},
        {"rts", 14, 0, 0x0, 0x0, 0},
        {"stop", 15, 0, 0x0, 0x0, 0}
    };

    fname = concat(filename, srcFiletype); /* getting the names of the .am and .ext files (with the filetype) */
    fnameExtern = concat(filename, extFiletype);

    /* opening files - if error arises, raise error*/
    if (!(fdIn = fopen(fname, "rt"))) {
        printf("UNABLE TO OPEN FILE %s\n", fname);
        return -1;
    }

    if (!(fdExt = fopen(fnameExtern, "wt"))) {
        printf("UNABLE TO OPEN FILE %s\n", fnameExtern);
        return -1;
    }


    /* MAIN LOOP ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * This is the main loop. In essence, it is very similar to the main loop in the first stage. We are going over the input
     * file, line by line, and filling in the blanks in the code words LL.
     * In case of data, string, or extern instructions, we skip them. In case of entry, we make sure that it isn't also extern,
     * but if not, we light the isEntry field in the given label.
     * In addition to filling in the values, whenever we see an extern label being used, we fill the location in the .ext file.
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    while (!feof(fdIn)) {
        /* getting input into line */
        errorsFlag = errorsFlag && errors;
        errors = 1;

        lineNum++;

        if (fgets(lineStart, MAX_LINE_LEN, fdIn) == NULL) {
            if (feof(fdIn))
                break;
            printf("ERROR WHILE READING FROM FILE %s\n", fname);
            return -1;
        }
        line = lineStart;

        token = getNextToken(&line, 0);

        if (*token == '\n' || *token == ';')
            continue; /* skip empty lines and comment lines */

        if (isLabel(token)) { /* if token is label, skip it */
            free(token);
            token = getNextToken(&line, 0);
        }

        if (isDataInstruction(token)) { /* skip data instructions */
            free(token);
            continue; /* TODO - maybe remove this later in place of else-if */
        }

        else if (isString(token)) { /* skip string instructions */
            free(token);
            continue;
        }

        else if (strlen(token) - 1 == strlen(externToken) && strncmp(token, externToken, strlen(externToken)) == 0) { /* skip extern instructions */
            free(token);
            continue;
        }

        /* ENTRY INSTRUCTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * In case of entry instruction, we make sure that it isn't an extern label as well, and if not, then we turn on the
         * isEntry field in the appropriate label. Raise errors if any are encountered
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


        else if (strlen(token) - 1 == strlen(entryToken) && strncmp(token, entryToken, strlen(entryToken)) == 0) { /* in case of entry instruction */
            free(token);
            token = getNextToken(&line, 1);
            token[strlen(token) - 1] = '\0'; /* make the '\n' character be the string terminator '\0' and shorten the size of the string to match */
            token = (char *) realloc(token, strlen(token) * sizeof(char));
            if (!token) { /* in case of memory allocation error */
                printf("ERROR WHILST ALLOCATING MEMORY\n");
                exit(0);
            }
            temp = getLabelByName(labels, token); /* get the label by name */
            if (temp == NULL) { /*  handle error in case of unrecognized label */
                printf("ERROR IN LINE %d - UNRECOGNIZED LABEL %s\n", lineNum, token);
                errors = 0;
            }
            else if (temp->attribute == EXTERN) { /*  handle error in case of extern & entry */
                printf("ERROR IN LINE %d - EXTERN LABEL CANNOT BE ENTRY\n", lineNum);
                errors = 0;
            }
            else
                temp->isEntry = 1; /* make the label entry */
            continue;
        }


        /* COMMAND ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * In case of command, we go over the arguments given and fill in the blanks in the code memory map.
         * If an extern label is used as an arg, we fill it in in the .ext file
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

        command = getCommandIndex(commands, token); /* get the commands index of the command */

        free(token);
        
        IC++;
        w = w->next; /* advance W and IC according to the amount of words the command has */

        if (commands[command].argCount != 0) {
            IC++;
            w = w->next;
        }

        argCount = 0;

        while ((token = getNextToken(&line, 1)) != NULL) { /* go over all arguments in line */
            argCount++;

            mtd = getAddressingMode(token); /* get the addressing mode of the arg */

            if (mtd == IMMEDIATE) { /* if argument is immediate, write the immediate value to the corresponding word in the code LL */
                writeImmediateToWord(w, token + 1, lineNum);
            }

            else if (mtd == INDEX || mtd == DIRECT) { /* if argument is index or direct */
                if (mtd == INDEX) { /* ignore everything after the end of the label */
                    tempChar = token;
                    while (*(tempChar++) != '[')
                        ;
                    *tempChar = 0;
                }

                token[strlen(token) - 1] = '\0'; /* make the '\n' character be the string terminator '\0' and shorten the size of the string to match */
                token = (char *) realloc(token, strlen(token) * sizeof(char));
                if (!token) { /* in case of memory allocation error */
                    printf("ERROR WHILST ALLOCATING MEMORY\n");
                    exit(0);
                }

                temp = getLabelByName(labels, token); /* getting the requested label */
                if (temp == NULL) { /* if the label is unrecognized, raise error */
                    printf("ERROR IN LINE %d - UNRECOGNIZED LABEL %s\n", lineNum, token);
                    free(token);
                    errors = 0;
                    break;
                }
                writeLabelToWords(w, temp); /* write the label data to the corresponding words in the code LL */
                if (temp->attribute == EXTERN) { /* if the label is extern, write the location it is in in the .ext file */
                    writtenToExt = 1; /* don't delete the file at the end */
                    fprintf(fdExt, "%s BASE %d\n", temp->name, IC);
                    fprintf(fdExt, "%s OFFSET %d\n\n", temp->name, IC + 1);
                }
            }

            for(i = 0; i < (1 + (mtd == INDEX || mtd == DIRECT)) * (mtd != REGISTER); i++) /* advance w and IC to go over the words we just went through */
                w = w->next;

            IC += (1 + (mtd == INDEX || mtd == DIRECT)) * (mtd != REGISTER);

            free(token);
        }
    }

    if (!errorsFlag || !writtenToExt)
        if(remove(fnameExtern))
            printf("ERROR - UNABLE TO DELETE FILE %s\n", fnameExtern); /* in case of error or nothing written, delete the extern file */


    /* freeing memory */
    free(fname);
    free(fnameExtern);

    /* closing files */
    fclose(fdIn);
    fclose(fdExt);

    /* WRITING DATA TO OUTPUT FILES */

    if (errorsFlag) {

        /* OBJ FILE */
        if (ICF > 100 || DCF > 0) {
            fname = concat(filename, objFiletype); 
            if (!(fd = fopen(fname, "wt"))) { /* in case we are unable to open the file, raise error */
                printf("UNABLE TO CREATE FILE %s\n", fname);
                freeWordLL(dataWordsHead);
                freeWordLL(codeWordsHead);
                freeLabelLL(labels);
                return -1;
            }
            fprintf(fd, "\t\t%d %d\n", ICF - 100, DCF); /* print the amount of code words, the amount of data words, and all of the code and data words to the file */
            printWordLL(codeWordsHead, fd, 100);
            printWordLL(dataWordsHead, fd, ICF);

            free(fname); /* close the file and free the name */
            fclose(fd);
        }

        /* ENTRY FILE */
        fname = concat(filename, entFiletype);
        if (!(fd = fopen(fname, "wt"))) { /* in case we are unable to open the file, raise error */
            printf("UNABLE TO CREATE FILE %s\n", fname);
            freeWordLL(dataWordsHead);
            freeWordLL(codeWordsHead);
            freeLabelLL(labels);
            return -1;
        }
        while (labels != NULL) { /* go over all labels and print the entry ones */
            if (labels->isEntry) {
                writtenToEntry = 1;
                fprintf(fd, "%s,%d,%d\n", labels->name, labels->base, labels->offset);
            }
            labels = labels->next;
        }

        fclose(fd);
        if (!writtenToEntry) {
            if(remove(fnameExtern))
                printf("ERROR - UNABLE TO DELETE FILE %s\n", fnameExtern); /* in case of nothing written, delete the extern file */
        }
        free(fname);

    }
    

    freeWordLL(dataWordsHead); /* free all LLs */
    freeWordLL(codeWordsHead);
    freeLabelLL(labels);


    return errorsFlag ? 0 : -1; /* return success or failure */
}



void freeWordLL(Word *head) {
    /* freeWordLL - free a word linked list
     * head - the head of the linked list to free
     * returns nothing */
    Word *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

void writeImmediateToWord(Word *w, char *arg, int lineNum) {
    /* writeImmediateToWord - write an immediate value to a word
     * w - the word to write to
     * arg - the argument to get the value from
     * lineNum - the line from which we are currently reading
     * returns nothing */
    int val, isNeg = 0;

    w->A = 1; /* make the A field 1 */

    if (*arg == '-') { /* in case of negative value, read the value as positive and then make negative with 2's complement */
        arg++;
        isNeg = 1;
    }
    val = atoi(arg);

    if (val > 32768) {
        printf("ERROR IN LINE %d - INTEGER CONSTANT IS TOO LARGE TO FIT IN 16 BITS\n", lineNum);
        return;
    }

    if (isNeg) { /* applying 2's complement if arg is negative */
        val = ~val;
        val += 1;
    }

    writeBlockToWord(w, val);
}

Label *getLabelByName(Label *head, char *name) {
    /* getLabelByName - gets the label that matches the given name
     * head - the head of the Label LL
     * name - the name to look for
     * returns a pointer to the Label node that is matching, if none found then NULL is returned */
    while (head != NULL) { /* go over all labels and if the names match, then return the pointer to the matching label */
        /*TODO - maybe bring this back
        if (head->attribute == EXTERN && strlen(head->name) == strlen(name) && (strncmp(head->name, name, strlen(name) - 1) == 0))
            return head;
        else if (head->attribute != EXTERN && strlen(head->name) - 1 == strlen(name) && (strncmp(head->name, name, strlen(head->name) - 2) == 0))
            return head;
            */
        if (strcmp(head->name, name) == 0)
            return head;
        head = head->next;
    }
    return NULL; /* if none found, return NULL */
}

void writeLabelToWords(Word *words, Label *label) {
    /* writeLabelToWords - writes label data to words
     * words - pointer to the bass and offset words (offset is words->next)
     * label - pointer to the label struct from which to take the data to write to the words
     * returns nothing*/
    /* writing base address */
    writeBlockToWord(words, label->base);
    if (label->attribute == EXTERN)
        words->E = 1;
    else
        words->R = 1;
    /* writing offset */
    words = words->next;
    writeBlockToWord(words, label->offset);
    if (label->attribute == EXTERN)
        words->E = 1;
    else
        words->R = 1;
}
