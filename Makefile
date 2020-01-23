CFLAGS=-std=c11 -g -static

rehubcc: rehubcc.c

test: rehubcc
	./test.sh

clean:
	rm -f rehubcc *.o tmp*

.PHONY: test clean

