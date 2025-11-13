# L2 Visualizer

A high-performance Level 2 order book visualizer built with C++, SDL2, and ImGui. Reads binary L2 events from stdin and displays real-time market depth with bid/ask ladders and top-of-book information.

- Real-time order book visualization
- Top-of-book display (best bid/ask and spread)
- Multi-threaded event processing

## Getting Started
### Prerequisites

- C++17 compatible compiler
- CMake 3.20+
- SDL2 development libraries

### Setup & Build

```bash
git clone <repo>
cd l2viz
make
```

### Data Setup

1. Create a `data` folder in the project root dir.

2. Download historic IEX DEEP pcap data (`.pcap.gz` files) and place them in the `data` folder.

3. Update the pcap filename in the `Makefile` if needed (currently looking for `data/data/data_feeds_20251107_20251107_IEXTP1_DEEP1.0.pcap.gz`).

## Running

```bash
make run SYMBOL=AAPL
```

Replace `AAPL` with any symbol in your data. The visualizer will display:

## Project Structure

```
.
├── CMakeLists.txt
├── Makefile
├── README.md
├── .gitignore
├── src/
│   ├── main.cpp
│   ├── book.hpp
│   └── common.hpp
├── data/                 # Place .pcap.gz files here
└── build/                # Generated on build
```

## Data Format

The visualizer expects binary L2 events piped from `pcap2l2`, which converts IEX DEEP pcap files to binary `L2Event` structs containing timestamp, price, size, side, and record type.
