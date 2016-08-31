#ifndef JSON_STRING_H
#define JSON_STRING_H

#define _JSON_BEGIN namespace json {
#define _JSON_END   }
#define _JSON   ::json::

#include <cstddef>
#include <utility>
#include <string>

_JSON_BEGIN

/// 类型别名，为了凸显出这是一个json字符串，
/// 或者说这是一个符合json“语法”的字符串。
/// JsonOOLib类解析和生成的都是json字符串。
typedef std::string JsonString;




/// JsonString的一部分，仅用于实现
struct SubString:
    public std::pair<const char *,
                     const char *>
{
    typedef const char* _iterator;
    typedef std::pair<_iterator, _iterator> _base;

    SubString(_iterator b, _iterator e): _base(b, e) {}

    std::size_t length() const { return second - first; }

    std::string str() const { return {first, second}; }
};


_JSON_END
#endif // JSON_STRING_H
