C_ARGS = -g -Wall -O2
CX = g++

LIB = ../../lib/libccf-redis.a

HIREDIS_DEPS=../../deps/hiredis-0.11.0u

HIREDIS_OBJ = net.o hiredis.o sds.o async.o
HIREDIS_OBJ_DEPS = $(HIREDIS_DEPS)/net.o $(HIREDIS_DEPS)/hiredis.o $(HIREDIS_DEPS)/sds.o $(HIREDIS_DEPS)/async.o

INC_ARGS = -I../../include -I../../src -I../../src/uv -I$(HIREDIS_DEPS)

all: $(LIB)

../../lib/libccf-redis.a: $(HIREDIS_OBJ) cocoflow-redis.o
	ar cr $@ $^
	
$(HIREDIS_OBJ): $(HIREDIS_OBJ_DEPS)
	cp $(HIREDIS_OBJ_DEPS) .

cocoflow-redis.o: cocoflow-redis.cc ../../src/cocoflow-comm.h ../../include/cocoflow.h ../../src/uv/uv.h
	$(CX) $(C_ARGS) -fPIC -c -o $@ cocoflow-redis.cc $(INC_ARGS)
	
../../src/uv/uv.h:
	make -C ../../src/
	
$(HIREDIS_OBJ_DEPS): $(HIREDIS_DEPS).zip
	@echo "Dependency analysis ..."
	@cd ../../deps/; unzip hiredis-0.11.0u.zip > /dev/null; cd ../extensions/redis/
	@echo "Compiling hiredis ..."
	@make -C $(HIREDIS_DEPS) > /dev/null
	@echo "Compiled hiredis"

.PHONY: clean
clean:
	rm -f $(LIB) *.o
