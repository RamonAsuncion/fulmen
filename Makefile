CXX?=g++
CFLAGS=-Isrc -Wall -Wextra # -g -DDEBUG
SRC=src
BIN=bin

main: src/main.cpp | mkbin
	$(CXX) $(CFLAGS) src/main.cpp -o $(BIN)/main
mkbin:
	@ mkdir -p $(BIN)
doc:
	doxygen

.PHONY: main mkbin doc clean
clean:
	@ rm -rf $(BIN)/* core* *~ src/*~ docs/* *.dSYM

