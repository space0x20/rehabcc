CFLAGS=-std=c11 -g -static

rehabcc: rehabcc.c

test: rehabcc
	./test.sh

clean:
	rm -f rehabcc *.o tmp*

.PHONY: test clean

