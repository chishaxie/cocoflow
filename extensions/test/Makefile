C_ARGS = -g -Wall -O2
CX = g++

LIB = ../../lib/libuv.a ../../lib/libccf.a
BIN = test_redis test_redis2 test_http_get test_http_post

LIB_ARGS = -L../../lib -lccf -luv -lpthread -lrt
INC_ARGS = -I../../include

LIB_REDIS = ../../lib/libccf-redis.a
LIB_REDIS_ARGS = -lccf-redis

LIB_HTTP = ../../lib/libccf-http.a
LIB_HTTP_ARGS = -lccf-http

all: $(BIN)
	
$(LIB):
	make -C ../../src/
	
$(LIB_REDIS):
	make -C ../redis/

$(LIB_HTTP):
	make -C ../http/
	
test_redis: test_redis.cc $(LIB) $(LIB_REDIS)
	$(CX) $(C_ARGS) -o $@ test_redis.cc $(LIB_REDIS_ARGS) $(LIB_ARGS) $(INC_ARGS)
test_redis2: test_redis2.cc $(LIB) $(LIB_REDIS)
	$(CX) $(C_ARGS) -o $@ test_redis2.cc $(LIB_REDIS_ARGS) $(LIB_ARGS) $(INC_ARGS)

test_http_get: test_http_get.cc $(LIB) $(LIB_HTTP)
	$(CX) $(C_ARGS) -o $@ test_http_get.cc $(LIB_HTTP_ARGS) $(LIB_ARGS) $(INC_ARGS)
test_http_post: test_http_post.cc $(LIB) $(LIB_HTTP)
	$(CX) $(C_ARGS) -o $@ test_http_post.cc $(LIB_HTTP_ARGS) $(LIB_ARGS) $(INC_ARGS)

.PHONY: clean
clean:
	rm -f $(BIN)
	
.PHONY: test
	
test: $(BIN)
	@echo 'test redis'
	@(./test_redis > /dev/null && echo 'Succeed') || echo 'Failed'
