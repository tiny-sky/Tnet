CC=g++

# 设置编译选项
CFLAGS=-I../../../../src/include -fsanitize=address
LDFLAGS=-lTnet -lprotobuf -lfmt -lpthread -lz

# 定义proto文件
PROTO_FILE=sudoku.proto

# 生成的源文件
PROTO_SRC=sudoku.pb.cc
PROTO_HEADER=sudoku.pb.h

COMMON=$(PROTO_SRC)

all: $(PROTO_SRC) server client

proto: $(PROTO_SRC)

$(PROTO_SRC): $(PROTO_FILE)
	$(PROTOC) --cpp_out=. $(PROTO_FILE)

server: server.cc $(COMMON)
	$(CC) -o server server.cc $(COMMON) $(CFLAGS) $(LDFLAGS)

client: client.cc $(COMMON)
	$(CC) -o client client.cc $(COMMON) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f server client 

.PHONY: all proto clean