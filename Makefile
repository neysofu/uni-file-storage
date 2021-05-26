FLAGS           = $(DBGFLAGS) $(OPTFLAGS)

# compilatore da usare
CC			= gcc
CCFLAGS    += \
	-std=c99 \
	-Wall \
	-Wpedantic \
	-Wextra \
	-Wshadow \
	-Wcomment \
	-Wno-missing-braces \
	-Wno-missing-field-initializers \
	-Wswitch-default \
	-Wcast-align \
	-Wpointer-arith \
	-Wundef \
	-Wuninitialized \
	-Wredundant-decls \
	-Wold-style-definition \
	-Wunreachable-code \
	-Wunused-macros

default_target: all

.PHONY: default_target

clean: 
	rm -f client server

cleanall: clean
	make clean

client:
	$(CC) $(CCFLAGS) \
		-o client \
		-I include -I lib -I src -I src/client \
		src/client/cli.c \
		src/client/cli.h \
		src/client/err.h \
		src/client/main.c \
		src/serverapi.c \
		src/utils.c \
		lib/logc/src/log.c

server:
	$(CC) $(CCFLAGS) \
		-o server \
		-I include -I lib -I src -I src/server \
		src/server/config.c \
		src/server/config.h \
		src/server/deserializer.h \
		src/server/deserializer.c \
		src/server/htable.c \
		src/server/htable.h \
		src/server/main.c \
		src/server/receiver.c \
		src/server/receiver.h \
		src/server/report.c \
		src/server/report.h \
		src/serverapi.c \
		src/utils.c \
		lib/logc/src/log.c \
		lib/xxHash/xxhash.c \
		lib/tomlc99/toml.c \
		-lpthread

serverapi:
	$(CC) $(CCFLAGS) -c \
		-o utils.o \
		-I include \
		src/utils.c
	$(CC) $(CCFLAGS) -c \
		-o serverapi.o \
		-I include \
		src/serverapi.c
	ld -r -o serverapi.o serverapi.o utils.o
	rm -f utils.o

test1: server client
	./test/test1.sh

test2: server client
	./test/test2.sh

# The shell in which to execute make rules.
SHELL = /bin/sh

help:
	@echo "List of valid targets for this Makefile:"
	@echo "... all (default)"
	@echo "... clean"
	@echo "... cleanall"
	@echo "... client"
	@echo "... server"
	@echo "... serverapi"
	@echo "... test1"
	@echo "... test2"
.PHONY: help