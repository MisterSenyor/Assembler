# include "utils.h"
# include "firstCheck.h"
# include "secondCheck.h"



int firstCheck(char *filename) {
    /* firstCheck - does the first stage of the assembling process - creating a memory map, with blank words instead of operand values
     * filename - the name of the file to assemble (without filetype) */
    FILE *fdIn; /* file descriptor for reading from the input file */
    char *fname, *line, *token, *name;
    /* fname - will hold the name of the file to read from (with filetype), line - will point to current reading location in line
     * token - will point to next token in line, name - will point to the name of a label found in the assembly code */
    char lineStart[MAX_LINE_LEN]; /* will contain lines from the .am file */
    int command, L, argCount, i, errors = 1, errorsFlag = 1, lineNum = 0;
    int DC = 0, IC = 100;
    int ICF, DCF;
    /* command - command index for commands array (initiated later)
     * L - length of currently built code word
     * errors - whether or not the current line has an error
     * errorsFlag - whether or not the file has an error
     * lineNum - number of read line
     * DC - data counter
     * IC - instruction counter
     * ICF - instruction counter final
     * DCF - data counter final */
    AddressingMode mtd; /* mtd - addressing mode of current line */
    Word *codeWordsHead = NULL, *codeWordsTail = NULL, *dataWordsHead = NULL, *dataWordsTail = NULL, *w, *commandStart, *argWord;
    /* codeWordsHead - head of code words linked list
     * codeWordsTail - tail of code words linked list
     * dataWordsHead - head of data words linked list
     * dataWordsTail - tail of data words linked list
     * w - temp variable to use when creating new words
     * commandStart - keeping track of the start of the current command's words (the head of the built linked list that is to be added to codeWordsHead)
     * argWord - temp variable to use when creating words for arguments*/
    Label *labels = NULL, *label = NULL; /* labels - head of labels linked list, label - temp variable to keep track of a Label node */
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
    State state = REG; /* state - flag to keep track of state (REG / LABEL) */

    fname = concat(filename, srcFiletype); /* concatenating the filename with the .am filetype */

    /* opening the file + returning error if unable to open file */
    if (!(fdIn = fopen(fname, "rt"))) {
        printf("UNABLE TO OPEN FILE %s\n", fname);
        return -1;
    }

    /* MAIN LOOP SECTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     * It works as following: we have errors and errorsFlag flags. errors is a flag for local errors in
     * the line, but it gets reset every line. errorsFlag is a flag for all errors (will be 0 after 1 error).
     * Each line, we sort it out - data, string, entry, extern, or code. We handle those cases accordingly.
     * We are managing the Word linked lists for the code and data words, as well as the Label LL, for the labels.
     * We are using the continue keyword as it aptly describes what we are doing - continuing to the next line.
     *
     * The reason that these are not separate functions is that they are all dependent on a variety of variables
     * that are local to the function, and therefore would be a huge waste to implement (aesthetic-wise, I've added
     * comments like these to make it more readable). I also think that doing such a division between functions
     * would make the code less beautiful - we'd be passing too many parameters to keep track of, which would make
     * reading the code a hassle.
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    while (!feof(fdIn)) { /* main loop for every line */
        errorsFlag = errorsFlag && errors; /* making errorsFlag keep track of errors while having errors be reset per-line */
        errors = 1;
        state = REG; /* state starts of as normal code, we'll check for symbols later */

        lineNum++; /* counting the line number */

        /* getting input into lineStart */
        if (fgets(lineStart, MAX_LINE_LEN, fdIn) == NULL) {
            if (feof(fdIn)) /* if we've reached the end of the file, break the loop */
                break;
            printf("ERROR WHILE READING FROM FILE %s\n", fname);
            free(fname); /* if unable to read from file, free + close everything and return -1 as failure */
            fclose(fdIn);
            return -1;
        }

        line = lineStart; /* setting line to the start of the line, from which it will go through the entire line */

        token = getNextToken(&line, 0); /* getting the next token - it can either be a label or a command */

        if (*token == '\n' || *token == ';')
            continue; /* skip empty lines and comment lines */

        if (isLabel(token)) { /* if token is label */
            state = LABEL;

            if ((token[strlen(token) - 1] != '\t' && token[strlen(token) - 1] != ' ') || token[strlen(token) - 2] != ':') {
                /* if there is illegal character after label declaration, raise error */
                printf("ERROR IN LINE %d - MISSING SPACE AFTER LABEL\n", lineNum);
                free(token);
                errors = 0;
                continue;
            }

            name = token;
            i = strlen(name); /* storing temp index in i */
            name[i - 2] = '\0'; /* make the ':' character be the string terminator '\0' and shorten the size of the string to match */
            name = (char *) realloc(name, (i - 1) * sizeof(char));
            if (!name) { /* in case of memory allocation error */
                printf("ERROR WHILST ALLOCATING MEMORY\n");
                exit(0);
            }
            token = getNextToken(&line, 0); /* saving the label name in name and getting the next token, which is the command */
        }

        if (token[strlen(token) - 1] == ',') {
            /* if command doesn't end in space, then raise error*/
            printf("ERROR IN LINE %d - ILLEGAL COMMA AFTER COMMAND\n", lineNum);
            free(token);
            errors = 0;
            continue;
        }

        /* DATA INSTRUCTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * In the data instruction section, we are gettin all the tokens left in the line, creating a data word
         * out of each one of them, and adding said word to the end of the data word LL.
         * In case of error, we free everything and skip to the next line
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

        if (isDataInstruction(token)) { /* in case the command is a data instruction */
            free(token);
            if (state == LABEL) { /* if there is a label in this line, check it and add to the label linked list */
                if (!(checkLabel(labels, name, 0, commands, lineNum)) || (!addLabelToLL(&labels, name, DC, DATA))) {
                    errors = 0;
                    free(name);
                    continue;
                }
            }
            while ((token = getNextToken(&line, 1)) != NULL) { /* get all arguments to the data instruction and build their words */
                if (*token == '\n') {
                    /* in case that the first character in the token is newline, then the previous operamd was the last one but ended in a comma */
                    printf("ERROR IN LINE %d - ILLEGAL COMMA AFTER LAST OPERAND\n", lineNum);
                    free(token);
                    errors = 0;
                    break;
                }
                if (token[strlen(token) - 1] != ',' && token[strlen(token) - 1] != '\n') { /* if token is not ending in either , or newline, then raise error */
                    printf("ERROR IN LINE %d - MISSING COMMA AFTER OPERAND\n", lineNum);
                    free(token);
                    errors = 0;
                    break;
                }
                w = constructDataWord(DATA, token, lineNum); /* create the data word according to the value in the token */
                if (w == NULL) { /* if null was returned, then the data word couldn't build, and therefore we raise an error */
                    free(token);
                    errors = 0;
                    break;
                }

                /* adding the new data word to the data linked list, advancing DC by 1 to count it, and freeing the token */
                addWordToLL(&dataWordsHead, &dataWordsTail, w);
                DC++;
                free(token);
            }
            continue; /* TODO - maybe remove this later in place of else-if */
        }

        /* STRING INSTRUCTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * In case of a string instruction, we make sure that everything is valid and create a LL of words,
         * each one representing a character in the string.
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

        else if (isString(token)) { /* in case the command is a string */
            if (state == LABEL) { /* if the line has a label, add it (if it's illegal, raise error) */
                if (!(checkLabel(labels, name, 0, commands, lineNum)) || (!addLabelToLL(&labels, name, DC, STRING))) {
                    errors = 0;
                    free(token);
                    free(name);
                    continue;
                }
            }
            free(token); /* we have no use for the string token anymore + getting the next token, which is supposed to be the str argument */
            token = getNextToken(&line, 1);
            if (token[strlen(token) - 1] != '\n') { /* in case the string argument has an illegal character after it */
                printf("ERROR IN LINE %d - ILLEGAL CHARACTER AFTER STRING ARGUMENT\n", lineNum);
                free(token);
                errors = 0;
                continue;
            }
            w = constructDataWord(STRING, token, lineNum); /* constructing data word LL out of the token */
            if (w == NULL) { /* if null was returned, then the data word couldn't build, and therefore we raise an error */
                free(token);
                errors = 0;
                continue;
            }
            addWordToLL(&dataWordsHead, &dataWordsTail, w); /* adding string data word LL to the main data words LL */
            DC += strlen(token) - 2; /* strlen(token) - 3 is amount of characters in the string, but we have to count the \0 word */
            continue;
        }

        /* ENTRY INSTRUCTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * In case of an entry instruction, we are skipping the line (but if there is a label declared in
         * the same line, we give out a warning)
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

        else if (strlen(token) - 1 == strlen(entryToken) && strncmp(token, entryToken, strlen(entryToken)) == 0) { /* in case of entry */
            free(token);
            if (state == LABEL) { /* if a label was declared in this line, ignore it and send out a warning */
                free(name);
                printf("WARNING IN LINE %d - LABEL SHOULD NOT BE DECLARED IN ENTRY LINE\n", lineNum);
            }
            continue;
        }

        /* EXTERN INSTRUCTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * In the case of an extern instruction, we are declaring the argument as a label.
         * In case of a label declared in the same line, we give out a warning
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

        else if (strlen(token) - 1 == strlen(externToken) && strncmp(token, externToken, strlen(externToken)) == 0) { /* in case of extern */
            free(token);
            if (state == LABEL) { /* if a label was declared in this line, ignore it and send out a warning */
                free(name);
                printf("WARNING IN LINE %d - LABEL SHOULD NOT BE DECLARED IN EXTERN LINE\n", lineNum);
            }
            name = getNextToken(&line, 1); /* the next token is the name of the extern label */
            if (name[strlen(name) - 1] != '\n') { /* if there is an illegal character after the name declaration, raise error */
                printf("ERROR IN LINE %d - ILLEGAL CHARACTER '%c' AFTER EXTERN LABEL\n", lineNum, name[strlen(name) - 1]);
                free(name);
                errors = 0;
                continue;
            }
            name[strlen(name) - 1] = '\0'; /* make the '\n' character be the string terminator '\0' and shorten the size of the string to match */
            name = (char *) realloc(name, strlen(name)* sizeof(char));
            if (!name) { /* in case of memory allocation error */
                printf("ERROR WHILST ALLOCATING MEMORY\n");
                exit(0);
            }
            if (!(checkLabel(labels, name, 1, commands, lineNum)) || (!addLabelToLL(&labels, name, 0, EXTERN))) {
                /* check the argument's validity as a label and if it's valid, create a label node and add it to the LL. In case of error, raise error */
                errors = 0;
                free(name);
                continue;
            }
            continue;
        }

        /* COMMAND ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * In case of a command, we get the command's index in the commands array (identify the command).
         * then, we go over all arguments passed to the command, and if they are valid, we add empty
         * words to the code words LL that will be filled in during the second stage. If they are invalid,
         * we raise an error and move on to the next line.
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

        /* If none of the declarations were activated, then this is a command */
        if (state == LABEL) {
            if (!checkLabel(labels, name, 0, commands, lineNum) || !addLabelToLL(&labels, name, IC, CODE)) {
                /* check the label's validity and if it's valid, create a label node and add it to the LL. In case of error, raise error */
                free(token);
                free(name);
                errors = 0;
                continue;
            }
        }

        if ((command = getCommandIndex(commands, token)) == -1) { /* in case of unrecognized command, raise error */
            printf("ERROR IN LINE %d - UNKNOWN COMMAND %s\n", lineNum, token);
            free(token);
            errors = 0;
            continue;
        }



        commandStart = w = constructCodeWord(&commands[command], 1); /* build the first data word that always exists + write the opcode of the command to it */
        writeBlockToWord(w, 1 << commands[command].opcode);

        L = 1; /* always 1 + another 1 only if there are args */

        if (commands[command].argCount != 0) { /* if the command has operands, build another code word for the second word */
            /* TODO - don't think we need this */
            if (token[strlen(token) - 1] == '\n') { /* in case of end of line despite there being arguments, raise error */
                printf("ERROR IN LINE %d - MISSING OPERAND AFTER COMMAND\n", lineNum);
                free(token);
                errors = 0;
                continue;
            }
            L += 1;
            w = constructCodeWord(&commands[command], 1);
        }

        free(token);

        argCount = 0; /* start the argCount at 0 */

        /* GETTING ARGS */

        while ((token = getNextToken(&line, 1)) != NULL && errors) {
            argCount++;
            if (token[strlen(token) - 1] != ',' && token[strlen(token) - 1] != '\n' && argCount < commands[command].argCount) { /* in case of missing comma, raise error */
                printf("ERROR IN LINE %d - MISSING COMMA BETWEEN OPERANDS\n", lineNum);
                free(token);
                errors = 0;
                break;
            }
            if (*token == ',') {
                printf("ERROR IN LINE %d - REPEATING COMMAS WITHOUT ARGUMENT BETWEEN THEM\n", lineNum);
                free(token);
                errors = 0;
                break;
            }
            if (token[strlen(token) - 1] != '\n' && argCount == commands[command].argCount) { /* in case of illegal character after last argument, raise error */
                printf("ERROR IN LINE %d - ILLEGAL CHARACTER AFTER LAST ARGUMENT IN FUNCTION\n", lineNum);
                free(token);
                errors = 0;
                break;
            }

            
            mtd = getAddressingMode(token); /* getting the addressing mode of the argument */
            /* if the addressing mode is illegal in the command (since the first argument is always the src arg if there are other args), raise error */
            if (!checkAddressingMode(token, mtd, &commands[command], argCount == 1 && commands[command].argCount != 1, lineNum)) { /* TODO - make sure that the first arg is always src arg */
                free(token);
                errors = 0;
                break;
            }

            L += (1 + (mtd == INDEX || mtd == DIRECT)) * (mtd != REGISTER);
            /* if the addressing mode is either index or direct, then we need 2 words to represent it. If not, then we need 1 word. If the method is
             * register, then we need no words (so we use the boolean expression to make it 0 when it's a register and not change it when it's something else) */

            for (i = 0; i < (1 + (mtd == INDEX || mtd == DIRECT)) * (mtd != REGISTER); i++) {
                /* construct empty words for the amount added to L earlier + add it to the command LL that will be added to the commandWords LL */
                argWord = constructEmptyWord();
                addWordToLLByHead(commandStart, argWord);
            }

            /* filling in the 2nd word of the command (addressing modes, registers, etc.) */
            if (argCount == 1) {
                if (commands[command].argCount == 1) { /* if the command gets only 1 arg, then it is always a dst arg */
                    if (mtd == REGISTER || mtd == INDEX) {
                        w->dstReg = getRegValue(token);
                    }
                    w->dstMtd = mtd;
                }
                else { /* if the command gets more than 1 arg, then the first is always a src arg */
                    if (mtd == REGISTER || mtd == INDEX) {
                        w->srcReg = getRegValue(token);
                    }
                    w->srcMtd = mtd;
                }
            }

            else { /* if the arg is not the 1st arg, then it must be a dst arg */
                if (mtd == REGISTER || mtd == INDEX)
                    w->dstReg = getRegValue(token);
                w->dstMtd = mtd;
            }

            free(token);
        }
        if (errors && argCount < commands[command].argCount) {
            printf("ERROR IN LINE %d - NOT ENOUGH ARGUMENTS PASSED TO COMMAND\n", lineNum);
            errors = 0;
            free(commandStart);
            continue;
        }

        if (w != commandStart) {
        w->next = commandStart->next; /* placing 2nd word after command start */
        commandStart->next = w;
        }

        /* adding command LL (first and optionally second words of command + words for operands) to code words LL*/
        addWordToLL(&codeWordsHead, &codeWordsTail, commandStart);

        IC += L;
    }


    ICF = IC; /* saving final IC and DC values */
    DCF = DC;

    /* adding ICF to DC-value-labels */
    for (label = labels; label != NULL; label = label->next) {
        if (label->attribute != CODE && label->attribute != EXTERN) {
            label->value += ICF;
            label->offset = label->value % 16;
            label->base = label->value - label->offset;
        }
    }

    /* freeing memory */
    free(fname);

    /* closing files */
    fclose(fdIn);

    if (errorsFlag) /* run the second check only if there were no errors */
        errors = secondCheck(codeWordsHead, dataWordsHead, labels, filename, ICF, DCF); /* storing temp value in errors */

    return errorsFlag ? errors : -1;
}


