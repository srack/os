all: copyit copyitr

copyit: copyit.o
	gcc copyit.o -o copyit

copyitr: copyit_extracredit.o
	gcc copyit_extracredit.o -o copyitr

copyit_extracredit.o: copyit_extracredit.c
	gcc -c -Wall copyit_extracredit.c

copyit.o: copyit.c
	gcc -c -Wall copyit.c

clean:
	rm -f *.o copyit copyitr 
