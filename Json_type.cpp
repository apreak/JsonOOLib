#include <stack>
#include <string>
#include <sstream>
#include <cstring>
#include "Json_error.h"
#include "Json_string.h"
#include "Json_type.h"

_JSON_BEGIN


/// ɾ���ַ�����β�Ŀհ׷���\u0020\t\n\r\v\f��
std::string erase_head_tail_ws(const std::string &s)
{
    auto b = s.cbegin(), e = s.cend();

    while(b != e && IsSpace(*b)) ++ b;
    while(e != b && IsSpace(*(e-1))) --e;

    return {b, e};
}


/**************************************
 ɾ���ַ����У�������˫����֮�ڵģ����пհ׷���\u0020\t\n\r\v\f��
 �㷨������
 1�����α���ÿ���ַ�
    ��1.1��������ǿհ��ַ������߸��ַ���˫����֮�ڣ�������ӵ�����ַ����У����򲻴���
    ��1.2��ȷ����ǰ�Ƿ���˫����֮�ڣ�
                ���������"�����䲻����ת�����е�һ���֣����Ǽ�������˫���ţ����Ǽ����뿪˫����
    ��1.3��ȷ���¸��ַ��Ƿ�����ת�����е�һ���֣�
                �����ǰ����˫����֮�ڣ��ҵ�ǰ�ַ�Ϊ��\��
                ��1.3.1�������ǰ�ַ���\������ת�����У����¸��ַ�������ת�����е�һ����
                ��1.3.2�������¸��ַ�����ת�����е�һ����
 2�����������ؽ��

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
 String::Parse�㷨˵����
 1������ַ���Ϊ�գ��׳� error_empty��
 2������ַ�������Ϊ1������β�ַ����ǡ�"�����׳� error_quote��
 3�������ַ�����������������ӵ�����ַ����У�
    ��3.1��������δת��ġ�"��������ַ���\u0000-\u001f��,�׳� error_escape
    ��3.2����������\��ʱ�������ĸ������ǡ�"\/bfnrtu���е�һ�����׳� error_escape
            ��3.2.1�������\�������š�"\/bfnrt���е�һ������ת��������������ַ���ӵ�����ַ�����
            ��3.2.2�������\�������š�u�����������ĸ��ַ���������ת�����У�
                     ����ת��������������ַ���ӵ��ַ����С��������ʧ�ܣ��׳�error_escape
 4�����������ؽ��

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
                ++b;  /// ָ��ת�����������ֲ��ֵĵ�һ���ַ�
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
                b += 4; /// ����ת�����е����ֲ��֣�4��ʮ�������ַ���
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
 String::Serialize�㷨˵����
 1�������ַ�����������һ����ӵ�����ַ����У�
    ��1.1�����������\"\\\/\b\f\n\r\t��ʱ�������ǵ�ת��������ӵ�����ַ����У�
    ��1.2��������������ַ���\u0000-\u001f����ȡ���Ƕ�Ӧ��16����ת�����У�
 2���ڽ���ַ�������β��ӡ�"��;
 3�����������ؽ��

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
            ret += "/"; break;  /// ע�⣺��/����ԭ�����������json.org����Щ��һ��

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

                /// ���c��char����wchar_t���͵ģ���˴������׳��쳣
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
 Object::Parse�㷨˵����
 1������ַ���Ϊ�գ��׳� error_empty��
 2������ַ�������Ϊ1������β�ַ����ǡ�{������}�����׳� error_brace;
 3��������{}��֮����ַ���������next_comma�ҵ���һ������λ�û��߽���λ�ã�
    ������֮��ġ���ֵ�ԡ���ӵ����map��
 4�����������ؽ��

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
 Object::parse_pair�㷨˵����
 1������ַ���Ϊ�գ��׳� error_empty��
 2������ַ�������Ϊ1���׳� error_pair��
 3�������ַ���������ҵ�ð���Ҳ���˫����֮�ڣ���ð�����ߵ��ַ����ֱ������String��Value��
 4�����������ؽ����

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
 Object::doFormat�㷨˵����
 1���������Ϊ�գ���ԭ�����
 2���������Ϊ�գ�
    ��2.1�����{�����У�
    ��2.2��ÿһ�����һ��Ԫ�أ���ʹ�ã�nest + 1��* padstr�����࿪ͷ��
    ��2.3�����}����ʹ��nest * padstr�����࿪ͷ
 4�����������ؽ����

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
 Array::Parse�㷨˵����
 1������ַ���Ϊ�գ��׳� error_empty��
 2������ַ�������Ϊ1������β�ַ����ǡ�[������]�����׳� error_brack;
 3��������[]��֮����ַ���������next_comma�ҵ���һ������λ�û��߽���λ�ã�
    ������֮���Value��ӵ����vector��
 4�����������ؽ��

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
 Value::Parse�㷨˵����
 1������ַ���Ϊ�գ��׳� error_empty��
 2������ַ�����СΪ1��������Number������
 3������ַ�����С���ڵ���2��
    ��3.1�������β�ַ�Ϊ��"������"����������String������
    ��3.2�������β�ַ�Ϊ��[������]����������Array������
    ��3.3�������β�ַ�Ϊ��{������}����������Object������
 4������ַ�����СΪ4���ҵ���"true"������True()��
 5������ַ�����СΪ5���ҵ���"false"������False()��
 6������ַ�����СΪ4���ҵ���"null"������Null();
 7��������Number������

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