signed int getCommandIndex(Command *commands, char *command) {
    /* getCommandIndex - gets the index of the command name in the commands array
     * commands - the array of Command structs, containing the data for all commands
     * command - a string containing the name to loko for in commands
     * returns the index of the command in the command array, -1 if it isn't there */
    int i;

    for (i = 0; i < commandCount; i++) { /* loop over all commands */
        /* with the way getNextToken works, command will have 1 trailing character to show what ended after it. This character should
         * be ignored, so we use strncmp */
        if (strlen(command) == (strlen(commands[i].name) + 1) && strncmp(command, commands[i].name, strlen(command) - 1) == 0) {
            return i;
        }
    }

    return -1;
}

int isLabel(char *command) {
    /* isLabel - checks whether or not a command is a label
     * command - the command to check for a label in
     * returns whether or not the command is a label (0 or 1) */
    int i; /* i - counter */
    for (i = 0; i < strlen(command); i++) { /* label characters will be checked later */
        if (command[i] == ':')
            return 1;
    }
    return 0;
}

int isDataInstruction(char *command) {
    /*  isDataInstruction - checks whether or not a command is a data instruction
     *  command - the command to check for a data instruction in
     *  return whether or not the command is a data instruction (0 or 1) */

    /* with the way getNextToken works, command will have 1 trailing character to show what ended after it. This character should
     * be ignored, so we use strncmp */
    return strlen(command) - 1 == strlen(dataToken) && strncmp(command, dataToken, strlen(dataToken)) == 0;
}

