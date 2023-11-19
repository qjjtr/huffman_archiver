#include "args_parser.h"

#include <iostream>

namespace {
constexpr char HELP_LETTER = 'h';  // NOLINT
const std::string HELP_NAME = "help";

template <typename T>
T ConvertFromString(const std::string& arg, bool is_signed) {
    if (arg.empty()) {
        throw std::bad_cast{};
    }
    int64_t result = 0;
    size_t i = 0;
    if (is_signed && arg[0] == '-') {
        ++i;
        if (arg.size() == 1) {
            throw std::bad_cast{};
        }
    }
    for (; i < arg.size(); ++i) {
        if (!std::isdigit(arg[i])) {
            throw std::bad_cast{};
        }
        result = result * 10 + arg[i] - '0';
    }
    if (arg[0] == '-') {
        result *= -1;
    }
    return result;
}
}  // namespace

ArgsParser::ParserException::ParserException(const std::string& what) : what_("ArgsParser: " + what) {
}

const char* ArgsParser::ParserException::what() const noexcept {
    return what_.data();
}

ArgsParser::ArgsParser() {
    AddLongOption('h', "help", static_cast<bool*>(nullptr), false, "to see this message");
}

ArgsParser& ArgsParser::SetHelpMessage(const std::string& message) {
    user_help_message_ = message;
    return *this;
}

void ArgsParser::Parse(int argc, char* argv[]) const {
    std::unordered_map<char, std::string> short_args;
    std::unordered_map<std::string, std::string> long_args;

    ReadArgs(argc, argv, short_args, long_args);

    if (short_args.count(HELP_LETTER) || long_args.count(HELP_NAME)) {
        PrintHelpMessage();
        exit(0);
    }

    for (const auto& info : parsers_) {
        if (short_args.count(info->letter) || long_args.count(info->name)) {
            const std::string& argument =
                short_args.count(info->letter) ? short_args.at(info->letter) : long_args.at(info->name);
            try {
                info->ParseAndSave(argument);
            } catch (const std::exception& e) {
                throw ParserException("can't parse " + info->GetName() + " option\nError: " + e.what());
            } catch (...) {
                throw ParserException("can't parse " + info->GetName() + " option");
            }
        } else {
            if (info->is_required) {
                throw ParserException("required option " + info->GetName() + " is not set");
            }
        }
    }
}

void ArgsParser::ReadArgs(int argc, char* argv[], std::unordered_map<char, std::string>& short_args,
                          std::unordered_map<std::string, std::string>& long_args) const {
    std::string* current_arg = nullptr;
    bool last_part_is_name = false;
    DataType last_type{};

    auto try_parse_arg_name = [&](const std::string& part) -> bool {
        if (part.starts_with("--")) {
            std::string name = part.substr(2);
            if (!long_name_to_parser_.count(name)) {
                throw ParserException("unknown long option: " + name);
            }
            if (long_args.count(name) || short_args.count(long_name_to_parser_.at(name)->letter)) {
                throw ParserException("option " + name + " is already set");
            }
            current_arg = &long_args[name];
            last_part_is_name = true;
            last_type = long_name_to_parser_.at(name)->data_type;
            return true;
        } else if (part.starts_with("-")) {
            if (part.size() > 2) {
                throw ParserException("short option name is single char");
            }
            char letter = part[1];
            if (!short_name_to_parser_.count(letter)) {
                throw ParserException(std::string{} + "unknown short option: " + letter);
            }
            if (short_args.count(letter) || long_args.count(short_name_to_parser_.at(letter)->name)) {
                throw ParserException(std::string{} + "option " + letter + " is already set");
            }
            current_arg = &short_args[letter];
            last_part_is_name = true;
            last_type = short_name_to_parser_.at(letter)->data_type;
            return true;
        }
        return false;
    };

    for (int i = 1; i < argc; ++i) {
        std::string part = argv[i];
        if (!last_part_is_name || last_type == DataType::BOOL) {
            try {
                if (try_parse_arg_name(part)) {
                    continue;
                }
            } catch (const std::exception& e) {
                if (last_type != DataType::REPEATED) {
                    throw;
                }
            }
        }

        if (current_arg == nullptr) {
            throw ParserException("type option name before value");
        }

        if (!last_part_is_name && last_type != DataType::REPEATED) {
            throw ParserException("repeated args are allowed only for repeated options");
        }

        if (current_arg->empty()) {
            *current_arg = part;
        } else {
            *current_arg += ' ' + part;
        }
        last_part_is_name = false;
    }
    if (last_part_is_name && last_type != DataType::BOOL) {
        throw ParserException("argument is required for non-bool options");
    }
}

