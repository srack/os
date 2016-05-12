all: myshell myshell_extracredit

myshell_extracredit: myshell_extracredit.o
	gcc myshell_extracredit.o -o myshell_extracredit

myshell: myshell.o
	gcc myshell.o -o myshell

myshell.o: myshell.c
	gcc -c -Wall myshell.c

myshell_extracredit.o: myshell_extracredit.c
	gcc -c -Wall myshell_extracredit.c

clean:
	rm -f *.o myshell myshell_extracredit
