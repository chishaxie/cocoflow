C_ARGS = -g -Wall -O2
CX = g++

LIB = ../../lib/libccf-http.a

INC_ARGS = -I../../include -I../../src -I../../src/uv

all: $(LIB)

../../lib/libccf-http.a: cocoflow-http.o
	ar cr $@ $^

cocoflow-http.o: cocoflow-http.cc ../../src/cocoflow-comm.h ../../include/cocoflow.h ../../include/cocoflow-http.h ../../src/uv/uv.h
	$(CX) $(C_ARGS) -fPIC -c -o $@ cocoflow-http.cc $(INC_ARGS)
	
../../src/uv/uv.h:
	make -C ../../src/

.PHONY: clean
clean:
	rm -f $(LIB) *.o