int checkLabel(Label *labels, char *label, int isExtern, Command *commands, int lineNum) {
    /* checkLabel - checks whether or not a label is valid
     * labels - a list of all labels
     * label - the name of the label to check
     * isExtern - whether or not the label is an extern one
     * commands - a pointer to the array of all commands used by the machine
     * returns whether or not the label is valid (0 or 1) */
    int i = 0; /* i - counter */
    if (!isalpha(*label)) /* if first character in label isn't an alphabetic character, then it is invalid */
        return 0;
    while (label[i]) { /* if other characters in label aren't alphanumeric, then it is invalid */
        if (!isalpha(label[i]) && !isdigit(label[i]))
            break;
        i++;
    }

    /* in case the label is too long */
    if (strlen(label) > 31) {
        printf("ERROR IN LINE %d - LABEL EXCEEDS 31 CHARACTER LIMIT\n", lineNum);
    }

    if (label[i] != '\0') {
        /* if the label is extern, then it will be at the end of the line and therefore will have a trailing newline (due to getNextToken)
         * else, the lavel should end in :. if not, raise error*/
        printf("ERROR IN LINE %d - LABEL HAS ILLEGAL CHARACTER %c\n", lineNum, label[i]);
        return 0;
    }


    while (labels != NULL) { /* checks for other labels with the same name. If there are any, raises error */
        if (strcmp(label, labels->name) == 0) { /* TODO - remove comment */
        /*if (strcmp(label, labels->name) == 0 || (strlen(label) == strlen(labels->name) - 1 && strncmp(label, labels->name, strlen(labels->name) - 1) == 0)
                || (strlen(labels->name) == strlen(labels->name) + 1 && strncmp(label, labels->name, strlen(label) - 1) == 0)) { */
            printf("ERROR IN LINE %d - LABEL %s DECLARED TWICE\n", lineNum, label);
            return 0;
        }
        labels = labels->next;
    }
    
    for (i = 0; i < commandCount; i++) { /* in case label is saved word */
        if (strcmp(label, commands[i].name) == 0) {
        /*if ((isExtern && strlen(label) == strlen(commands[i].name) + 1 && strncmp(label, commands[i].name, strlen(commands[i].name)) == 0) ||
                (!isExtern && strlen(label) == strlen(commands[i].name) + 2 && strncmp(label, commands[i].name, strlen(commands[i].name)) == 0)) { */
            printf("ERROR IN LINE %d - LABEL %s CANNOT BE SAVED SEQUENCE\n", lineNum, label);
            return 0;
        }
    }
    if (*label == 'r' && isdigit(label[1]) && ((!isdigit(label[2]) && label[2] == '\0') || (isdigit(label[2]) && label[1] == '1' && label[2] < '6'))) {
        printf("ERROR IN LINE %d - LABEL %s CANNOT BE SAVED SEQUENCE\n", lineNum, label);
        return 0;
    }
    return 1;
}

