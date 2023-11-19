#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

class ArgsParser {
    enum class DataType {
        BOOL,
        INT,
        UINT,
        STRING,
        REPEATED,
        SPECIAL,
    };

public:
    struct ParserException : public std::exception {
        explicit ParserException(const std::string& what);
        const char* what() const noexcept override;

    private:
        const std::string what_;
    };

public:
    ArgsParser();
    ArgsParser(const ArgsParser& other) = default;
    ArgsParser(ArgsParser&& other) = default;
    ArgsParser& operator=(const ArgsParser& other) = default;
    ArgsParser& operator=(ArgsParser&& other) = default;

    template <typename T>
    ArgsParser& AddShortOption(char letter, T* container, bool required, const std::string& help_message,
                               std::function<T(const std::string&)> parser);

    template <typename T>
    ArgsParser& AddLongOption(char letter, const std::string& name, T* container, bool required,
                              const std::string& help_message, std::function<T(const std::string&)> parser);

    template <typename T>
    ArgsParser& AddLongOption(const std::string& name, T* container, bool required, const std::string& help_message,
                              std::function<T(const std::string&)> parser);

    ArgsParser& SetHelpMessage(const std::string& message);

    void Parse(int argc, char* argv[]) const;

#define DECLARE_BASE_TYPE_FUNCTION(TYPE)                                                                      \
    ArgsParser& AddShortOption(char letter, TYPE* container, bool required, const std::string& help_message); \
    ArgsParser& AddLongOption(const std::string& name, TYPE* container, bool required,                        \
                              const std::string& help_message);                                               \
    ArgsParser& AddLongOption(char letter, const std::string& name, TYPE* container, bool required,           \
                              const std::string& help_message)

    DECLARE_BASE_TYPE_FUNCTION(bool);
    DECLARE_BASE_TYPE_FUNCTION(uint64_t);
    DECLARE_BASE_TYPE_FUNCTION(int64_t);
    DECLARE_BASE_TYPE_FUNCTION(std::string);
    DECLARE_BASE_TYPE_FUNCTION(std::vector<std::string>);
#undef DECLARE_BASE_TYPE_FUNCTION

private:
    template <typename T>
    ArgsParser& AddOption(char letter, const std::string& name, T* container, bool required,
                          const std::string& help_message, std::function<T(const std::string&)> parser, DataType type);

    void CheckNamesAreAllowed(char letter, const std::string& name) const;
    void ReadArgs(int argc, char* argv[], std::unordered_map<char, std::string>& short_arg,
                  std::unordered_map<std::string, std::string>& long_args) const;
    void PrintHelpMessage() const;

private:
    struct SingleArgumentInfoBase {
        std::string GetName() const;
        virtual void ParseAndSave(const std::string& arg) const = 0;
        virtual ~SingleArgumentInfoBase() = default;

    public:
        char letter;
        std::string name;
        DataType data_type;
        std::string help_message;
        bool is_required;
    };

    template <typename T>
    struct SingleArgumentInfo : public SingleArgumentInfoBase {
        void ParseAndSave(const std::string& arg) const override;

    public:
        T* container;
        std::function<T(const std::string&)> parser;
    };

private:
    std::string user_help_message_;
    std::vector<std::unique_ptr<SingleArgumentInfoBase>> parsers_;
    std::unordered_map<char, SingleArgumentInfoBase*> short_name_to_parser_;
    std::unordered_map<std::string, SingleArgumentInfoBase*> long_name_to_parser_;
};

template <typename T>
ArgsParser& ArgsParser::AddShortOption(char letter, T* container, bool required, const std::string& help_message,
                                       std::function<T(const std::string&)> parser) {
    return AddOption<T>(letter, "", container, required, help_message, parser, DataType::SPECIAL);
}

template <typename T>
ArgsParser& ArgsParser::AddLongOption(char letter, const std::string& name, T* container, bool required,
                                      const std::string& help_message, std::function<T(const std::string&)> parser) {
    return AddOption<T>(letter, name, container, required, help_message, parser, DataType::SPECIAL);
}

template <typename T>
ArgsParser& ArgsParser::AddLongOption(const std::string& name, T* container, bool required,
                                      const std::string& help_message, std::function<T(const std::string&)> parser) {
    return AddOption<T>(0, name, container, required, help_message, parser, DataType::SPECIAL);
}

template <typename T>
ArgsParser& ArgsParser::AddOption(char letter, const std::string& name, T* container, bool required,
                                  const std::string& help_message, std::function<T(const std::string&)> parser,
                                  DataType type) {
    CheckNamesAreAllowed(letter, name);

    std::unique_ptr<SingleArgumentInfo<T>> info = std::make_unique<SingleArgumentInfo<T>>();
    info->letter = letter;
    info->name = name;
    info->data_type = type;
    info->help_message = help_message;
    info->is_required = required;
    info->container = container;
    info->parser = parser;

    parsers_.emplace_back(std::move(info));
    if (letter != 0) {
        short_name_to_parser_[letter] = parsers_.back().get();
    }
    if (!name.empty()) {
        long_name_to_parser_[name] = parsers_.back().get();
    }
    return *this;
}

template <typename T>
void ArgsParser::SingleArgumentInfo<T>::ParseAndSave(const std::string& arg) const {
    *container = parser(arg);
}
