
#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#if defined(_WIN32) || defined(WIN32) || defined(OS_WINDOWS) || defined(_WINDOWS_)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // WIN32_LEAN_AND_MEAN
#endif // _WIN32

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>

#include "char_traits.h"

namespace app {

FILE * const LOG_FILE = stderr;

static inline
bool string_is_null_or_empty(const std::string & str)
{
    if (str.size() == 0) {
        return true;
    } else {
        const char * data = str.c_str();
        if (data != nullptr) {
            std::size_t ltrim = str.find_first_of(' ');
            if (ltrim == std::string::npos)
                ltrim = 0;
            std::size_t rtrim = str.find_last_of(' ');
            if (rtrim == std::string::npos)
                rtrim = str.size();

            return (ltrim == rtrim);
        } else {
            return true;
        }
    }
}

static inline
std::size_t split_string_by_token(const std::string & names, char token,
                                  std::vector<std::string> & name_list)
{
    name_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = names.find_first_of(token, last_pos);
        if (token_pos == std::string::npos) {
            token_pos = names.size();
        }

        std::string name;
        for (std::size_t i = last_pos; i < token_pos; i++) {
            name.push_back(names[i]);
        }
        name_list.push_back(name);

        if (token_pos < names.size())
            last_pos = token_pos + sizeof(token);
        else
            break;
    } while (1);

    return name_list.size();
}

static inline
std::size_t split_string_by_token(const std::string & names, const std::string & token,
                                  std::vector<std::string> & name_list)
{
    name_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = names.find_first_of(token, last_pos);
        if (token_pos == std::string::npos) {
            token_pos = names.size();
        }

        std::string name;
        for (std::size_t i = last_pos; i < token_pos; i++) {
            name.push_back(names[i]);
        }
        name_list.push_back(name);

        if (token_pos < names.size())
            last_pos = token_pos + token.size();
        else
            break;
    } while (1);

    return name_list.size();
}

struct Error {
    enum {
        ERROR_FIRST = -10000,
        TEXT_FILE_IS_NULL,
        NO_ERRORS = 0,
    };

    static bool isSuccess(int error_id) {
        return (error_id == NO_ERRORS);
    }

    static bool hasErrors(int error_id) {
        return (error_id < NO_ERRORS);
    }
};

struct ValueType {
    enum {
        Unknown,
        Integer,
        Float,
        Double,
        String,
        Path,
        FilePath
    };
};

struct Config {
    const char * text_file;

    Config() : text_file(nullptr) {
        this->init();
    }

    void init() {
        this->text_file = nullptr;
    }

    bool assert_check(bool condition, const char * format, ...) {
        if (!condition) {
            va_list args;
            va_start(args, format);
            vfprintf(LOG_FILE, format, args);
            va_end(args);
        }

        return condition;
    }

    int check() {
        bool condition;

        condition = assert_check((this->text_file != nullptr), "[text_file] must be specified.\n");
        if (!condition) { return Error::TEXT_FILE_IS_NULL; }

        return Error::NO_ERRORS;
    }
};

template <typename CharT = char>
class CmdLine {
public:
    typedef CmdLine<CharT>                                  this_type;
    typedef typename ::detail::char_trait<CharT>::NoSigned  char_type;
    typedef typename ::detail::char_trait<CharT>::Signed    schar_type;
    typedef typename ::detail::char_trait<CharT>::Unsigned  uchar_type;

    typedef std::basic_string<char_type>                    string_type;

#if defined(_MSC_VER)
    static const char_type kPathSeparator = '\\';
#else
    static const char_type kPathSeparator = '/';
#endif

    struct Option {
        std::size_t value_type;
        std::size_t desc_id;
        string_type names;
        string_type names_str;
        string_type value;
        string_type default_value;
        string_type desc;

        Option(std::size_t _value_type = ValueType::Unknown)
            : value_type(_value_type), desc_id(0) {
        }
    };

    struct OptionsDescription {
        string_type title;

        std::vector<Option>                          option_list_;
        std::unordered_map<string_type, std::size_t> option_map_;

        OptionsDescription() {
        }

