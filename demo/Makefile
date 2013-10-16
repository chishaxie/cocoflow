C_ARGS = -g -Wall -O2
CX = g++

LIB = ../lib/libuv.a ../lib/libccf.a
BIN = demo_all_sort demo_any_sort demo_http_server

LIB_ARGS = -L../lib -lccf -luv -lpthread -lrt
INC_ARGS = -I../include

all: $(BIN)
	
$(LIB):
	make -C ../src/
	
demo_all_sort: demo_all_sort.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ demo_all_sort.cc $(LIB_ARGS) $(INC_ARGS)
	
demo_any_sort: demo_any_sort.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ demo_any_sort.cc $(LIB_ARGS) $(INC_ARGS)
	
demo_http_server: demo_http_server.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ demo_http_server.cc $(LIB_ARGS) $(INC_ARGS)

.PHONY: clean
clean:
	rm -f $(BIN)