int addLabelToLL(Label **labels, char *name, int address, Attribute attr) {
    /*  addLabelToLL - adds a label to a label LL
     *  labels - the address of the a variable pointing to the head of the labels LL
     *  name - the name of the label to add
     *  address - the address value of the label
     *  attr - the attribute the label has (code, data, string, extern)
     *  returns success (1) or failure (0) */
    Label *new = (Label *) malloc(sizeof(Label)); /* allocate enough memory */
    if (!new) { /* in case of memory error, raise error */
        printf("ERROR WHILST ALLOCATING MEMORY\n");
        exit(0);
    }

    new->name = name; /* fill in label values - base & offset according to value, name, attribute */
    new->value = address;
    new->base = address - address % 16;
    new->offset = address % 16;
    new->attribute = attr;
    new->isEntry = 0; /* isEntry will be filled in during second stage */
    new->next = *labels; /* make this label the new head */

    *labels = new; /* make the variable containing the head point to the new head */

    return 1;
}

Word *constructDataWord(Attribute attr, char *token, int lineNum) {
    /* constructDataWord - constructs data word/s according to the attribute and token
     * attr - the attribute according to which we construct the data word/s
     * token - the token from which we get the word/s data
     * returns a pointer to the word/s, NULL if failed */
    int val, i; /* val - the value contained in the token, i - counter */
    Word *w = constructEmptyWord(), *temp, *prev; /* w - head word, temp - temp variable, prev - temp variable to keep track of previous node */

    /* data word will be A = 1 */
    w->A = 1;

    switch (attr) {
        case DATA: /* in case of data, get the integer value of the token */
            for (i = 0; i < strlen(token) - 1; i++) {
                if ((!isdigit(token[i]) && i > 0) || (i == 0 && token[i] != '-' && token[i] != '+' && !isdigit(token[i]))) {
                    token[strlen(token) - 1] = '\0'; /* making the arg printable */
                    printf("ERROR IN LINE %d - DATA ARGUMENT %s HAS NONDIGIT CHARACTER\n", lineNum, token);
                    free(w);
                    return NULL;
                }
            }
            val = atoi(token);
            break;
        case STRING: /* in case of string, make w the head of a LL containing all of the words needed to represent the string */
            if (token[strlen(token) - 2] != '"' || *token != '"') { /* if argument isn't surrounded by double quotes, then it is illegal - raise error */
                printf("ERROR IN LINE %d - STRING ARGUMENT DOESN'T HAVE BRACES AROUND IT\n", lineNum);
                free(w);
                return NULL;
            }
            val = token[i = 1]; /* the value of the first word will be the value of the first character inside the string */
            prev = w;
            for (i++; i < strlen(token) - 2; i++) { /* go over all characters inside the string, creating new data words with the characters as their value */
                temp = constructEmptyWord();
                writeBlockToWord(temp, token[i]); /* write ascii value to word and add it to data LL to be returned */
                temp->A = 1;
                prev->next = temp;
                prev = temp;
            }
            temp = constructEmptyWord(); /* add an emtpy word for a string terminator ('\0') */
            temp->A = 1;
            prev->next = temp;
            break;
        default:
            break; /* all other cases of data don't call this function */
    }
    writeBlockToWord(w, val);

    return w;
}

