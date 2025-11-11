.PHONY: all build rebuild run clean help
B := build
BIN := $(B)/l2_visualizer
PCAP2L2 := pcap2l2
PCAP := data/data_feeds_20251107_20251107_IEXTP1_DEEP1.0.pcap.gz

CXX ?= c++
J := $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic

all: build $(PCAP2L2)

build:
	@cmake -S . -B $(B)
	@cmake --build $(B) -j $(J)

rebuild: clean all

$(PCAP2L2): pcap2l2.cpp
	@$(CXX) $(CXXFLAGS) -o $@ $<

run: $(BIN) $(PCAP2L2)
	@[ "$(SYMBOL)" ] || { echo "Usage: make run SYMBOL=AAPL"; exit 1; }
	@gzcat $(PCAP) | ./$(PCAP2L2) /dev/stdin $(SYMBOL) | $(BIN) --levels 10

clean:
	@rm -rf $(B) $(PCAP2L2)

help:
	@echo "Targets: build (CMake) | rebuild | run SYMBOL=... | clean"