        OptionsDescription(const string_type & _title) : title(_title) {
        }

        void addText(const char * format, ...) {
            static const std::size_t kMaxTextSize = 4096;
            char_type text[kMaxTextSize];
            va_list args;
            va_start(args, format);
            int text_size = vsnprintf(text, kMaxTextSize, format, args);
            assert(text_size < (int)kMaxTextSize);
            va_end(args);

            Option option;
            option.desc = text;
            this->option_list_.push_back(option);
        }

        std::size_t parseOptionName(const string_type & names, std::vector<string_type> & name_list) {
            return split_string_by_token(names, char_type(','), name_list);
        }

        // Accept format: --name=abc, or -n=10, or -n 10
        template <std::size_t valueType = ValueType::String>
        int addOptions(const string_type & names, const string_type & desc,
                       const string_type & default_value) {
            int err_code = Error::NO_ERRORS;
            Option option(valueType);
            option.names = names;
            option.default_value = default_value;
            option.desc = desc;
            std::vector<string_type> name_list;
            std::size_t nums_name = parseOptionName(names, name_list);
            if (nums_name > 0) {
                string_type names_str;
                std::size_t option_id = this->option_list_.size();
                this->option_list_.push_back(option);
        
                for (auto iter = name_list.begin(); iter != name_list.end(); ++iter) {
                    const string_type & name = *iter;
                    this->option_map_.insert(std::make_pair(name, option_id));
                }
            }
            return (err_code == Error::NO_ERRORS) ? (int)nums_name : err_code;
        }

        void print() const {
            if (!string_is_null_or_empty(title)) {
                printf("%s:\n\n", title.c_str());
            }

            for (auto iter = this->option_list_.begin(); iter != this->option_list_.end(); ++iter) {
                const Option & option = *iter;
                if (!string_is_null_or_empty(option.names)) {
                    printf("  %s:\n\n", option.names.c_str());
                    printf("    %s\n", option.desc.c_str());
                } else {
                    if (!string_is_null_or_empty(option.desc)) {
                        printf("%s\n", option.desc.c_str());
                    }
                }
            }

            printf("\n");
        }
    };

private:
    std::vector<OptionsDescription>                 option_desc_list_;
    std::vector<Option>                             option_list_;
    std::unordered_map<string_type, std::size_t>    option_map_;

public:
    CmdLine() {
        //
    }

private:
    string_type getOptionValue(std::size_t option_id) const  {
        assert(option_id >= 0 && option_id < this->option_list_.size());
        return this->option_list_[option_id];
    }

public:
    static string_type getAppName(char_type * argv[]) {
        string_type strModuleName = argv[0];
        std::size_t lastSepPos = strModuleName.find_last_of(kPathSeparator);
        if (lastSepPos != string_type::npos) {
            string_type appName;
            for (std::size_t i = lastSepPos + 1; i < strModuleName.size(); i++) {
                appName.push_back(strModuleName[i]);
            }
            return appName;
        } else {
            return strModuleName;
        }
    }

    bool hasOption(const string_type & name) const {
        auto iter = this->option_map_.find(name);
        return (iter != this->option_map_.end());
    }

    string_type optionValue(const string_type & name) const {
        auto iter = this->option_map_.find(name);
        if (iter != this->option_map_.end()) {
            std::size_t option_id = iter->second;
            return this->getOptionValue(option_id);
        } else {
            return string_type(char_type(""));
        }
    }

    std::size_t parseOptionName(const string_type & names, std::vector<string_type> & name_list) {
        return split_string_by_token(names, char_type(','), name_list);
    }

    std::size_t addDesc(const OptionsDescription & desc) {
        std::size_t desc_id = this->option_desc_list_.size();
        this->option_desc_list_.push_back(desc);
        return desc_id;
    }

    void printUsage() const {
        for (auto iter = this->option_desc_list_.begin();
             iter != this->option_desc_list_.end(); ++iter) {
            const OptionsDescription & desc = *iter;
            desc.print();
        }
    }

    int parseArgs(int argc, char_type * argv[]) {
        //
        return argc;
    }
};

} // namespace app

#endif // APP_MANAGER_H
