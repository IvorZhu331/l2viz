#pragma once
#include <map>
#include <optional>
#include <vector>
#include <algorithm>
#include <functional>
#include <cstdint>

struct Book {
    // price -> size
    std::map<double, uint32_t, std::greater<double>> bid;
    std::map<double, uint32_t, std::less<double>>    ask;
    std::optional<double> best_bid, best_ask;

    void apply(uint64_t /*ts_ns*/, double price, uint32_t size, uint8_t side, uint8_t rtype) {
        if (side == 1) {
            if (rtype == 'R' && size > 0) ask[price] = size;
            else ask.erase(price);
        } else {
            if (rtype == 'R' && size > 0) bid[price] = size;
            else bid.erase(price);
        }
        best_bid = bid.empty() ? std::optional<double>() : bid.begin()->first;
        best_ask = ask.empty() ? std::optional<double>() : ask.begin()->first;
    }

    std::vector<std::pair<double,uint32_t>> top_bids(int n, uint32_t min_size=1) const {
        std::vector<std::pair<double,uint32_t>> out;
        for (auto it=bid.begin(); it!=bid.end() && (int)out.size()<n; ++it)
            if (it->second >= min_size) out.push_back(*it);
        return out;
    }
    std::vector<std::pair<double,uint32_t>> top_asks(int n, uint32_t min_size=1) const {
        std::vector<std::pair<double,uint32_t>> out;
        for (auto it=ask.begin(); it!=ask.end() && (int)out.size()<n; ++it)
            if (it->second >= min_size) out.push_back(*it);
        return out;
    }
};