int isString(char *command) {
    /* isString - checks wheter or not a command is a string command
     * command - the command to check
     * returns whether or not the command is a string (1 or 0) */
    return strlen(command) - 1 == strlen(stringToken) && strncmp(command, stringToken, strlen(stringToken)) == 0;
}

Word *constructCodeWord(Command *command, int flags) { /*  TODO - check what flags is for */
    /* constructCodeWord - constructs a code word from the Command specified
     * command - the Command struct to get the data from
     * flags - ???
     * returns a pointer to the code word constructed */
    Word *w = constructEmptyWord(); /* constructs an empty word */

    if (w == NULL) {
        printf("ERROR WHILST ALLOCATING MEMORY");
        exit(0);
    }
    
    /* setting the word's attributes according to the command's attributes (will always be A = 1) */
    w->A = 1;
    w->R = w->E = 0;
    writeBlockToWord(w, 0);
    w->next = NULL;


    w->funct = command->funct;

    return w;
}


Word *constructEmptyWord() {
    /* constructEmptyWord - constructs an empty word struct
     * returns a pointer to an empty word struct */
    Word *w = (Word *) malloc(sizeof(Word)); /* allocate memory */

    if (!w) { /* handle allocation error */
        printf("ERROR WHILST ALLOCATING MEMORY");
        exit(0);
    }
    
    /* make all fields 0 */
    w->unused = w->A = w->R = w->E = 0;
    writeBlockToWord(w, 0);
    w->next = NULL;

    return w;
}

