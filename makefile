.DEFAULT_GOAL := shell

shell: shell.c shell.h
	gcc shell.c -o shell

tests: redirTest.c smallerWaitTest.c waitTest.c
	gcc redirTest.c -o redirTest
	gcc smallerWaitTest.c -o smallerWaitTest
	gcc waitTest.c -o waitTest

clean:
	rm -f *.o redirTest smallerWaitTest waitTest shell