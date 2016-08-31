#include <stack>
#include <string>
#include <sstream>
#include <cstring>
#include "Json_error.h"
#include "Json_string.h"
#include "Json_type.h"

_JSON_BEGIN


/// 删除字符串首尾的空白符「\u0020\t\n\r\v\f」
std::string erase_head_tail_ws(const std::string &s)
{
    auto b = s.cbegin(), e = s.cend();

    while(b != e && IsSpace(*b)) ++ b;
    while(e != b && IsSpace(*(e-1))) --e;

    return {b, e};
}


/**************************************
 删除字符串中（不包括双引号之内的）所有空白符「\u0020\t\n\r\v\f」
 算法描述：
 1、依次遍历每个字符
    （1.1）如果不是空白字符，或者该字符在双引号之内，将它添加到结果字符串中，否则不处理
    （1.2）确定当前是否处于双引号之内：
                如果遇到「"」且其不属于转义序列的一部分，则不是即将进入双引号，就是即将离开双引号
    （1.3）确定下个字符是否属于转义序列的一部分：
                如果当前处于双引号之内，且当前字符为「\」
                （1.3.1）如果当前字符「\」属于转义序列，则下个字符不属于转义序列的一部分
                （1.3.2）否则，下个字符属于转义序列的一部分
 2、结束，返回结果

**************************************/
std::string erase_all_whitespace(const std::string &s)
{
    bool inQuote = false, nextEscape = false;
    std::string ret;
    for (decltype(s.size()) i = 0; i != s.size(); ++i)
    {
        if (!IsSpace(s[i]) || inQuote)
        ret.push_back(s[i]);

        inQuote ^= (s[i] == '\"') && !nextEscape;
        nextEscape = inQuote && s[i] == '\\' && !nextEscape;
    }
    return ret;
}



auto next_comma(const SubString &subStr) ->decltype(subStr.first)
{
    std::stack<char> letters;
    bool inQuote = false, nextEscape = false;

    for(auto b = subStr.first, e = subStr.second; b != e; ++b)
    {
        inQuote ^= (*b == '\"') && !nextEscape;
        nextEscape = inQuote && *b == '\\' && !nextEscape;
        if(inQuote) continue;

        if(*b == ',' && letters.empty())
            return b;

        switch(*b)
        {
        case '[': case '{':
            letters.push(*b); break;

        case ']':
            if(letters.empty() || letters.top() != '[')
                throw JsonError(error_brack);
            else
                letters.pop();
            break;

        case '}':
            if(letters.empty() || letters.top() != '{')
                throw JsonError(error_brace);
            else
                letters.pop();
            break;
        }
    }
    if(!letters.empty())
        throw JsonError(error_mismatch);
    return subStr.second;
}



String String::Parse(const JsonString &js)
{
    auto ret = erase_head_tail_ws(js);
    SubString subStr(ret.c_str(), ret.c_str() + ret.size());
    return doParse(subStr);
}


/**************************************
 String::Parse算法说明：
 1、如果字符串为空，抛出 error_empty；
 2、如果字符串长度为1，或首尾字符不是「"」，抛出 error_quote；
 3、遍历字符串，将它们依次添加到结果字符串中：
    （3.1）当遇到未转义的「"」或控制字符「\u0000-\u001f」,抛出 error_escape
    （3.2）当遇到「\」时，其后字母如果不是「"\/bfnrtu」中的一个，抛出 error_escape
            （3.2.1）如果「\」紧跟着「"\/bfnrt」中的一个，将转义序列所代表的字符添加到结果字符串中
            （3.2.2）如果「\」紧跟着「u」，则其后的四个字符被解析成转义序列，
                     并将转义序列所代表的字符添加到字符串中。如果解析失败，抛出error_escape
 4、结束，返回结果

**************************************/
String String::doParse(const SubString &subStr)
{
    if(subStr.length() == 0)
        throw JsonError(error_empty);

    if(subStr.length() == 1
            || *subStr.first != '\"' || *(subStr.second - 1) != '\"')
        throw JsonError(error_quote);

    std::string content;

    for(auto b = subStr.first + 1, e = subStr.second - 1; b != e; )
    {
        if(*b == '\"' || IsCntrl(*b))
            throw JsonError(error_escape);

        if(*b == '\\')
        {
            if(b + 1 == e)
                throw JsonError(error_escape);
            ++b;

            switch (*b)
            {
            case '\"':
                content.push_back('\"');
                ++b;
                break;

            case '\\':
                content.push_back('\\');
                ++b;
                break;

            case '/':
                content.push_back('/');
                ++b;
                break;

            case 'b':
                content.push_back('\b');
                ++b;
                break;

            case 'f':
                content.push_back('\f');
                ++b;
                break;

            case 'n':
                content.push_back('\n');
                ++b;
                break;

            case 'r':
                content.push_back('\r');
                ++b;
                break;

            case 't':
                content.push_back('\t');
                ++b;
                break;

            case 'u':
            {
                ++b;  /// 指向转义序列中数字部分的第一个字符
                if(b + 4 > e)
                    throw JsonError(error_escape);

                std::string hexstr(b, b + 4);
                if(hexstr.find_first_not_of("0123456789AaBbCcDdEeFf")
                        != std::string::npos)
                    throw JsonError(error_escape);

                std::istringstream stream(hexstr);
                unsigned n;
                stream >> std::hex >> n;
                content.push_back(static_cast<char>(n));
                b += 4; /// 跳过转义序列的数字部分（4个十六进制字符）
            }
            break;

            default:
                throw JsonError(error_escape);
                break;
            }
        }
        else
        {
            content.push_back(*b);
            ++b;
        }
    }

    return String(content);
}



