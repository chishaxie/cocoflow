C_ARGS = -g -Wall -O2
CX = g++

LIB = ../lib/libuv.a ../lib/libccf.a
BIN = test_primitive test_sleep test_udp test_udp2 test_udp3 test_tcp test_tcp2 test_tcp3 benchmark_sleep benchmark_udp benchmark_udp2 benchmark_tcp

LIB_ARGS = -L../lib -lccf -luv -lpthread -lrt
INC_ARGS = -I../include

all: $(BIN)
	
$(LIB):
	make -C ../src/
	
test_primitive: test_primitive.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ test_primitive.cc $(LIB_ARGS) $(INC_ARGS)	

test_sleep: test_sleep.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ test_sleep.cc $(LIB_ARGS) $(INC_ARGS)
	
test_udp: test_udp.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ test_udp.cc $(LIB_ARGS) $(INC_ARGS)
	
test_udp2: test_udp2.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ test_udp2.cc $(LIB_ARGS) $(INC_ARGS)
	
test_udp3: test_udp3.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ test_udp3.cc $(LIB_ARGS) $(INC_ARGS)
	
test_tcp: test_tcp.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ test_tcp.cc $(LIB_ARGS) $(INC_ARGS)

test_tcp2: test_tcp2.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ test_tcp2.cc $(LIB_ARGS) $(INC_ARGS)
	
test_tcp3: test_tcp3.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ test_tcp3.cc $(LIB_ARGS) $(INC_ARGS)

benchmark_sleep: benchmark_sleep.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ benchmark_sleep.cc $(LIB_ARGS) $(INC_ARGS)	

benchmark_udp: benchmark_udp.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ benchmark_udp.cc $(LIB_ARGS) $(INC_ARGS)
	
benchmark_udp2: benchmark_udp2.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ benchmark_udp2.cc $(LIB_ARGS) $(INC_ARGS)
	
benchmark_tcp: benchmark_tcp.cc $(LIB)
	$(CX) $(C_ARGS) -o $@ benchmark_tcp.cc $(LIB_ARGS) $(INC_ARGS)

.PHONY: clean
clean:
	rm -f $(BIN)
	
.PHONY: test all_test

test: test_primitive
	@(./test_primitive && echo 'Succeed!') || echo 'Failed!'
	
all_test: $(BIN)
	@echo 'test primitive'
	@(./test_primitive && echo 'Succeed') || echo 'Failed'
	@echo 'test sleep'
	@(./test_sleep > /dev/null && echo 'Succeed') || echo 'Failed'
	@echo 'test udp [3]'
	@(./test_udp > /dev/null && echo 'Succeed') || echo 'Failed'
	@(./test_udp2 > /dev/null && echo 'Succeed') || echo 'Failed'
	@(./test_udp3 > /dev/null && echo 'Succeed') || echo 'Failed'
	@echo 'test tcp [3]'
	@(./test_tcp > /dev/null && echo 'Succeed') || echo 'Failed'
	@(./test_tcp2 > /dev/null && echo 'Succeed') || echo 'Failed'
	@(./test_tcp3 > /dev/null && echo 'Succeed') || echo 'Failed'
	@echo 'benchmark sleep'
	@(./benchmark_sleep > /dev/null && echo 'Succeed') || echo 'Failed'
	@echo 'benchmark udp'
	@(./benchmark_udp2 > /dev/null && echo 'Succeed') || echo 'Failed'
	@echo 'benchmark tcp'
	@(./benchmark_tcp > /dev/null && echo 'Succeed') || echo 'Failed'
