#include "huffman.h"

#include "bit_stream.h"
#include "priority_queue.h"
#include "trie.h"

#include <algorithm>
#include <string_view>
#include <unordered_map>

namespace {
const uint64_t FILENAME_END = 256;
const uint64_t ONE_MORE_FILE = 257;
const uint64_t ARCHIVE_END = 258;

struct SymbolCodeInfo {
    uint64_t bits = 0;
    size_t length = 0;
};

using Codebook = std::unordered_map<uint64_t, SymbolCodeInfo>;

std::string_view GetShortPath(std::string_view path) {
    auto slash = path.rfind('/', path.size() - 1);
    if (slash == std::string::npos) {
        return path;
    } else {
        return path.substr(slash + 1);
    }
}

uint64_t ConvertToUnsigned(char c) {
    return static_cast<uint64_t>(static_cast<unsigned char>(c));
}

struct PriorityQueueElement {
    size_t frequency;
    uint64_t the_least_symbol;
    Trie trie;

    bool operator<(const PriorityQueueElement& other) const {
        return frequency == other.frequency ? the_least_symbol < other.the_least_symbol : frequency < other.frequency;
    }
    bool operator>(const PriorityQueueElement& other) const {
        return other < *this;
    }
};

void CollectCodes(TrieRunner& runner, std::vector<std::pair<uint64_t, SymbolCodeInfo>>& codebook,
                  SymbolCodeInfo current_code_info = {0, 0}) {
    if (runner.IsInTerminal()) {
        codebook.push_back({runner.GetCurrentSymbol(), current_code_info});
    }
    for (bool value : {0, 1}) {
        if (runner.HasChild(value)) {
            runner.GoToChild(value);
            CollectCodes(runner, codebook,
                         SymbolCodeInfo{static_cast<uint64_t>(current_code_info.bits * 2 + value),
                                        current_code_info.length + 1});
        }
    }
    runner.GoToParent();
}

Codebook MakeCodebook(std::string_view filename, std::ifstream& stream) {
    std::unordered_map<uint64_t, size_t> frequencies;
    frequencies[FILENAME_END] = 1;
    frequencies[ONE_MORE_FILE] = 1;
    frequencies[ARCHIVE_END] = 1;

    for (const char& c : filename) {
        ++frequencies[ConvertToUnsigned(c)];
    }

    for (char symbol = stream.get(); stream.good() && !stream.eof(); symbol = stream.get()) {
        ++frequencies[ConvertToUnsigned(symbol)];
    }

    PriorityQueue<PriorityQueueElement, std::greater<PriorityQueueElement>> queue;
    for (const auto& [symbol, frequency] : frequencies) {
        queue.Push(PriorityQueueElement{frequency, symbol, Trie(symbol)});
    }

    while (queue.Size() > 1) {
        auto left = queue.Top();
        queue.Pop();
        auto right = queue.Top();
        queue.Pop();
        queue.Push(PriorityQueueElement{left.frequency + right.frequency,
                                        std::min(left.the_least_symbol, right.the_least_symbol),
                                        Trie(std::move(left.trie), std::move(right.trie))});
    }

    std::vector<std::pair<uint64_t, SymbolCodeInfo>> simple_codebook;
    auto runner = TrieRunner(queue.Top().trie);
    CollectCodes(runner, simple_codebook);
    std::sort(simple_codebook.begin(), simple_codebook.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.second.length == rhs.second.length ? lhs.first < rhs.first : lhs.second.length < rhs.second.length;
    });

    Codebook codebook;
    SymbolCodeInfo last{};
    bool is_first = true;
    for (auto [symbol, codeInfo] : simple_codebook) {
        if (is_first) {
            codeInfo.bits = 0;
            is_first = false;
        } else {
            codeInfo.bits = last.bits + 1;
            codeInfo.bits <<= (codeInfo.length - last.length);
        }
        codebook[symbol] = codeInfo;
        last = codeInfo;
    }
    return codebook;
}

