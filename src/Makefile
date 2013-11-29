C_ARGS = -g -Wall -O2
CX = g++

UV_VERSION=0.10.19
UV_BUILDTYPE=Release

LIB = ../lib/libuv.a ../lib/libccf.a

INC_ARGS = -I../include

all: $(LIB)

../lib/libccf.a: cocoflow.o sleep.o sync.o udp.o tcp.o tools.o
	ar cr $@ $^

cocoflow.o: cocoflow.cc cocoflow-comm.h ../include/cocoflow.h uv/uv.h
	$(CX) $(C_ARGS) -fPIC -c -o $@ cocoflow.cc $(INC_ARGS)
	
sleep.o: sleep.cc cocoflow-comm.h ../include/cocoflow.h uv/uv.h
	$(CX) $(C_ARGS) -fPIC -c -o $@ sleep.cc $(INC_ARGS)
	
sync.o: sync.cc cocoflow-comm.h ../include/cocoflow.h uv/uv.h
	$(CX) $(C_ARGS) -fPIC -c -o $@ sync.cc $(INC_ARGS)
	
udp.o: udp.cc cocoflow-comm.h ../include/cocoflow.h uv/uv.h
	$(CX) $(C_ARGS) -fPIC -c -o $@ udp.cc $(INC_ARGS)
	
tcp.o: tcp.cc cocoflow-comm.h ../include/cocoflow.h uv/uv.h
	$(CX) $(C_ARGS) -fPIC -c -o $@ tcp.cc $(INC_ARGS)
	
tools.o: tools.cc cocoflow-comm.h ../include/cocoflow.h uv/uv.h
	$(CX) $(C_ARGS) -fPIC -c -o $@ tools.cc $(INC_ARGS)

uv/uv.h: ../deps/libuv.a
	@mkdir uv; cp -r ../deps/libuv-$(UV_VERSION)/include/* uv/

../lib/libuv.a: ../deps/libuv.a
	@cp $^ $@

../deps/libuv.a: ../deps/libuv-$(UV_VERSION).zip ../deps/gyp.zip
	@echo "Dependency analysis ..."
	@cd ../deps/; unzip libuv-$(UV_VERSION).zip > /dev/null; unzip gyp.zip > /dev/null; cd libuv-$(UV_VERSION)/; mkdir build; cd build/; mkdir gyp; cd gyp; cp -r ../../../gyp/* ./; cd ../../../../src/
	@echo "Compiling libuv ..."
	@cd ../deps/libuv-$(UV_VERSION)/; ./gyp_uv.py -f make > /dev/null; cd out/; make BUILDTYPE=$(UV_BUILDTYPE) > /dev/null; cd ../../../src/;
	@cp ../deps/libuv-$(UV_VERSION)/out/$(UV_BUILDTYPE)/libuv.a $@
	@echo "Compiled libuv"

.PHONY: clean
clean:
	rm -f $(LIB) *.o
