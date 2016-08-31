#ifndef JSON_STRING_H
#define JSON_STRING_H

#define _JSON_BEGIN namespace json {
#define _JSON_END   }
#define _JSON   ::json::

#include <cstddef>
#include <utility>
#include <string>

_JSON_BEGIN

/// ���ͱ�����Ϊ��͹�Գ�����һ��json�ַ�����
/// ����˵����һ������json���﷨�����ַ�����
/// JsonOOLib����������ɵĶ���json�ַ�����
typedef std::string JsonString;




/// JsonString��һ���֣�������ʵ��
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
