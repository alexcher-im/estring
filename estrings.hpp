// Estring - intrusive C++ compile&run-time strings
// github.com/alexcher-im/estring
// copyright by alexcher-im (c) 2022, zlib license


#pragma once
#include <bit>
#include <cstdint>
#include <cstring>
#include <string>


// string hash
constexpr uint32_t ascii_string_hash(const char* string) { // polynomial hash with q=127 p=2**32-1
    uint32_t q = 127;
    uint32_t hash = 0;
    uint32_t temp = 1;

    for (int i = 0; *string != '\0'; ++string, ++i) {
        hash += temp * (uint8_t)*string;
        temp *= q;
    }

    return hash;
}


// string itself
template <char... Chars> // Chars are required to have \0 at the end
class EstrRawData
{
public:
    uint32_t needs_disposing : 1 = false;
    uint32_t length : 31 = sizeof...(Chars) - 1;
    uint32_t hash;
    char chars[sizeof...(Chars)] = {Chars...};

    consteval EstrRawData() {
        hash = ascii_string_hash(chars);
    }

    constexpr EstrRawData(int) {}
};


// user-visible string class
class EstrPtr
{
public:
    EstrRawData<'\0'>* _raw_data{}; // field is actually private, but I will not limit access just in case

    [[nodiscard]] uint32_t size() const {
        return _raw_data->length;
    }

    [[nodiscard]] const char* data() const {
        return _raw_data->chars;
    }

    [[nodiscard]] bool needs_disposing() const {
        return _raw_data->needs_disposing;
    }

    [[nodiscard]] uint32_t hash() const {
        return _raw_data->hash;
    }

    [[nodiscard]] const std::string_view view() const {
        return {data(), size()};
    }

    void destroy() {
        if (needs_disposing())
            delete[] (char*) _raw_data;
    }

    static EstrPtr _create_from_raw_data(EstrRawData<'\0'>* raw_data) {
        auto estr = (EstrPtr*) alloca(sizeof(EstrPtr));
        estr->_raw_data = raw_data;
        return std::move(*estr);
    }

protected:
    EstrPtr() = default;
};


// user-visible unique string class
class EstrUniquePtr;
inline EstrUniquePtr estr_unique_new(std::string_view string);
class EstrUniquePtr : public EstrPtr
{
public:
    EstrUniquePtr(EstrUniquePtr&& old_ptr) {
        _raw_data = old_ptr._raw_data;
        old_ptr._raw_data = nullptr;
    }

    EstrPtr get() const {
        return EstrPtr::_create_from_raw_data(_raw_data);
    }

    EstrUniquePtr clone() {
        return estr_unique_new(view());
    }

    ~EstrUniquePtr() {
        get().destroy();
    }

    static EstrUniquePtr claim(EstrPtr ptr) {
        auto estr = (EstrUniquePtr*) alloca(sizeof(EstrUniquePtr));
        estr->_raw_data = ptr._raw_data;
        return std::move(*estr);
    }
};


// external string operators and functions
namespace std
{
    template<>
    struct hash<EstrPtr> {
        size_t operator()(const EstrPtr& string) const {
            return string.hash();
        }
    };

    ostream& operator<<(ostream& outs, const EstrPtr& string) {
        return outs << string.view();
    }

    std::string to_string(const EstrPtr& string) {
        return std::string(string.view());
    }
}

inline bool operator==(const EstrPtr& a, const EstrPtr& b) {
    if (a.size() != b.size())
        return false;
    if (a.hash() != b.hash())
        return false;
    return !memcmp(a.data(), b.data(), a.size());
}

inline bool operator!=(const EstrPtr& a, const EstrPtr& b) {
    return !(a == b);
}


// only for compile-time strings
// unique class by which the full compile-time string interning works
template <char... Chars>
class EstrUniqueStorage
{
public:
    static constexpr EstrRawData<Chars...> data = {};
};


// recursive variadic generator (converts const char* to variadic template <char...>)
template<typename str_storage, int index, typename is_enabled, char... chars>
class TmpInstanceVariadicCharGenerator {};

template<typename str_storage, int index, char... chars>
class TmpInstanceVariadicCharGenerator<str_storage, index, std::true_type, chars...>
{
public:
    using type = typename TmpInstanceVariadicCharGenerator<
            str_storage,
            index + 1,
            typename std::conditional<(str_storage().size - 1 > index), std::true_type, std::false_type>::type,
            chars..., str_storage().value[index]>::type;
};

template<typename str_storage, int index, char... chars>
class TmpInstanceVariadicCharGenerator<str_storage, index, std::false_type, chars...>
{
public:
    using type = EstrUniqueStorage<chars...>;
};


// string suffix variadic generator (converts const char* to variadic template <char...>)
#ifndef _MSC_VER
// todo silence clang warning
template<typename Char, Char... chars>
auto operator ""_estr() {
    return EstrPtr::_create_from_raw_data((EstrRawData<'\0'>*) &EstrUniqueStorage<chars..., '\0'>::data);
}
#endif


// macro for generating string from literal

// MSVC supports declaring a type inside decltype()
// GCC supports string postfix operator with variadic char template (GNU extension)
// Clang supports both (but postfix is faster to compile)

#ifdef _MSC_VER
#define ESTR(literal) EstrPtr::_create_from_raw_data((EstrRawData<'\0'>*) & \
TmpInstanceVariadicCharGenerator< \
        decltype([](){struct {const char* value = literal; size_t size = sizeof(literal);} tmp; \
                      return tmp; }()), \
        0, std::true_type>::type::data)

#elif defined(__clang__) || defined(__GNUC__)
#define ESTR(literal) literal##_estr

#endif

// functions to create runtime string
inline EstrPtr estr_new(std::string_view string) {
    auto ptr = (EstrRawData<'\0'>*) new char[std::max(sizeof(EstrRawData<'\0'>), offsetof(EstrRawData<'\0'>, chars) + string.size() + 1)];
    new (ptr) EstrRawData<'\0'>(0);

    ptr->needs_disposing = true;
    ptr->length = string.size();
    memcpy(ptr->chars, string.data(), string.size());
    ptr->chars[string.size()] = '\0';
    ptr->hash = ascii_string_hash(ptr->chars);

    return EstrPtr::_create_from_raw_data(ptr);
}

inline EstrUniquePtr estr_unique_new(std::string_view string) {
    auto estr = estr_new(string);
    return EstrUniquePtr::claim(estr);
}
