##Makefile for generating my_cd and my_dir
##Course 20594 Operating Systems
##Rachel Cohen Yeshurun

CC = gcc
CFLAGS = -Wall -ansi -g

#all: clean my_utils.o my_cd my_dir
all: clean my_utils.o my_cd

my_utils.o: my_utils.h my_utils.c
	gcc ${FLAGS} -c my_utils.h my_utils.c	
# -lm to link math library AFTER the objects to link 
my_cd: my_cd.o
	$(CC) -o my_cd my_utils.o my_cd.o -lm

my_cd.o: my_cd.c
	$(CC) $(CFLAGS) -c my_cd.c

#my_dir: my_dir.o
#	$(CC) -o my_dir my_utils.o my_dir.o -lm

#my_dir.o: my_dir.c
#	$(CC) $(CFLAGS) -c my_dir.c


	
#cleanup
clean:
	rm -f my_cd my_dir *.o *~

	
#How to read a core file:
#In makefile: make sure flags include -g
#at command prompt type: ulimit -c unlimited
#run @#$%$! program that crashes
#at command prompt type: gdb ./linux-scalabiltiy core
#at gdb prompt type: bt (to see stack)
#at gdb prompt type: quit (to quit...)
