
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

// Text is wide char ? Is Unicode on windows ?
#define TEXT_IS_WIDE_CHAR   0

#define __Ansi_Text(x)  x
#define __L_Text(x)     L ## x

#ifndef _TEXT
  #if TEXT_IS_WIDE_CHAR
    #define _Text(x)    __L_Text(x)
  #else
    #define _Text(x)    __Ansi_Text(x)
  #endif
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
std::size_t string_copy(std::basic_string<CharT> & dest,
                        const std::basic_string<CharT> & src,
                        std::size_t first, std::size_t last)
{
    for (std::size_t i = first; i < last; i++) {
        dest.push_back(src[i]);
    }
    return (last > first) ? (last - first) : 0;
}

template <typename CharT = char>
static inline
std::size_t string_copy_n(std::basic_string<CharT> & dest,
                          const std::basic_string<CharT> & src,
                          std::size_t offset, std::size_t count)
{
    std::size_t last = offset + count;
    for (std::size_t i = offset; i < last; i++) {
        dest.push_back(src[i]);
    }
    return count;
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
            std::size_t len = string_copy(name, names, ltrim, rtrim);
            assert(len > 0);
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
            std::size_t len = string_copy(name, names, ltrim, rtrim);
            assert(len > 0);
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
        ErrorFirst = -10000,
        // Default Errors
        CmdLine_UnknownArgument,
        CmdLine_UnrecognizedArgument,
        CmdLine_IllegalFormat,
        ProcessTerminate,

        // User errors
        TextFileIsNull,
        NoError = 0,
    };

    static bool isSuccess(int error_id) {
        return (error_id == NoError);
    }

    static bool hasErrors(int error_id) {
        return (error_id < NoError);
    }
};

struct OptType {
    enum {
        Unknown,
        Text,
        Void,
        String,
    };
};

template <typename CharT = char>
struct BasicConfig {
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef typename ::jstd::char_traits<CharT>::Signed     schar_type;
    typedef typename ::jstd::char_traits<CharT>::Unsigned   uchar_type;

    typedef std::basic_string<char_type>                    string_type;

    const char_type * text_file;

    BasicConfig() : text_file(nullptr) {
        this->init();
    }

    void init() {
        this->text_file = nullptr;
    }

    bool assert_check(bool condition, const char_type * format, ...) {
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

        condition = assert_check((this->text_file != nullptr), _Text("[text_file] must be specified.\n"));
        if (!condition) { return Error::TextFileIsNull; }

        return Error::NoError;
    }
};

typedef jstd::Variant<bool, char, short, int, long, long long,
                      int8_t, uint8_t, int16_t, uint16_t,
                      int32_t, uint32_t, int64_t, uint64_t,
                      size_t, intptr_t, uintptr_t, ptrdiff_t,
                      float, double, void *, const void *,                     
                      char *, const char *, wchar_t *, const wchar_t *,
                      char * const, const char * const, wchar_t * const, const wchar_t * const,
                      std::string, std::wstring,
                      int8_t *, uint8_t *, int16_t *, uint16_t *,
                      int32_t *, uint32_t *, int64_t *, uint64_t *,
                      size_t *, intptr_t *, uintptr_t *, ptrdiff_t *
        > Variant;

template <typename CharT = char>
class BasicCmdLine {
public:
    typedef BasicCmdLine<CharT>                             this_type;
    typedef typename ::jstd::char_traits<CharT>::NoSigned   char_type;
    typedef typename ::jstd::char_traits<CharT>::Signed     schar_type;
    typedef typename ::jstd::char_traits<CharT>::Unsigned   uchar_type;

    typedef std::basic_string<char_type>                    string_type;

    typedef Variant variant_t;

#if defined(_MSC_VER)
    static const char_type kPathSeparator = char_type('\\');
#else
    static const char_type kPathSeparator = char_type('/');
#endif

    struct VariableState {
        std::uint16_t   has_default;
        std::uint16_t   order;
        std::uint16_t   visited;
        std::uint16_t   assigned;

