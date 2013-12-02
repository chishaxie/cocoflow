C_ARGS = -g -Wall -O2
CX = g++

LIB = ../../lib/libuv.a ../../lib/libccf.a
BIN = test_redis

LIB_ARGS = -L../../lib -lccf -luv -lpthread -lrt
INC_ARGS = -I../../include

LIB_REDIS = ../../lib/libccf-redis.a
LIB_REDIS_ARGS = -lccf-redis

all: $(BIN)
	
$(LIB):
	make -C ../../src/
	
$(LIB_REDIS):
	make -C ../redis/
	
test_redis: test_redis.cc $(LIB) $(LIB_REDIS)
	$(CX) $(C_ARGS) -o $@ test_redis.cc $(LIB_ARGS) $(LIB_REDIS_ARGS) $(INC_ARGS)	

.PHONY: clean
clean:
	rm -f $(BIN)
	
.PHONY: test
	
test: $(BIN)
	@echo 'test redis'
	@(./test_redis > /dev/null && echo 'Succeed') || echo 'Failed'