#define DEFINE_BASE_TYPE_FUNCTION(TYPE)                                                                         \
    ArgsParser& ArgsParser::AddShortOption(char letter, TYPE* container, bool required,                         \
                                           const std::string& help_message) {                                   \
        return AddLongOption(letter, "", container, required, help_message);                                    \
    }                                                                                                           \
    ArgsParser& ArgsParser::AddLongOption(const std::string& name, TYPE* container, bool required,              \
                                          const std::string& help_message) {                                    \
        return AddLongOption(0, name, container, required, help_message);                                       \
    }                                                                                                           \
    ArgsParser& ArgsParser::AddLongOption(char letter, const std::string& name, TYPE* container, bool required, \
                                          const std::string& help_message)

DEFINE_BASE_TYPE_FUNCTION(bool) {
    return AddOption<bool>(
        letter, name, container, required, help_message,
        [](const std::string& arg) { return arg.empty() || (arg != "0" && arg != "false" && arg != "False"); },
        DataType::BOOL);
}
DEFINE_BASE_TYPE_FUNCTION(uint64_t) {
    return AddOption<uint64_t>(
        letter, name, container, required, help_message,
        [](const std::string& arg) { return ConvertFromString<int64_t>(arg, false); }, DataType::UINT);
}

DEFINE_BASE_TYPE_FUNCTION(int64_t) {
    return AddOption<int64_t>(
        letter, name, container, required, help_message,
        [](const std::string& arg) { return ConvertFromString<int64_t>(arg, true); }, DataType::INT);
}
DEFINE_BASE_TYPE_FUNCTION(std::string) {
    return AddOption<std::string>(
        letter, name, container, required, help_message, [](const std::string& arg) { return arg; }, DataType::STRING);
}
DEFINE_BASE_TYPE_FUNCTION(std::vector<std::string>) {
    return AddOption<std::vector<std::string>>(
        letter, name, container, required, help_message,
        [](const std::string& arg) {
            std::vector<std::string> result;
            size_t pos = 0;
            while (pos != std::string::npos) {
                size_t new_pos = arg.find(' ', pos);
                if (new_pos == std::string::npos) {
                    result.push_back(arg.substr(pos));
                    pos = new_pos;
                } else {
                    result.push_back(arg.substr(pos, new_pos - pos));
                    pos = new_pos + 1;
                }
            }
            return result;
        },
        DataType::REPEATED);
}

#undef DEFINE_BASE_TYPE_FUNCTION

void ArgsParser::CheckNamesAreAllowed(char letter, const std::string& name) const {
    if (short_name_to_parser_.count(letter)) {
        throw ParserException(std::string{} + "short name " + letter + " is already taken");
    }
    if (long_name_to_parser_.count(name)) {
        throw ParserException("long name " + name + " is already taken");
    }
}

void ArgsParser::PrintHelpMessage() const {
    if (!user_help_message_.empty()) {
        std::cout << user_help_message_ << std::endl;
    }
    std::cout << "Usage: " << std::endl;
    for (const auto& info : parsers_) {
        if (info->letter) {
            std::cout << '-' << info->letter << ' ';
        }
        if (!info->name.empty()) {
            std::cout << "--" << info->name << ' ';
        }
        std::string type{};
        switch (info->data_type) {
            case DataType::BOOL:
                type = "BOOL";
                break;
            case DataType::INT:
                type = "INT";
                break;
            case DataType::UINT:
                type = "UINT";
                break;
            case DataType::STRING:
                type = "STRING";
                break;
            case DataType::REPEATED:
                type = "STR1 STR2 ...";
                break;
            case DataType::SPECIAL:
                type = "SPECIAL";
                break;
        }
        std::cout << '<' << type << "> ";
        if (info->is_required) {
            std::cout << "[required]" << ' ';
        }
        if (!info->help_message.empty()) {
            std::cout << info->help_message;
        }
        std::cout << std::endl;
    }
}

std::string ArgsParser::SingleArgumentInfoBase::GetName() const {
    return name.empty() ? std::string{letter} : name;
}