        VariableState() : has_default(0), order(0), visited(0), assigned(0) {
        }
    };

    struct Variable {
        VariableState state;
        string_type   name;
        variant_t     value;
    };

    struct Option {
        std::uint32_t   type;
        std::uint32_t   desc_id;
        string_type     names;
        string_type     display_text;
        string_type     desc;
        Variable        variable;        

        static const std::size_t   NotFound   = (std::size_t)-1;
        static const std::uint32_t NotFound32 = (std::uint32_t)-1;
        static const std::uint16_t NotFound16 = (std::uint16_t)-1;

        static const std::size_t   Unknown   = (std::size_t)-1;
        static const std::uint32_t Unknown32 = (std::uint32_t)-1;

        Option(std::uint32_t _type = OptType::Unknown)
            : type(_type), desc_id(Unknown32) {
        }
    };

    class OptionsDescription {
    public:
        string_type title;

        std::vector<Option>                          option_list_;
        std::unordered_map<string_type, std::size_t> option_map_;

        OptionsDescription() {
        }

        OptionsDescription(const string_type & _title) : title(_title) {
        }

    private:
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
        int addOptionsImpl(std::uint32_t type, const string_type & names,
                           const string_type & desc,
                           const variant_t & value,
                           bool is_default_value) {
            int err_code = Error::NoError;

            Option option(type);
            option.names = names;
            option.display_text = names;
            option.desc = desc;
            option.variable.state.has_default = is_default_value;
            option.variable.value = value;

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
                string_type display_text;
                for (std::size_t i = 0; i < name_list.size(); i++) {
                    const string_type & name = name_list[i];
                    if (name.size() == 1)
                        display_text += _Text("-");
                    else if (name.size() > 1)
                        display_text += _Text("--");
                    else
                        continue;
                    display_text += name;
                    if ((i + 1) < name_list.size())
                        display_text += _Text(", ");
                }
                if (text_list.size() > 0) {
                    display_text += _Text(" ");
                }
                for (std::size_t i = 0; i < text_list.size(); i++) {
                    display_text += text_list[i];
                    if ((i + 1) < text_list.size())
                        display_text += _Text(" ");
                }
                option.display_text = display_text;

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
            return (err_code == Error::NoError) ? (int)nums_name : err_code;
        }

    public:
        void find_all_name(std::size_t target_id, std::vector<string_type> & name_list) const {
            name_list.clear();
            for (auto iter = this->option_map_.begin(); iter != this->option_map_.end(); ++iter) {
                const string_type & arg_name = iter->first;
                std::size_t option_id = iter->second;
                if (option_id == target_id) {
                    name_list.push_back(arg_name);
                }
            }
        }

        void addText(const char * format, ...) {
            static const std::size_t kMaxTextSize = 4096;
            char text[kMaxTextSize];
            va_list args;
            va_start(args, format);
            int text_size = vsnprintf(text, kMaxTextSize, format, args);
            assert(text_size < (int)kMaxTextSize);
            va_end(args);

            Option option(OptType::Text);
            option.desc = text;
            this->option_list_.push_back(option);
        }

#if defined(_MSC_VER)
        void addText(const wchar_t * format, ...) {
            static const std::size_t kMaxTextSize = 4096;
            wchar_t text[kMaxTextSize];
            va_list args;
            va_start(args, format);
            int text_size = _vsnwprintf(text, kMaxTextSize, format, args);
            assert(text_size < (int)kMaxTextSize);
            va_end(args);

            Option option(OptType::Text);
            option.desc = text;
            this->option_list_.push_back(option);
        }
#endif

        int addOptions(const string_type & names, const string_type & desc, const variant_t & value) {
            return this->addOptionsImpl(OptType::String, names, desc, value, true);
        }

        int addOptions(const string_type & names, const string_type & desc) {
            Variant default_value(std::size_t(0));
            return this->addOptionsImpl(OptType::Void, names, desc, default_value, false);
        }

