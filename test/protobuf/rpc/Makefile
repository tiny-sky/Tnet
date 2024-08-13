CC=g++

# 设置编译选项
CFLAGS=-I../../../src/include -fsanitize=address
LDFLAGS=-lTnet -lprotobuf -lfmt -lpthread -lz

COMMON=sudoku.pb.cc

all: server client

server: server.cc $(COMMON)
	$(CC) -o server server.cc $(COMMON) $(CFLAGS) $(LDFLAGS)

client: client.cc $(COMMON)
	$(CC) -o client client.cc $(COMMON) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f server client