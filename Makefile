CC=gcc
DEPS = BF.h  defs.h  hash_func.h  ht.h  mem.h  sht.h print.h hash_func.h
OBJS = hash_func.o  ht.o mem.o print.o sht.o testmain.c
OBJS2 = hash_func.o  ht.o mem.o print.o sht.o main.c

findex: $(OBJS)
	$(CC) -o findex $(OBJS) BF_64.a -Wall -g

findex2: $(OBJS2)
	$(CC) -o findex2 $(OBJS2) BF_64.a -Wall -g

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -Wall

clean:
	rm *.o findex*
