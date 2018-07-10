CC = gcc
DEBUG = -g
INC = -I./include/ 
LIBS = -lpthread -llog
LIBDIR = -L./lib/ -Wl,-rpath=./lib
CFLAGS = 
#-D__FILENAME__='"$(subst $(dir $<), ,$<)"'
SHARED = -fpic -shared


SRC_DIR = . 

SRC = $(foreach dir,$(SRC_DIR), $(wildcard $(dir)/*.c))

BIN = net 

SHAREDBIN = net.so

all:$(SRC)
	$(CC) $(SRC) $(CFLAGS) $(DEBUG) $(INC) $(LIBDIR) $(LIBS) -o $(BIN)

shared:$(SRC)
	$(CC) $(SRC) $(CFLAGS) $(INC) $(LIBS) $(SHARED) -o $(SHAREDBIN)

client:$(SRC)
	$(CC) $(SRC) $(CFLAGS) -DCLIENT $(DEBUG) $(INC) $(LIBDIR) $(LIBS) -o client

server:$(SRC)
	$(CC) $(SRC) $(CFLAGS) -DSERVER $(DEBUG) $(INC) $(LIBDIR) $(LIBS) -o server

.PHONY:

clean:
	rm -rf $(BIN) $(SHAREDBIN) client server
