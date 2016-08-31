#ifndef JSON_ERROR_H
#define JSON_ERROR_H

#define _JSON_BEGIN namespace json {
#define _JSON_END   }
#define _JSON   ::json::

#include <string>

_JSON_BEGIN

enum ErrorType
{
    /// 下面error开头的异常是在Parse过程中可能抛出的
    error_empty,    /// 待解析字符串为空
    error_escape,   /// 无效的转义字符
    error_quote,    /// 不匹配的双引号
    error_brack,    /// 不匹配的方括号
    error_brace,    /// 不匹配的花括号
    error_mismatch, /// 不匹配的双引号/方括号/小括号/花括号
    error_comma,    /// 不合法的逗号（位置）
    error_pair,     /// 不合法的键值对
    error_badnum,   /// 不合法的数字字符串
    error_literal,  /// 错误的字面值（true/false/null）

    /// 下面两个异常则是在使用Value过程中可能抛出的
    deref_nullptr,  /// 试图使用一个空Value对象的数据
    json_bad_cast   /// 试图将Value解释成一个不匹配的Json类
};


class JsonError
{
public:
    JsonError(ErrorType t): type(t) {}

    std::string What() const;

    int Code() const { return type; }

private:
    ErrorType type;
};


_JSON_END
#endif // JSON_ERROR_H
