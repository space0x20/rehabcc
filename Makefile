CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

rehabcc: $(OBJS)
	$(CC) -o rehabcc $(OBJS) $(LDFLAGS)

rehabcc_debug: $(OBJS)
	$(CC) -o rehabcc_debug -g -O0 $(OBJS) $(LDFLAGS)

$(OBJS): rehabcc.h

test: rehabcc test/helper.o
	./test.sh

test/helper.o: test/helper.c
	$(CC) -o test/helper.o -c test/helper.c

clean:
	rm -f rehabcc *.o *.s tmp*

.PHONY: test clean core rehabcc_debug

