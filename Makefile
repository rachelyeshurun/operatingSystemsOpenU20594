##Makefile for generating my_cd and my_dir
##Course 20594 Operating Systems

CC = gcc
CFLAGS = -Wall -ansi -g

all: my_cd my_dir

# -lm to link math library AFTER the objects to link 
my_cd: my_cd.o
	$(CC) -o my_cd my_cd.o -lm

my_cd.o: my_cd.c
	$(CC) $(CFLAGS) -c my_cd.c

my_dir: my_dir.o
	$(CC) -o my_dir my_dir.o -lm

my_dir.o: my_dir.c
	$(CC) $(CFLAGS) -c my_dir.c


#cleanup
clean:
	rm -f forktest *.o *~
