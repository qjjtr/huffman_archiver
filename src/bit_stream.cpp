#include "bit_stream.h"

namespace {
constexpr size_t MAX_BIT_COUNT = sizeof(unsigned char) * 8;  // NOLINT
}

BitIfstream::BitIfstream(std::ifstream& stream) : stream_(stream), next_(0), next_len_(MAX_BIT_COUNT) {
}

uint64_t BitIfstream::Get(size_t count) {
    uint64_t result = 0;
    for (size_t i = 0; i < count; ++i) {
        if (next_len_ == MAX_BIT_COUNT) {
            char next_char = 0;
            stream_.get(next_char);
            next_ = static_cast<unsigned char>(next_char);
            next_len_ = 0;
        }
        result = result * 2 + ((next_ >> (MAX_BIT_COUNT - next_len_ - 1)) & 1);
        ++next_len_;
    }
    return result;
}

BitOfstream::BitOfstream(std::ofstream& stream) : stream_(stream), next_(0), next_len_(0) {
}

void BitOfstream::Put(uint64_t bits, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        next_ += (((bits >> (count - i - 1)) & 1) << (MAX_BIT_COUNT - next_len_ - 1));
        ++next_len_;
        if (next_len_ == MAX_BIT_COUNT) {
            stream_.put(next_);
            next_ = 0;
            next_len_ = 0;
        }
    }
}

BitOfstream::~BitOfstream() {
    if (next_len_ != 0) {
        stream_.put(next_);
    }
}
