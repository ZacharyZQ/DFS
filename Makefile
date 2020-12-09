CC = gcc
LINK = $(CC)
CFLAGS = -Wall -Werror -g -I./include
LDFLAGS = -pthread
SOURCES = $(wildcard *.c) $(wildcard fs/*.c)
EX_MAIN_SOURCES = $(filter-out test.c slave_main.c master_main.c, $(SOURCES))
EX_MAIN_OBJS = $(patsubst %.c,%.o,$(EX_MAIN_SOURCES))
OBJS = $(patsubst %.c,%.o,$(SOURCES))

.PHONY:clean all

all: test master slave
	mkdir -p log/
	mkdir -p disk/
test:$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(EX_MAIN_OBJS) test.o

master:$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(EX_MAIN_OBJS) master_main.o

slave:$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(EX_MAIN_OBJS) slave_main.o

clean:
	rm -f $(OBJS) test master slave
	rm -f log/*

#rm -f disk/*
