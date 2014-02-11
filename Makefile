LIB = lib/libuv.a lib/libccf.a
DEMO = demo/demo_all_sort demo/demo_any_sort demo/demo_http_server
TEST = test/test_primitive test/test_sleep test/test_udp test/test_udp2 test/test_udp3 test/test_tcp test/test_tcp2 test/test_tcp3 test/benchmark_udp test/benchmark_udp2 test/benchmark_tcp

top: $(LIB)
all: $(LIB) $(DEMO) $(TEST)
	
$(LIB): src/cocoflow.cc include/cocoflow.h
	make -C src
	
$(DEMO): demo/*.cc
	make -C demo
	
$(TEST): test/*.cc
	make -C test

.PHONY: clean test del

clean:
	make -C src clean
	make -C demo clean
	make -C test clean
	
test: $(TEST)
	make -C test all_test
	
del: $(LIB)
	cp lib/libuv.a libuv.a
	cp lib/libccf.a libccf.a
	cp include/cocoflow.h cocoflow.h
	rm -rf include/ lib/ src/ demo/ test/ deps/ docs/ extensions/ test/ vc/ Makefile README.md vcbuild.bat
