assembler: assembler.o macros.o firstCheck.o secondCheck.o utils.o
	gcc assembler.o macros.o firstCheck.o secondCheck.o utils.o -o assembler
assembler.o: assembler.c assembler.h
	gcc -c assembler.c -Wall -ansi -pedantic
macros.o: macros.c macros.h
	gcc -c macros.c -Wall -ansi -pedantic
firstCheck.o: firstCheck.c firstCheck.h
	gcc -c firstCheck.c -Wall -ansi -pedantic
secondCheck.o: secondCheck.c secondCheck.h
	gcc -c secondCheck.c -Wall -ansi -pedantic

utils.o: utils.c
	gcc -c utils.c -Wall -ansi -pedantic