int getRegValue(char *token) {
    /* getRegValue - gets the number of a register from a token
     * token - the token from which to get the register number
     * returns the number of the retister*/
    int i = 0;
    if (*token != 'r') {
        for (; token[i] != '['; i++) /* jump to the part where the register starts in case of addressing mode */
            ;
        i++;
    }
    return atoi(token + i + 1); /* return the integer value of the number after the 'r' */
}

AddressingMode getAddressingMode(char *arg) {
    /* getAddressingMode - get the addressing mode of an argument
     * arg - the argument to get the addressing mode of
     * returns the addressing mode of the argument */
    if (*arg == '#')
        return IMMEDIATE; /* if the arg starts off with #, then it must be an immediate */
    
    else if (*arg == 'r') /* if the arg start off with 'r', then it must be a register */
        return REGISTER;

    while (*arg != 0) { /* looping until the end of string. If we see '[', then we stop the loop */
        if (*++arg == '[')
            break;
    }
    
    return *arg == 0 ? DIRECT : INDEX; /* if the loop stopped, then return index. else, return direct */
}

int checkAddressingMode(char *arg, AddressingMode mtd, Command *command, int isSrc, int lineNum) {
    /* checkAddressingMode - checks the addressing mode relative to the command and argument it is for
     * arg - the argument in question
     * mtd - the method in question
     * command - the command to compare to
     * isSrc - whether or not the argument is for a source or a destination
     * returns whether or not the method is valid (1 or 0) */
    int i; /* counter */

    /* if the argument is a src argument and the bit corresponding to mtd isn't lit in command->srcMtds OR
     * if the arguments is a dst argument and the bit corresponding to mtd isn't lit in command->dstMtds
     * then it is an error */
    if ((isSrc && ((1 << mtd) & command->srcMtds) == 0) || (!isSrc && ((1 << mtd) & command->dstMtds) == 0)) {
        printf("ERROR IN LINE %d - ARGUMENT ADRESSING MODE IS ILLEGAL IN COMMAND %s\n", lineNum, command->name);
        return 0;
    }

    /* checking the argument according to the addressing mode */

    switch (mtd) {
        case IMMEDIATE: /* in case of an immediate */
            if (!isdigit(arg[1]) && arg[1] != '-' && arg[1] != '+') { /* if the first character in the argument isn't a digit or a - sign, raise error */
                printf("ERROR IN LINE %d - ILLEGAL CHARACTER IN IMMEDIATE ARGUMENT\n", lineNum);
                return 0;
            }

            for (i = 2; i < strlen(arg) - 1; i++) { /* if all other characters in the argument aren't digits, raise error */
                if (!isdigit(arg[i])) {
                    printf("ERROR IN LINE %d - ILLEGAL CHARACTER IN IMMEDIATE ARGUMENT\n", lineNum);
                    return 0;
                }
            }

            break;

        case DIRECT: /* in case of a direct method */
            /* the argument must be a label, and therefore should start with an alphabetic character and then have
             * only alphanumeric characters. If not, raise error*/
            if (!isalpha(*arg)) {
                printf("ERROR IN LINE %d - ILLEGAL CHARACTER IN LABEL ARGUMENT\n", lineNum);
                return 0;
            }
            for (i = 1; i < strlen(arg) - 1; i++) {
                if (!isalpha(arg[i]) && !isdigit(arg[i])) {
                    printf("ERROR IN LINE %d - ILLEGAL CHARACTER IN LABEL ARGUMENT\n", lineNum);
                    return 0;
                }
            }

            break;
        
        case INDEX: /* in case of index mode */
            /* the argument must have a label, and therefore should start with an alphabetic character and then have
             * only alphanumeric characters. If not, raise error*/
            if (!isalpha(*arg)) {
                printf("ERROR IN LINE %d - ILLEGAL CHARACTER IN LABEL ARGUMENT\n", lineNum);
                return 0;
            }
            for (i = 1; arg[i] != '['; i++) {
                if (!isalpha(arg[i]) && !isdigit(arg[i])) {
                    printf("ERROR IN LINE %d - ILLEGAL CHARACTER IN LABEL ARGUMENT\n", lineNum);
                    return 0;
                }
            }

            /* if, after '[', there isn't an r, then raise error */
            if (arg[++i] != 'r') {
                printf("ERROR IN LINE %d - INDEX ADDRESSING METHOD DOES NOT CONTAIN REGISTER\n", lineNum);
                return 0;
            }


            /* check for first digit of register and second one */
            if (!isdigit(arg[i + 1]) || !isdigit(arg[i + 2])) {
                printf("ERROR IN LINE %d - EXTRANEOUS TEXT AFTER REGISTER IN ARGUMENT\n", lineNum);
                return 0;
            }
            /* check for illegal register numbers (e.g. 02, 23, etc.) */
            else if (arg[i + 2] >= '6' || arg[i + 1] != '1') {
                printf("ERROR IN LINE %d - ILLEGAL REGISTER NUMBER FOR INDEXING\n", lineNum);
                return 0;
            }

            /* if the argument isn't closed by a ']', raise error */
            if (arg[i + 3] != ']') {
                printf("ERROR IN LINE %d - EXTRANEOUS TEXT AFTER REGISTER IN ARGUMENT\n", lineNum);
                return 0;
            }

            break;

        case REGISTER: /* in case of a register */
            arg++;
            for (i = 0; i < 2 && isdigit(arg[i]); i++) { /* make sure that the numbers are valid */
                if ((i == 0 && arg[i] == '0' && isdigit(arg[i + 1])) || (i > 0 && arg[i] >= '6') || (i > 0 && arg[i - 1] != '1')) {
                    printf("ERROR IN LINE %d - ILLEGAL REGISTER NUMBER\n", lineNum);
                    return 0;
                }
            }
            if (i == 0 || (i == 1 && arg[i] != '\n' && arg[i] != ',') || (i == 2 && arg[i] != '\n' && arg[i] != ',')) {
                printf("ERROR IN LINE %d - EXTRANEOUS TEXT AFTER REGISTER IN ARGUMENT\n", lineNum);
                return 0;
            }

            break;
    }

    return 1; /* if no error was raised, return true */
}
