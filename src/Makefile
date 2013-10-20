C_ARGS = -g -Wall -O2
CX = g++

UV_VERSION=0.10.18
UV_BUILDTYPE=Release

LIB = ../lib/libuv.a ../lib/libccf.a

INC_ARGS = -I../include

all: $(LIB)

../lib/libccf.a: cocoflow.o
	ar cr $@ $^

cocoflow.o: cocoflow.cc ../include/cocoflow.h uv/uv.h
	$(CX) $(C_ARGS) -fPIC -c -o $@ cocoflow.cc $(INC_ARGS)

uv/uv.h: ../deps/libuv.a
	@mkdir uv; cp -r ../deps/libuv-$(UV_VERSION)/include/* uv/

../lib/libuv.a: ../deps/libuv.a
	@cp $^ $@

../deps/libuv.a: ../deps/libuv-$(UV_VERSION).zip ../deps/gyp.zip
	@echo "Dependency analysis ..."
	@cd ../deps/; unzip libuv-$(UV_VERSION).zip > /dev/null; unzip gyp.zip > /dev/null; cd libuv-$(UV_VERSION)/; mkdir build; cd build/; mkdir gyp; cd gyp; cp -r ../../../gyp/* ./; cd ../../../../src/
	@echo "Compiling libuv ..."
	@cd ../deps/libuv-$(UV_VERSION)/; ./gyp_uv -f make > /dev/null; cd out/; make BUILDTYPE=$(UV_BUILDTYPE) > /dev/null; cd ../../../src/;
	@cp ../deps/libuv-$(UV_VERSION)/out/$(UV_BUILDTYPE)/libuv.a $@
	@echo "Compiled libuv"

.PHONY: clean
clean:
	rm -f $(LIB) *.o
