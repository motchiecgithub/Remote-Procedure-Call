CC=cc -Wall
RPC_SYSTEM_A=rpc.a
OBJS = rpc.o utils.o
CFLAGS = -Wall -o -g
LDFLAGS = -lm
all: $(RPC_SYSTEM_A)

.PHONY: format all

$(RPC_SYSTEM_A): $(OBJS) 
	ar rcs $(RPC_SYSTEM_A) $(OBJS)


format:
	clang-format -style=file -i *.c *.h
clean: 
	rm -f *.o rpc.a