# include "assembler.h"

extern int firstCheck(char *);
extern int spread(char *);

int main(int argc, char **argv) {
    /* main
     * argc - argument count from command line
     * argv - string array for all arguments from command line
     * returns exit code */
    int i; /* i - counter */
    if (argc < 2) {
        printf("NOT ENOUGH ARGS\n");
        return -1;
    }

    for (i = 1; i < argc; i++) {
        printf("ASSEMBLING FILE %s...\n", argv[i]);
        printf("PREPROCESSING STAGE %s\n", spread(argv[i]) == 0 ? "SUCCESSFUL" : "FAILED");
        printf("ASSEMBLING STAGE %s\n\n\n", firstCheck(argv[i]) == 0 ? "SUCCESSFUL" : "FAILED");
    }


    
    return 0;
}
