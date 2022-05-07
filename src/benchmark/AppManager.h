
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
#include "Variant.h"

#define __Text(x)   x
#define __L_Text(x) L ## x

#ifndef _TEXT
#define _Text(x)    __Text(x)
#endif

namespace app {

FILE * const LOG_FILE = stderr;

template <typename CharT = char>
static inline
bool is_null_or_empty(const std::basic_string<CharT> & str)
{
    if (str.size() == 0) {
        return true;
    } else {
        const CharT * data = str.c_str();
        if (data != nullptr) {
            // Trim left
            std::size_t ltrim = str.find_first_not_of(CharT(' '));
            if (ltrim == std::basic_string<CharT>::npos)
                ltrim = str.size();
            // Trim right
            std::size_t rtrim = str.find_last_not_of(CharT(' '));
            if (rtrim == std::basic_string<CharT>::npos)
                rtrim = 0;
            else
                ++rtrim;

            return (ltrim >= rtrim);
        } else {
            return true;
        }
    }
}

template <typename CharT = char>
static inline
std::size_t find_first_not_of(const std::basic_string<CharT> & str, char token, std::size_t first, std::size_t last)
{
    std::size_t cur = first;
    while (cur < last) {
        if (str[cur] == token)
            ++cur;
        else
            return cur;
    }
    return std::basic_string<CharT>::npos;
}

template <typename CharT = char>
static inline
std::size_t find_last_not_of(const std::basic_string<CharT> & str, char token, std::size_t first, std::size_t last)
{
    std::size_t cur = --last;
    while (cur >= first) {
        if (str[cur] == token)
            --cur;
        else
            return (cur + 1);
    }
    return std::basic_string<CharT>::npos;
}

template <typename CharT = char>
static inline
void string_trim_left(const std::basic_string<CharT> & str, std::size_t & first, std::size_t last)
{
    // Trim left
    std::size_t ltrim = find_first_not_of<CharT>(str, CharT(' '), first, last);
    if (ltrim == std::basic_string<CharT>::npos)
        ltrim = last;

    first = ltrim;
}

template <typename CharT = char>
static inline
void string_trim_right(const std::basic_string<CharT> & str, std::size_t first, std::size_t & last)
{
    // Trim right
    std::size_t rtrim = find_last_not_of<CharT>(str, CharT(' '), first, last);
    if (rtrim == std::basic_string<CharT>::npos)
        rtrim = first;

    last = rtrim;
}

template <typename CharT = char>
static inline
void string_trim(const std::basic_string<CharT> & str, std::size_t & first, std::size_t & last)
{
    // Trim left
    std::size_t ltrim = find_first_not_of<CharT>(str, CharT(' '), first, last);
    if (ltrim == std::basic_string<CharT>::npos)
        ltrim = last;

    // Trim right
    std::size_t rtrim = find_last_not_of<CharT>(str, CharT(' '), ltrim, last);
    if (rtrim == std::basic_string<CharT>::npos)
        rtrim = ltrim;

    first = ltrim;
    last = rtrim;
}

template <typename CharT = char>
static inline
std::size_t split_string_by_token(const std::basic_string<CharT> & names, CharT token,
                                  std::vector<std::basic_string<CharT>> & name_list)
{
    name_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = names.find_first_of(token, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = names.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        string_trim<CharT>(names, ltrim, rtrim);

        if (ltrim < rtrim) {
            std::basic_string<CharT> name;
            for (std::size_t i = ltrim; i < rtrim; i++) {
                name.push_back(names[i]);
            }
            name_list.push_back(name);
        }

        if (token_pos < names.size())
            last_pos = token_pos + 1;
        else
            break;
    } while (1);

    return name_list.size();
}

template <typename CharT = char>
static inline
std::size_t split_string_by_token(const std::basic_string<CharT> & names,
                                  const std::basic_string<CharT> & tokens,
                                  std::vector<std::basic_string<CharT>> & name_list)
{
    name_list.clear();

    std::size_t last_pos = 0;
    do {
        std::size_t token_pos = names.find_first_of(tokens, last_pos);
        if (token_pos == std::basic_string<CharT>::npos) {
            token_pos = names.size();
        }

        std::size_t ltrim = last_pos;
        std::size_t rtrim = token_pos;

        // Trim left and right space chars
        string_trim<CharT>(names, ltrim, rtrim);

        if (ltrim < rtrim) {
            std::basic_string<CharT> name;
            for (std::size_t i = ltrim; i < rtrim; i++) {
                name.push_back(names[i]);
            }
            name_list.push_back(name);
        }

        if (token_pos < names.size())
            last_pos = token_pos + 1;
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
        Text,
        Void,
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

typedef jstd::Variant<bool, int8_t, uint8_t, int16_t, uint16_t,
                      int32_t, uint32_t, int64_t, uint64_t,
                      size_t, intptr_t, uintptr_t, ptrdiff_t,
                      float, double, void *,
                      char *, const char *, wchar_t *, const wchar_t *,
                      std::string, std::wstring
        > Variant;

template <typename CharT = char>
class CmdLine {
public:
    typedef CmdLine<CharT>                                  this_type;
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef typename ::jstd::char_traits<CharT>::Signed     schar_type;
    typedef typename ::jstd::char_traits<CharT>::Unsigned   uchar_type;

    typedef std::basic_string<char_type>                    string_type;

#if defined(_MSC_VER)
    static const char_type kPathSeparator = _Text('\\');
#else
    static const char_type kPathSeparator = _Text('/');
#endif

    struct Option {
        std::size_t value_type;
        std::size_t desc_id;
        string_type names;
        string_type names_str;
        string_type desc;
        Variant     value;

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

        void find_all_name(std::size_t target_option_id, std::vector<string_type> & name_list) const {
            name_list.clear();
            for (auto iter = this->option_map_.begin(); iter != this->option_map_.end(); ++iter) {
                const string_type & arg_name = iter->first;
                std::size_t option_id = iter->second;
                if (option_id == target_option_id) {
                    name_list.push_back(arg_name);
                }
            }
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
            option.value_type = ValueType::Text;
            option.desc = text;
            this->option_list_.push_back(option);
        }

        std::size_t parseOptionName(const string_type & names,
                                    std::vector<string_type> & name_list,
                                    std::vector<string_type> & text_list) {
            std::vector<string_type> token_list;
            string_type tokens = _Text(", ");
            std::size_t nums_token = split_string_by_token(names, tokens, token_list);
            if (nums_token > 0) {
                name_list.clear();
                text_list.clear();
                for (auto iter = token_list.begin(); iter != token_list.end(); ++iter) {
                    const string_type & token = *iter;
                    string_type name;
                    std::size_t pos = token.find(_Text("--"));
                    if (pos == 0) {
                        for (std::size_t i = pos + 2; i < token.size(); i++) {
                            name.push_back(token[i]);
                        }
                        if (!is_null_or_empty(name)) {
                            name_list.push_back(name);
                        }
                    } else {
                        pos = token.find(char_type('-'));
                        if (pos == 0) {
                            for (std::size_t i = pos + 1; i < token.size(); i++) {
                                name.push_back(token[i]);
                            }
                            if (!is_null_or_empty(name)) {
                                name_list.push_back(name);
                            }
                        } else {
                            assert(!is_null_or_empty(token));
                            text_list.push_back(token);
                        }
                    }
                }
            }
            return name_list.size();
        }

        // Accept format: --name=abc, or -n=10, or -n 10
        template <std::size_t valueType = ValueType::Void>
        int addOption(const string_type & names, const string_type & desc,
                      const Variant & value) {
            int err_code = Error::NO_ERRORS;

            Option option(valueType);
            option.names = names;
            option.names_str = names;
            option.value = value;
            option.desc = desc;

            std::vector<string_type> name_list;
            std::vector<string_type> text_list;
            std::size_t nums_name = this->parseOptionName(names, name_list, text_list);
            if (nums_name > 0) {
                // Sort all the option names asc
                for (std::size_t i = 0; i < (name_list.size() - 1); i++) {
                    for (std::size_t j = i + 1; j < name_list.size(); j++) {
                        if (name_list[i].size() > name_list[j].size()) {
                            std::swap(name_list[i], name_list[j]);
                        } else if (name_list[i].size() == name_list[j].size()) {
                            if (name_list[i] > name_list[j])
                                std::swap(name_list[i], name_list[j]);
                        }
                    }
                }

                // Format short name text
                string_type s_names;
                for (std::size_t i = 0; i < name_list.size(); i++) {
                    s_names += name_list[i];
                    if ((i + 1) < name_list.size())
                        s_names += _Text(",");
                }
                option.names = s_names;

                // Format display name text
                string_type names_str;
                for (std::size_t i = 0; i < name_list.size(); i++) {
                    const string_type & name = name_list[i];
                    if (name.size() == 1)
                        names_str += _Text("-");
                    else if (name.size() > 1)
                        names_str += _Text("--");
                    else
                        continue;
                    names_str += name;
                    if ((i + 1) < name_list.size())
                        names_str += _Text(", ");
                }
                if (text_list.size() > 0) {
                    names_str += _Text("  ");
                }
                for (std::size_t i = 0; i < text_list.size(); i++) {
                    names_str += text_list[i];
                    if ((i + 1) < text_list.size())
                        names_str += _Text(" ");
                }
                option.names_str = names_str;

                std::size_t option_id = this->option_list_.size();
                this->option_list_.push_back(option);
        
                for (auto iter = name_list.begin(); iter != name_list.end(); ++iter) {
                    const string_type & name = *iter;
                    // Only the first option with the same name is valid.
                    if (this->option_map_.count(name) == 0) {
                        this->option_map_.insert(std::make_pair(name, option_id));
                    } else {
                        // Warning
                        printf("Warning: desc: \"%s\", option_id: %u, arg_name = \"%s\" already exists.\n\n",
                               this->title.c_str(), (uint32_t)option_id, name.c_str());
                    }
                }
            }
            return (err_code == Error::NO_ERRORS) ? (int)nums_name : err_code;
        }

        template <std::size_t valueType = ValueType::Void>
        int addOption(const string_type & names, const string_type & desc) {
            Variant default_value(0);
            return this->addOption<valueType>(names, desc, default_value);
        }

        void print() const {
            if (!is_null_or_empty(title)) {
                printf("%s:\n\n", title.c_str());
            }

            for (auto iter = this->option_list_.begin(); iter != this->option_list_.end(); ++iter) {
                const Option & option = *iter;
                if (option.value_type == ValueType::Text) {
                    if (!is_null_or_empty(option.desc)) {
                        printf("%s", option.desc.c_str());
                    }
                } else {
                    if (!is_null_or_empty(option.names_str)) {
                        printf("  %s :\n\n", option.names_str.c_str());
                        if (!is_null_or_empty(option.desc)) {
                            printf("    %s\n\n", option.desc.c_str());
                        }
                    } else {
                        if (!is_null_or_empty(option.desc)) {
                            printf("%s\n\n", option.desc.c_str());
                        }
                    }
                }
            }
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
    const Option & getOption(std::size_t option_id) const  {
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

    bool hasVariable(const string_type & name) const {
        auto iter = this->option_map_.find(name);
        return (iter != this->option_map_.end());
    }

    bool getVariable(const string_type & name, Variant & variable) const {
        auto iter = this->option_map_.find(name);
        if (iter != this->option_map_.end()) {
            std::size_t option_id = iter->second;
            if (option_id <  this->option_list_.size()) {
                const Option & option = this->getOption(option_id);
                variable = option.value;
                return true;
            }
        }
        return false;
    }

    void setVariable(const string_type & name, Variant & variable) {
        auto iter = this->option_map_.find(name);
        if (iter != this->option_map_.end()) {
            std::size_t option_id = iter->second;
            if (option_id <  this->option_list_.size()) {
                const Option & option = this->getOption(option_id);
                option.value = variable;;
            }
        }
    }

    std::size_t addDesc(const OptionsDescription & desc) {
        std::size_t desc_id = this->option_desc_list_.size();
        this->option_desc_list_.push_back(desc);
        std::size_t index = 0;
        for (auto iter = desc.option_list_.begin(); iter != desc.option_list_.end(); ++iter) {
            const Option & option = *iter;
            std::size_t desc_option_id = index;
            std::vector<string_type> arg_names;
            desc.find_all_name(desc_option_id, arg_names);
            if (arg_names.size() != 0) {
                std::size_t option_id = this->option_list_.size();
                this->option_list_.push_back(option);
                for (std::size_t i = 0; i < arg_names.size(); i++) {
                    if (this->option_map_.count(arg_names[i]) == 0) {
                        this->option_map_.insert(std::make_pair(arg_names[i], option_id));
                    } else {
                        printf("Warning: desc_id: %u, desc: \"%s\", option_id: %u, arg_name = \"%s\" already exists.\n\n",
                               (uint32_t)desc_id, desc.title.c_str(), (uint32_t)option_id, arg_names[i].c_str());
                    }
                };
            }
            index++;
        }
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
