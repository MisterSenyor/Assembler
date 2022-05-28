
/* macros for filenames */
# define objFiletype ".ob"
# define entFiletype ".ent"
# define extFiletype ".ext"

/* function protoypes */
void freeWordLL(Word *);
void writeImmediateToWord(Word *, char *, int);
void writeLabelToWords(Word *, Label *);
Label *getLabelByName(Label *, char *);
int secondCheck(Word *, Word *, Label *, char *, int, int);
