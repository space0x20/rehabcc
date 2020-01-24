CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

rehabcc: $(OBJS)
	$(CC) -o rehabcc $(OBJS) $(LDFLAGS)

$(OBJS): rehabcc.h

test: rehabcc
	./test.sh

clean:
	rm -f rehabcc *.o tmp*

.PHONY: test clean