        void print() const {
            if (!is_null_or_empty(title)) {
                printf("%s:\n\n", title.c_str());
            }

            for (auto iter = this->option_list_.begin(); iter != this->option_list_.end(); ++iter) {
                const Option & option = *iter;
                if (option.type == OptType::Text) {
                    if (!is_null_or_empty(option.desc)) {
                        printf("%s\n", option.desc.c_str());
                    }
                } else {
                    if (!is_null_or_empty(option.display_text)) {
                        printf("  %s :\n\n", option.display_text.c_str());
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
    }; // class OptionsDescription

protected:
    std::vector<OptionsDescription>                 option_desc_list_;
    std::vector<Option>                             option_list_;
    std::unordered_map<string_type, std::size_t>    option_map_;

    std::vector<string_type> arg_list_;

    string_type app_name_;
    string_type display_name_;
    string_type version_;

    Variable    empty_variable_;

public:
    BasicCmdLine() {
        this->version_ = _Text("1.0.0");
    }

protected:
    Option & getOptionById(std::size_t option_id) {
        assert(option_id >= 0 && option_id < this->option_list_.size());
        return this->option_list_[option_id];
    }

    const Option & getOptionById(std::size_t option_id) const {
        assert(option_id >= 0 && option_id < this->option_list_.size());
        return this->option_list_[option_id];
    }

    bool hasOption(const string_type & name) const {
        auto iter = this->option_map_.find(name);
        return (iter != this->option_map_.end());
    }

    std::size_t getOption(const string_type & name, Option *& option) {
        auto iter = this->option_map_.find(name);
        if (iter != this->option_map_.end()) {
            std::size_t option_id = iter->second;
            if (option_id < this->option_list_.size()) {
                option = (Option *)&this->getOptionById(option_id);
                return option_id;
            }
        }
        return Option::NotFound;
    }

    std::size_t getOption(const string_type & name, const Option *& option) const {
        auto iter = this->option_map_.find(name);
        if (iter != this->option_map_.end()) {
            std::size_t option_id = iter->second;
            if (option_id < this->option_list_.size()) {
                option = (const Option *)&this->getOptionById(option_id);
                return option_id;
            }
        }
        return Option::NotFound;
    }

public:
    static bool isWideString() {
        return (sizeof(char_type) != 1);
    }

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

    std::size_t arg_count() const {
        return this->arg_list_.size();
    }

    std::vector<string_type> & arg_list() {
        return this->arg_list_;
    }

    const std::vector<string_type> & arg_list() const {
        return this->arg_list_;
    }

    string_type & getAppName() {
        return this->app_name_;
    }

    const string_type & getAppName() const {
        return this->app_name_;
    }

    string_type & getDisplayName() {
        return this->display_name_;
    }

    const string_type & getDisplayName() const {
        return this->display_name_;
    }

    string_type & getVersion() {
        return this->version_;
    }

    const string_type & getVersion() const {
        return this->version_;
    }

    void setDisplayName(const string_type & display_name) {
        this->display_name_ = display_name;
    }

    void setVersion(const string_type & version) {
        this->version_ = version;
    }

    std::size_t visitOrder(const string_type & name) const {
        const Option * option = nullptr;
        std::size_t option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return option->variable.state.order;
        } else {
            return Option::NotFound;
        }
    }

    bool isVisited(const string_type & name) const {
        const Option * option = nullptr;
        std::size_t option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return (option->variable.state.visited != 0);
        } else {
            return false;
        }
    }

    bool hasAssigned(const string_type & name) const {
        const Option * option = nullptr;
        std::size_t option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return (option->variable.state.assigned != 0);
        } else {
            return false;
        }
    }

    bool getVariableState(const string_type & name, VariableState & variable_state) const {
        const Option * option = nullptr;
        std::size_t option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            variable_state = option->variable.state;
            return true;
        } else {
            return false;
        }
    }

    bool hasVariable(const string_type & name) const {
        return this->hasOption(name);
    }

