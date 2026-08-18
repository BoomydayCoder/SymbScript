CXX ?= g++

SOURCES := main.cpp compiler.cc driver.cc exptree.cc lex.yy.c parser.tab.cc program.cc value.cc vm.cc
RELEASE_FLAGS := -std=c++17 -O3 -flto -march=native -DNDEBUG
BIN := build/symbscript

.PHONY: all release benchmark compatibility clean

all: release

release: $(BIN)

$(BIN): $(SOURCES)
	@mkdir -p build
	$(CXX) $(SOURCES) $(RELEASE_FLAGS) -o $(BIN)

benchmark: release
	python3 benchmarks/run_benchmarks.py --binary $(BIN)

compatibility: release
	@test -n "$(REFERENCE)" || (echo "Usage: make compatibility REFERENCE=/path/to/reference" && false)
	python3 tests/run_compatibility.py --reference $(REFERENCE) --candidate $(BIN)

clean:
	rm -rf build