/**************************************
 String::Serialize算法说明：
 1、遍历字符串，将它们一次添加到结果字符串中；
    （1.1）如果遇到「\"\\\/\b\f\n\r\t」时，将它们的转义序列添加到结果字符串中；
    （1.2）如果遇到控制字符「\u0000-\u001f」，取它们对应的16进制转义序列；
 2、在结果字符串的首尾添加「"」;
 3、结束，返回结果

**************************************/
JsonString String::Serialize() const
{
    std::string ret = "\"";

    for (auto c : str)
    {
        switch (c)
        {
        case '\"':
            ret += "\\\""; break;

        case '\\':
            ret += "\\\\"; break;

        case '/':
            ret += "/"; break;  /// 注意：「/」按原样输出，这与json.org上有些不一样

        case '\b':
            ret += "\\b"; break;

        case '\f':
            ret += "\\f"; break;

        case '\n':
            ret += "\\n"; break;

        case '\r':
            ret += "\\r"; break;

        case '\t':
            ret += "\\t"; break;

        default:
            if (IsCntrl(c))
            {
                std::ostringstream buffer;
                buffer << std::hex << static_cast<unsigned>(c);

                std::string hex = buffer.str();

                /// 如果c是char或者wchar_t类型的，则此处不会抛出异常
                ///if (hex.size() > 4)
                ///    throw JsonError(error_escape);

                hex.insert(hex.begin(), 4 - hex.size(), '0');
                ret += "\\u" + hex;
            }
            else
            {
                ret.push_back(c);
            }
            break;
        }
    }

    return ret + "\"";
}



Object Object::Parse(const JsonString &js)
{
    auto ret = erase_all_whitespace(js);
    SubString subStr(ret.c_str(), ret.c_str() + ret.size());
    return doParse(subStr);
}


/**************************************
 Object::Parse算法说明：
 1、如果字符串为空，抛出 error_empty；
 2、如果字符串长度为1，或首尾字符不是「{」、「}」，抛出 error_brace;
 3、遍历「{}」之间的字符串，调用next_comma找到下一个逗号位置或者结束位置，
    将逗号之间的“键值对“添加到结果map中
 4、结束，返回结果

**************************************/
Object Object::doParse(const SubString &subStr)
{
    if(subStr.length() == 0)
        throw JsonError(error_empty);

    if(subStr.length() == 1
       || *subStr.first != '{' || *(subStr.second - 1) != '}')
        throw JsonError(error_brace);

    Object ret;
    for(auto b = subStr.first + 1, e = subStr.second - 1; b != e; )
    {
        auto comma = next_comma(SubString(b, e));
        if(b == comma || comma + 1 == e)
            throw JsonError(error_comma);

        auto p = parse_pair(SubString(b, comma));
        ret.insert(std::move(p));

        if(comma == e)
            break;
        else
            b = comma + 1;
    }
    return ret;
}


/**************************************
 Object::parse_pair算法说明：
 1、如果字符串为空，抛出 error_empty；
 2、如果字符串长度为1，抛出 error_pair；
 3、遍历字符串，如果找到冒号且不在双引号之内，将冒号两边的字符串分别解析成String和Value；
 4、结束，返回结果。

**************************************/
std::pair<String, Value>
    Object::parse_pair(const SubString &subStr)

