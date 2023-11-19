#include "huffman.h"

#include "args_parser.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

struct Args {
    enum class Mode { ARCHIVE, UNARCHIVE };
    Mode mode;
    std::string archive_name;
    std::vector<std::string> files_names;
};

bool AreStringsEqual(const char* lhs, const char* rhs) {
    return std::string(lhs) == std::string(rhs);
}

Args ParseArgs(int argc, char* argv[]) {
    Args args;
    ArgsParser{}
        .SetHelpMessage("Archive and unarchive files using Huffman algorithm with canonization")
        .AddShortOption('c', &args.files_names, false, "archive files. [archive_name file1 file2 ...]")
        .AddShortOption('d', &args.archive_name, false, "unarchive files")
        .Parse(argc, argv);
    if (!args.files_names.empty()) {
        args.archive_name = args.files_names[0];
        args.files_names.erase(args.files_names.begin());
        args.mode = Args::Mode::ARCHIVE;
    } else {
        args.mode = Args::Mode::UNARCHIVE;
    }
    return args;
}

int main(int argc, char* argv[]) {
    Args args;
    try {
        args = ParseArgs(argc, argv);
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 111;
    }
    switch (args.mode) {
        case Args::Mode::ARCHIVE: {
            std::vector<std::pair<std::string, std::ifstream>> files_streams;
            for (const auto& file : args.files_names) {
                files_streams.push_back({file, std::ifstream{file}});
                if (!files_streams.back().second.is_open()) {
                    std::cout << "cannot open file " << file << std::endl;
                    return 111;
                }
            }
            std::ofstream ofstream{args.archive_name};
            if (!files_streams.back().second.is_open()) {
                std::cout << "cannot open or create file " << args.archive_name << std::endl;
                return 111;
            }
            try {
                HuffmanArchive(ofstream, files_streams);
            } catch (const std::exception& e) {
                std::cout << "error occurred while archivation: " << e.what() << std::endl;
                return 111;
            }
            break;
        }
        case Args::Mode::UNARCHIVE: {
            std::ifstream ifstream{args.archive_name};
            if (!ifstream.is_open()) {
                std::cout << "cannot open archive file" << std::endl;
                return 111;
            }
            try {
                HuffmanUnarchive(ifstream);
            } catch (const std::exception& e) {
                std::cout << "error occurred while unarchivation: " << e.what() << std::endl;
                return 111;
            }
            break;
        }
    }
    return 0;
}
