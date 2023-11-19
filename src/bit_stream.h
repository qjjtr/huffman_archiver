#include <fstream>

class BitIfstream {
public:
    explicit BitIfstream(std::ifstream& stream);
    uint64_t Get(size_t count);

private:
    std::ifstream& stream_;
    unsigned char next_;
    size_t next_len_;
};

class BitOfstream {
public:
    explicit BitOfstream(std::ofstream& stream);
    ~BitOfstream();

    void Put(uint64_t bits, size_t count);

private:
    std::ofstream& stream_;
    unsigned char next_;
    size_t next_len_;
};