{
    if(subStr.length() == 0)
        throw JsonError(error_empty);
    if(subStr.length() == 1)
        throw JsonError(error_pair);

    bool inQuote = false, nextEscape = false;
    auto b = subStr.first, e = subStr.second;
    for(; b != e; ++b)
    {
        inQuote ^= (*b == '\"') && !nextEscape;
        nextEscape = inQuote && *b == '\\' && !nextEscape;

        if(inQuote) continue;
        if(*b == ':') break;
    }

    if(b == e || b == subStr.first || b + 1 == subStr.second)
        throw JsonError(error_pair);

    String key = String::doParse(SubString(subStr.first, b));
    Value value = Value::doParse(SubString(++b, e));

    return {std::move(key), std::move(value)};
}



JsonString Object::Serialize() const
{
    std::string ret;
    for(auto it = obj.cbegin(); it != obj.cend(); ++it)
    {
        if(it != obj.cbegin())
            ret += ",";
        ret += it->first.Serialize() + ":" + it->second.Serialize();
    }
    return "{" + ret + "}";
}


/**************************************
 Object::doFormat算法说明：
 1、如果对象为空，则原样输出
 2、如果对象不为空，
    （2.1）输出{并换行，
    （2.2）每一行输出一个元素，并使用（nest + 1）* padstr填充左侧开头。
    （2.3）输出}，并使用nest * padstr填充左侧开头
 4、结束，返回结果。

**************************************/
JsonString Object::doFormat(unsigned nest,
                            const JsonString &padstr) const
{
    if(obj.empty())
        return Serialize();

    std::string ret;
    for(auto it = obj.cbegin(); it != obj.cend(); ++it)
    {
        if(it != obj.cbegin())
            ret += ",\n";
        for(unsigned index = 0; index != nest + 1; ++index)
            ret += padstr;
        ret += it->first.doFormat(nest + 1, padstr);
        ret += ": ";
        ret += it->second.doFormat(nest + 1, padstr);
    }

    ret = "{\n" + ret + "\n";
    for(unsigned index = 0; index != nest; ++index)
        ret += padstr;
    ret += "}";
    return ret;
}




Array Array::Parse(const JsonString &js)
{
    auto ret = erase_all_whitespace(js);
    SubString subStr(ret.c_str(), ret.c_str() + ret.size());
    return doParse(subStr);
}



/**************************************
 Array::Parse算法说明：
 1、如果字符串为空，抛出 error_empty；
 2、如果字符串长度为1，或首尾字符不是「[」、「]」，抛出 error_brack;
 3、遍历「[]」之间的字符串，调用next_comma找到下一个逗号位置或者结束位置，
    将逗号之间的Value添加到结果vector中
 4、结束，返回结果

**************************************/
Array Array::doParse(const SubString &subStr)
{
    if(subStr.length() == 0)
        throw JsonError(error_empty);

    if(subStr.length() == 1
       || *subStr.first != '[' || *(subStr.second - 1) != ']')
       throw JsonError(error_brack);

    Array ret;
    for(auto b = subStr.first + 1, e = subStr.second - 1; b != e;)
    {
        auto comma = next_comma(SubString(b, e));
        if(b == comma || comma + 1 == e)
            throw JsonError(error_comma);

        Value value = Value::doParse(SubString(b, comma));
        ret.push_back(std::move(value));

        if(comma == e)
            break;
        else
            b = comma + 1;
    }
    return ret;
}



JsonString Array::Serialize() const
{
    std::string ret;
    for (auto it = arr.cbegin(); it != arr.cend(); ++it)
    {
        if (it != arr.cbegin())
            ret += ",";
        ret += it->Serialize();
    }
    return "[" + ret + "]";
}



JsonString Array::doFormat(unsigned nest,
                           const JsonString &padstr) const
{
    if(arr.empty())
        return Serialize();

    std::string ret;
    for(auto it = arr.cbegin(); it != arr.cend(); ++it)
    {
        if(it != arr.cbegin())
            ret += ",\n";
        for(unsigned index = 0; index != nest + 1; ++index)
            ret += padstr;
        ret += it->doFormat(nest + 1, padstr);
    }
    ret = "[\n" + ret + "\n";
    for(unsigned index = 0; index != nest; ++index)
        ret += padstr;
    ret += "]";
    return ret;
}



True True::Parse(const JsonString &js)
{
    auto ret = erase_head_tail_ws(js);
    SubString subStr(ret.c_str(), ret.c_str() + ret.size());
    return doParse(subStr);
}