Codebook ReadCodebook(BitIfstream& ifstream) {
    size_t codebook_size = ifstream.Get(9);
    std::vector<std::pair<uint64_t, SymbolCodeInfo>> infos(codebook_size);
    for (auto& [symbol, info] : infos) {
        symbol = ifstream.Get(9);
    }
    size_t previous_length = 0;
    size_t current_length = 0;
    size_t symbols_with_current_length = 0;
    uint64_t bits = 0;
    bool is_first = true;

    for (auto& [symbol, info] : infos) {
        while (symbols_with_current_length == 0) {
            symbols_with_current_length = ifstream.Get(9);
            ++current_length;
        }
        if (!is_first) {
            ++bits;
            if (previous_length < current_length) {
                bits <<= current_length - previous_length;
            }
        }
        info.length = current_length;
        info.bits = bits;

        is_first = false;
        previous_length = current_length;
        --symbols_with_current_length;
    }
    return Codebook{infos.begin(), infos.end()};
}

Trie BuildTrie(const Codebook& codebook) {
    Trie trie;
    for (const auto& [symbol, info] : codebook) {
        trie.AddBranch(info.bits, info.length, symbol);
    }
    return trie;
}

void HuffmanWriteCodebook(const Codebook& codebook, BitOfstream& ofstream) {
    ofstream.Put(codebook.size(), 9);
    std::vector<std::pair<uint64_t, SymbolCodeInfo>> sorted_codebook(codebook.begin(), codebook.end());
    std::sort(sorted_codebook.begin(), sorted_codebook.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.second.bits < rhs.second.bits; });
    std::vector<size_t> lengths_counts(sorted_codebook.back().second.length, 0);
    for (const auto& [symbol, codeInfo] : sorted_codebook) {
        ofstream.Put(symbol, 9);
        ++lengths_counts[codeInfo.length - 1];
    }
    for (auto count : lengths_counts) {
        ofstream.Put(count, 9);
    }
}

void WriteFromCodebook(BitOfstream& ofstream, const Codebook& codebook, uint64_t symbol) {
    ofstream.Put(codebook.at(symbol).bits, codebook.at(symbol).length);
}

void HuffmanArchiveSingleFile(std::string_view filename, std::ifstream& ifstream, BitOfstream& ofstream,
                              bool is_last_file) {
    auto codebook = MakeCodebook(filename, ifstream);
    HuffmanWriteCodebook(codebook, ofstream);

    for (const char& c : filename) {
        WriteFromCodebook(ofstream, codebook, ConvertToUnsigned(c));
    }
    WriteFromCodebook(ofstream, codebook, FILENAME_END);

    ifstream.clear();
    ifstream.seekg(0);
    for (char symbol = ifstream.get(); ifstream.good() && !ifstream.eof(); symbol = ifstream.get()) {
        WriteFromCodebook(ofstream, codebook, ConvertToUnsigned(symbol));
    }

    if (is_last_file) {
        WriteFromCodebook(ofstream, codebook, ARCHIVE_END);
    } else {
        WriteFromCodebook(ofstream, codebook, ONE_MORE_FILE);
    }
}

bool HuffmanUnarchiveSingleFile(BitIfstream& ifstream) {
    auto codebook = ReadCodebook(ifstream);
    auto trie = BuildTrie(codebook);
    TrieRunner runner(trie);
    auto get_symbol = [&runner, &ifstream]() -> uint64_t {
        runner.GoToRoot();
        while (!runner.IsInTerminal()) {
            auto value = ifstream.Get(1);
            runner.GoToChild(value);
        }
        return runner.GetCurrentSymbol();
    };
    std::string file_name{};
    uint64_t symbol = 0;
    for (symbol = get_symbol(); symbol != FILENAME_END; symbol = get_symbol()) {
        file_name += static_cast<char>(symbol);
    }
    std::ofstream file{file_name};
    for (symbol = get_symbol(); symbol != ARCHIVE_END && symbol != ONE_MORE_FILE; symbol = get_symbol()) {
        file.put(static_cast<char>(symbol));
    }
    return symbol == ONE_MORE_FILE;
}
}  // namespace

void HuffmanArchive(std::ofstream& archive, std::vector<std::pair<std::string, std::ifstream>>& files) {
    BitOfstream ofstream{archive};
    size_t i = 0;
    for (auto& [name, stream] : files) {
        ++i;
        HuffmanArchiveSingleFile(GetShortPath(name), stream, ofstream, i == files.size());
    }
}

void HuffmanUnarchive(std::ifstream& archive) {
    BitIfstream ifstream{archive};
    while (HuffmanUnarchiveSingleFile(ifstream)) {
    }
}
