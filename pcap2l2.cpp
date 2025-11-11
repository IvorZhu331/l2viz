#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>

// IEX DEEP Price Level Update Message Format
static constexpr int    PRL_MSG_LEN = 30;
static constexpr uint8_t MSG_TYPE_BUY  = 0x38;
static constexpr uint8_t MSG_TYPE_SELL = 0x35;

#pragma pack(push,1)
struct L2Event {
    uint64_t ts_ns;  // timestamp in nanoseconds
    double   price;
    uint32_t size;
    uint8_t  side;   // 1=ask, 0=bid
    uint8_t  rtype;  // 'R' or 'Z'
};
#pragma pack(pop)

// endian helpers
static inline uint16_t le16(const unsigned char* p){
    return uint16_t(p[0]) | (uint16_t(p[1])<<8);
}
static inline uint32_t le32(const unsigned char* p){
    return uint32_t(p[0]) | (uint32_t(p[1])<<8) | (uint32_t(p[2])<<16) | (uint32_t(p[3])<<24);
}
static inline uint64_t le64(const unsigned char* p){
    return uint64_t(p[0]) | (uint64_t(p[1])<<8) | (uint64_t(p[2])<<16) | (uint64_t(p[3])<<24) |
           (uint64_t(p[4])<<32) | (uint64_t(p[5])<<40) | (uint64_t(p[6])<<48) | (uint64_t(p[7])<<56);
}
static inline int64_t le64_signed(const unsigned char* p){
    return (int64_t)le64(p);
}
static inline uint16_t be16(const unsigned char* p){
    return (uint16_t(p[0])<<8) | uint16_t(p[1]);
}

static std::string parse_symbol(const unsigned char* s, size_t n){
    size_t end = n;
    while(end > 0 && (s[end-1] == ' ' || s[end-1] == '\0')) --end;
    return std::string((const char*)s, end);
}

int main(int argc, char** argv){
    if (argc < 3){
        std::fprintf(stderr, "usage: %s <pcapng-file /dev/stdin > <SYMBOL>\n", argv[0]);
        return 1;
    }
    const char* path = argv[1];
    const std::string want_sym = argv[2];
    
    FILE* fp = std::fopen(path, "rb");
    if (!fp){
        std::perror("fopen");
        return 1;
    }

    // Skip Section Header
    unsigned char shb_hdr[8];
    if (std::fread(shb_hdr, 1, 8, fp) != 8) return 1;
    uint32_t shb_len = le32(shb_hdr + 4);
    std::vector<unsigned char> shb_rest(shb_len - 8);
    if (std::fread(shb_rest.data(), 1, shb_rest.size(), fp) != shb_rest.size()) return 1;

    size_t packets = 0, msgs = 0, emitted = 0;

    while(true){
        unsigned char bh[8];
        if (std::fread(bh, 1, 8, fp) != 8) break; // EOF
        uint32_t bt = le32(bh + 0);
        uint32_t bl = le32(bh + 4);
        if (bl < 12){
            std::fprintf(stderr, "bad block length\n");
            break;
        }
        std::vector<unsigned char> blk(bl - 8);
        if (std::fread(blk.data(), 1, blk.size(), fp) != blk.size()) break;
        // Only process Enhanced Packet Blocks
        if (bt != 0x00000006) continue;
        // Enhanced Packet Block
        if (blk.size() < 20) continue;
        const unsigned char* b = blk.data();
        uint32_t if_id    = le32(b + 0);
        // uint32_t ts_high  = le32(b + 4);
        // uint32_t ts_low   = le32(b + 8);
        uint32_t cap_len  = le32(b + 12);
        uint32_t orig_len = le32(b + 16);
        (void)if_id; (void)orig_len;

        // Calculate padded length
        size_t pad4_len = (cap_len + 3u) & ~size_t(3);
        if (blk.size() < 20 + pad4_len) continue;

        const unsigned char* pkt = b + 20;
        size_t pkt_len = cap_len;
        const unsigned char* p = pkt;

        if (pkt_len < 14) continue;
        uint16_t eth_type = be16(p + 12);
        if (eth_type != 0x0800) continue;

        // Parse IPv4 header
        if (pkt_len < 14 + 20) continue;
        const unsigned char* ip = p + 14;
        uint8_t ihl = (ip[0] & 0x0F);
        size_t ip_hlen = size_t(ihl) * 4;
        if (14 + ip_hlen + 8 > pkt_len) continue;

        // Parse UDP header (8 bytes)
        size_t l4_off = 14 + ip_hlen + 8;
        if (l4_off + 40 > pkt_len) continue;

        // IEX-TP header (40 bytes) - skip it
        size_t off = l4_off + 40;

        while (off + 2 <= pkt_len){
            uint16_t mlen = le16(p + off);
            off += 2;
            if (mlen == 0) break;
            if (off + mlen > pkt_len){
                off = pkt_len;
                break;
            }

            const unsigned char* msg = p + off;
            uint8_t msg_type = msg[0];

            // Check if this is a Price Level Update message
            if (mlen == PRL_MSG_LEN && (msg_type == MSG_TYPE_BUY || msg_type == MSG_TYPE_SELL)){
                // uint8_t event_flags = msg[1];
                uint64_t msg_ts_ns = le64(msg + 2);
                std::string sym = parse_symbol(msg + 10, 8);
                uint32_t size = le32(msg + 18);
                int64_t price_fixed = le64_signed(msg + 22);
                double price = double(price_fixed) / 10000.0;

                if (sym == want_sym){
                    uint8_t side = (msg_type == MSG_TYPE_SELL) ? 1 : 0; // 1=ask, 0=bid
                    uint8_t rtype = (size > 0) ? 'R' : 'Z';
                    L2Event ev{msg_ts_ns, price, size, side, rtype};
                    ::write(1, &ev, sizeof(ev));
                    ++emitted;
                }
            }

            ++msgs;
            off += mlen;
        }
        ++packets;
    }

    std::fprintf(stderr, "pcapng: packets=%zu msgs=%zu emitted=%zu\n", packets, msgs, emitted);
    std::fclose(fp);
    return 0;
}
