# Simple Makefile to build and run gen_mats.cpp
# to produce MatrixA/B/C .txt and corresponding .h files.

CXX      := g++
CXXFLAGS := -std=c++17 -O2

GEN_SRC := gen_mats.cpp
GEN_EXE := gen_mats

# Files that gen_mats produces
TXT_FILES := MatrixA.txt MatrixB.txt GoldenC.txt
HDR_FILES := inputA_data.h inputB_data.h goldenC_data.h

.PHONY: all gen clean

# Default: build generator and run it
all: gen

# Explicit target if you just want to regenerate the data
gen: $(GEN_EXE)
	./$(GEN_EXE)

# Build the generator binary
$(GEN_EXE): $(GEN_SRC) ../include/matmul_config.h
	$(CXX) $(CXXFLAGS) -o $(GEN_EXE) $(GEN_SRC)

# Clean everything produced in this directory
clean:
	rm -f $(GEN_EXE) $(TXT_FILES) $(HDR_FILES)