    bool readVariable(const string_type & name, Variable & variable) const {
        const Option * option = nullptr;
        std::size_t option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            variable = option->variable;
            return true;
        } else {
            return false;
        }
    }

    bool readVariable(const string_type & name, variant_t & value) const {
        const Option * option = nullptr;
        std::size_t option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            value = option->variable.value;
            return true;
        } else {
            return false;
        }
    }

    Variable & getVariable(const string_type & name) {
        Option * option = nullptr;
        std::size_t option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return option->variable;
        } else {
            return this->empty_variable_;
        }
    }

    const Variable & getVariable(const string_type & name) const {
        Option * option;
        std::size_t option_id = this->getOption(name, option);
        if (option_id != Option::NotFound) {
            assert(option != nullptr);
            return option->variable;
        } else {
            return this->empty_variable_;
        }
    }

    void setVariable(const string_type & name, Variant & value) {
        auto iter = this->option_map_.find(name);
        if (iter != this->option_map_.end()) {
            std::size_t option_id = iter->second;
            if (option_id < this->option_list_.size()) {
                Option & option = this->getOptionById(option_id);
                option.variable.value = value;;
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

    void printVersion() const {
        printf("\n");
        if (!is_null_or_empty(this->display_name_)) {
            printf("%s v%s\n", this->display_name_.c_str(), this->version_.c_str());
        } else {
            if (!is_null_or_empty(this->app_name_)) {
                printf("%s v%s\n", this->app_name_.c_str(), this->version_.c_str());
            } else {
                printf("No-name program v%s\n", this->version_.c_str());
            }
        }
        printf("\n");
    }

    void printUsage() const {
        for (auto iter = this->option_desc_list_.begin();
             iter != this->option_desc_list_.end(); ++iter) {
            const OptionsDescription & desc = *iter;
            desc.print();
        }
    }

    int parseArgs(int argc, char_type * argv[]) {
        int err_code = Error::NoError;
        string_type last_arg;

        this->arg_list_.clear();
        this->arg_list_.push_back(argv[0]);

        this->app_name_ == this_type::getAppName(argv);

        int i = 1;
        while (i < argc) {
            std::size_t start_pos, end_pos;
            string_type arg_name;

            string_type arg = argv[i];
            this->arg_list_.push_back(argv[i]);

            std::size_t separator_pos = arg.find(char_type('='));
            if (separator_pos != string_type::npos) {
                if (separator_pos == 0) {
                    // Skip error format
                    err_code = Error::CmdLine_IllegalFormat;
                    i++;
                    continue;
                }
                assert(separator_pos > 0);
                end_pos = separator_pos;
            } else {
                end_pos = arg.size();
            }

            char head_char = arg[0];
            assert(head_char != char_type(' '));
            if (head_char == char_type('-')) {
                if (arg[1] == char_type('-')) {
                    // --name=abc
                    start_pos = 2;
                } else {
                    // -n=abc
                    start_pos = 1;
                }
                Variable variable;
                string_copy(arg_name, arg, start_pos, end_pos);
                if (arg_name.size() > 0) {
                    bool exists = this->hasVariable(arg_name);
                    if (exists) {
                        Variable & variable = this->getVariable(arg_name);
                        variable.name = arg_name;
                        variable.state.order = i;
                        variable.state.visited = 1;
                        last_arg = arg_name;
                    } else {
                        // Unknown argument
                        err_code = Error::CmdLine_UnknownArgument;
                    }
                }
            } else {
                // Unrecognized argument
                err_code = Error::CmdLine_UnrecognizedArgument;
            }

            // Scan next argument
            i++;
        }
        return err_code;
    }
};

typedef BasicConfig<char>       Config;
typedef BasicCmdLine<char>      CmdLine;

typedef BasicConfig<wchar_t>    ConfigW;
typedef BasicCmdLine<wchar_t>   CmdLineW;

} // namespace app

#endif // APP_MANAGER_H
