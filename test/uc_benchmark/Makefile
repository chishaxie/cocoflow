C_ARGS = -g -Wall -O2
CX = g++

BIN = switch_in_ping_pong switch_new switch_diff switch_diff_small_stack switch_diff_middle_stack switch_diff_big_stack switch_diff_super_big_stack

all: $(BIN)

switch_in_ping_pong: switch_in_ping_pong.cc
	$(CX) $(C_ARGS) -o $@ $^

switch_new: switch_new.cc
	$(CX) $(C_ARGS) -o $@ $^
	
switch_diff: switch_diff.cc
	$(CX) $(C_ARGS) -o $@ $^
	
switch_diff_small_stack: switch_diff_small_stack.cc
	$(CX) $(C_ARGS) -o $@ $^
	
switch_diff_middle_stack: switch_diff_middle_stack.cc
	$(CX) $(C_ARGS) -o $@ $^
	
switch_diff_big_stack: switch_diff_big_stack.cc
	$(CX) $(C_ARGS) -o $@ $^
	
switch_diff_super_big_stack: switch_diff_super_big_stack.cc
	$(CX) $(C_ARGS) -o $@ $^

.PHONY: test
test: all
	./switch_in_ping_pong
	./switch_new
	./switch_diff
	./switch_diff_small_stack
	./switch_diff_middle_stack
	./switch_diff_big_stack
	./switch_diff_super_big_stack
	
.PHONY: clean
clean:
	rm -f $(BIN)