True True::doParse(const SubString &subStr)
{
    if(std::strncmp(subStr.first, "true", 4) != 0)
        throw JsonError(error_literal);
    return True();
}


False False::Parse(const JsonString &js)
{
    auto ret = erase_head_tail_ws(js);
    SubString subStr(ret.c_str(), ret.c_str() + ret.size());
    return doParse(subStr);
}


False False::doParse(const SubString &subStr)
{
    if(std::strncmp(subStr.first, "false", 5) != 0)
        throw JsonError(error_literal);
    return False();
}


Null Null::Parse(const JsonString &js)
{
    auto ret = erase_head_tail_ws(js);
    SubString subStr(ret.c_str(), ret.c_str() + ret.size());
    return doParse(subStr);
}


Null Null::doParse(const SubString &subStr)
{
    if(std::strncmp(subStr.first, "null", 4) != 0)
        throw JsonError(error_literal);
    return Null();
}




Value &Value::operator=(const Value &rhs)
{
    auto newp = rhs.pbase ?
        rhs.pbase->clone(): nullptr;
    delete pbase;
    pbase = newp;
    return *this;
}


Value &Value::operator=(Value &&rhs) noexcept
{
    if(this != &rhs)
    {
        delete pbase;
        pbase = rhs.pbase;
        rhs.pbase = nullptr;
    }
    return *this;
}



Value Value::Parse(const JsonString &js)
{
    auto ret = erase_all_whitespace(js);
    SubString subStr(ret.c_str(), ret.c_str() + ret.size());
    return doParse(subStr);
}


/**************************************
 Value::Parse算法说明：
 1、如果字符串为空，抛出 error_empty；
 2、如果字符串大小为1，解析成Number并返回
 3、如果字符串大小大于等于2：
    （3.1）如果首尾字符为「"」、「"」，解析成String并返回
    （3.2）如果首尾字符为「[」、「]」，解析成Array并返回
    （3.3）如果首尾字符为「{」、「}」，解析成Object并返回
 4、如果字符串大小为4，且等于"true"，返回True()；
 5、如果字符串大小为5，且等于"false"，返回False()；
 6、如果字符串大小为4，且等于"null"，返回Null();
 7、解析成Number并返回

**************************************/
Value Value::doParse(const SubString &subStr)
{
    if(subStr.length() == 0)
        throw JsonError(error_empty);

    if(subStr.length() == 1)
        return Number::doParse(subStr);

    char head = *(subStr.first), tail = *(subStr.second -1);

    if(head == '[' && tail == ']')
        return Array::doParse(subStr);

    if(head == '[' || head == ']')
        throw JsonError(error_brack);


    if(head == '{' && tail == '}')
        return Object::doParse(subStr);

    if(head == '{' || tail == '}')
        throw JsonError(error_brace);


    if(head == '\"' && tail == '\"')
        return String::doParse(subStr);

    if(head == '\"' || tail == '\"')
        throw JsonError(error_quote);


    if(subStr.length() == 4
       && std::strncmp(subStr.first, "true", 4) == 0)
        return True();

    if(subStr.length() == 5
       && std::strncmp(subStr.first, "false", 5) == 0)
        return False();

    if(subStr.length() == 4
       && std::strncmp(subStr.first, "null", 4) == 0)
       return Null();

    return Number::doParse(subStr);
}



void Value::check() const
{
    if(pbase == nullptr)
        throw JsonError(deref_nullptr);
}


JsonType Value::Type() const
{
    check();
    return pbase->Type();
}


JsonString Value::Serialize() const
{
    check();
    return pbase->Serialize();
}


JsonString Value::Format(const JsonString &padstr) const
{
    check();
    return pbase->Format(padstr);
}

JsonString Value::doFormat(unsigned nest,
                           const JsonString &padstr) const
{
    check();
    return pbase->doFormat(nest, padstr);
}

#define GETPOINTERIMPL(_FuncName, _ClassName) \
_ClassName *Value::_FuncName() const \
{ \
    auto p = dynamic_cast<_ClassName*>(pbase); \
    if(!p) \
        throw JsonError(json_bad_cast); \
    return p; \
}

GETPOINTERIMPL(getString, String)
GETPOINTERIMPL(getNumber, Number)
GETPOINTERIMPL(getObject, Object)
GETPOINTERIMPL(getArray, Array)
GETPOINTERIMPL(getTrue, True)
GETPOINTERIMPL(getFalse, False)
GETPOINTERIMPL(getNull, Null)


_JSON_END
