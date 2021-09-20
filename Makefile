SHELL = /bin/sh
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

all: lz4 server client serverapi test1 test2 test3

default_target: all
.PHONY: default_target

clean: 
	@echo "Clearing current directory from build artifacts..."
	@rm -f client server server.out serverapi.o
	@$(MAKE) clean -C lib/lz4
	@echo "Done."
.PHONY: clean

cleanall: clean
.PHONY: cleanall

client:
	$(CC) $(CCFLAGS) \
		-o client \
		-I include -I lib -I src -I src/client \
		src/client/cli.c \
		src/client/cli.h \
		src/client/main.c \
		src/client/run_action.h \
		src/client/run_action.c \
		src/serverapi.c \
		src/utilities.c \
		lib/logc/src/log.c
	@echo "-- Done building the client binary."
.PHONY: client

lz4:
	$(MAKE) -C lib/lz4
.PHONY: lz4

server: lz4
	$(CC) $(CCFLAGS) \
		-o server \
		-I include -I lib -I src -I src/server \
		src/server/config.c \
		src/server/config.h \
		src/server/deserializer.h \
		src/server/deserializer.c \
		src/server/fifo_cache.c \
		src/server/fifo_cache.h \
		src/server/global_state.h \
		src/server/global_state.c \
		src/server/htable.c \
		src/server/htable.h \
		src/server/main.c \
		src/server/receiver.c \
		src/server/receiver.h \
		src/server/shutdown.h \
		src/server/shutdown.c \
		src/server/ts_counter.h \
		src/server/ts_counter.c \
		src/server/worker.h \
		src/server/worker.c \
		src/server/workload_queue.h \
		src/server/workload_queue.c \
		src/serverapi.c \
		src/utilities.c \
		lib/logc/src/log.c \
		lib/xxHash/xxhash.c \
		lib/tomlc99/toml.c \
		-lpthread
	@echo "-- Done building the server binary."
.PHONY: server

serverapi:
	$(CC) $(CCFLAGS) -c \
		-o logc.o \
		-I lib \
		lib/logc/src/log.c
	$(CC) $(CCFLAGS) -c \
		-o utils.o \
		-I include -I lib \
		src/utilities.c
	$(CC) $(CCFLAGS) -c \
		-o api.o \
		-I include -I lib \
		src/serverapi.c
	@ld -r -o serverapi.o api.o utils.o logc.o
	@rm -f utils.o
	@rm -f api.o
	@rm -f logc.o
	@echo "-- Done building the server API library."
.PHONY: serverapi

test1: server client
	@valgrind --leak-check=full ./server config/test1.toml >> server.out 2>&1 &
	@./test/test1.sh
	@pidof valgrind.bin | xargs kill -SIGHUP
.PHONY: test1

test2: server client
	@./server config/test2.toml >> server.out 2>&1 &
	@./test/test2.sh
	@pidof ./server | xargs kill -SIGHUP
.PHONY: test2

test3: server client
	@./server config/test3.toml >> server.out 2>&1 &
	@./test/test3.sh
.PHONY: test3

help:
	@echo "List of valid targets for this Makefile:"
	@echo "- all (default)"
	@echo "- clean"
	@echo "- cleanall"
	@echo "- client"
	@echo "- lz4"
	@echo "- server"
	@echo "- serverapi"
	@echo "- test1"
	@echo "- test2"
	@echo "- test3"
.PHONY: help
