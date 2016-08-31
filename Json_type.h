#ifndef JSON_TYPE_H
#define JSON_TYPE_H

#define _JSON_BEGIN namespace json {
#define _JSON_END   }
#define _JSON   ::json::

#include <utility>
#include <vector>
#include <map>
#include <string>
#include <initializer_list>
#include "Json_string.h"


#define DECLARE_IMPL(_ClassName, _JsonType) \
    friend class Value; \
    static _ClassName doParse(const SubString&); \
    JsonType Type() const { return _JsonType; } \
    _ClassName *clone() const & { return new _ClassName(*this); } \
    _ClassName *clone() && { return new _ClassName(std::move(*this));}


_JSON_BEGIN

inline bool IsSpace(char c)
{
    return c == ' '  || c == '\t' || c == '\n'
        || c == '\r' || c == '\v' || c == '\f';
}

inline bool IsCntrl(char c)
{
    return c >= '\x00' && c <= '\x1f';
}

/// 删除字符串首尾的空白字符
std::string erase_head_tail_ws(const std::string &s);
/// 删除Array或Object字符串中（不包括双引号之内的）所有空白符
std::string erase_all_whitespace(const std::string &s);
/// 在Array或Object字符串中寻找下一个逗号分隔符
auto next_comma(const SubString &subStr) ->decltype(subStr.first);


enum JsonType
{
    string_type, number_type, object_type, array_type,
    true_type, false_type, null_type
};



class Value_base
{
    friend class Value;
    friend class Object;
    virtual Value_base *clone() const & = 0;
    virtual Value_base *clone() && = 0;
    virtual JsonString Serialize() const = 0;
    virtual JsonString doFormat(unsigned nest,
                                const JsonString &padstr) const
                                { return Serialize(); }
    virtual JsonType Type() const = 0;

protected:

    Value_base() = default;
    Value_base(const Value_base &) = default;
    Value_base(Value_base &&) = default;
    Value_base &operator =(const Value_base &) = default;
    Value_base &operator =(Value_base &&) = default;
    virtual ~Value_base() = default;

public:

    /// 格式化成可读性较好的json格式字符串
    JsonString Format(const JsonString &padstr = "    ")
        { return doFormat(0, padstr); }
};



class String : public Value_base
{
    friend class Object;
    friend bool operator==(const String &lhs, const String &rhs);
    friend bool operator!=(const String &lhs, const String &rhs);
    friend bool operator< (const String &lhs, const String &rhs);
    friend bool operator<=(const String &lhs, const String &rhs);
    friend bool operator> (const String &lhs, const String &rhs);
    friend bool operator>=(const String &lhs, const String &rhs);

public:
    static String Parse(const JsonString &);
    JsonString Serialize() const;

    String() = default;
    String(const char *cp):       str(cp) {}
    String(const std::string &s): str(s) {}
    String(std::string &&s):      str(std::move(s)) {}

    std::string to_string() const { return str; }

    void clear() { str.clear(); }

private:
    DECLARE_IMPL(String, string_type)
     ///由于small string optimization，
     ///当有非常多的小String元素时会浪费大量内存！
    std::string str;
};



class NumberImpl;
class Number : public Value_base
{
public:
    static Number Parse(const JsonString &);
    JsonString Serialize() const;

    Number();
    ~Number();
    Number(const Number &);
    Number(Number &&) noexcept;
    Number &operator=(const Number &);
    Number &operator=(Number &&) noexcept;

    /************************************************/
    Number(int i);
    Number(unsigned int u);
    Number(long l);
    Number(unsigned long ul);
    Number(long long ll);
    Number(unsigned long long ull);
    Number(float f);
    Number(double d);
    Number(long double ld);
    /// 能够保持浮点数输出（Serialze结果）中有效数字个数
    Number(float f, int p);
    Number(double d, int p);
    Number(long double ld, int p);
    /************************************************/

    /************************************************/
    int                to_int()        const;
    unsigned int       to_uint()       const;
    long               to_long()       const;
    unsigned long      to_ulong()      const;
    long long          to_longlong()   const;
    unsigned long long to_ulonglong()  const;
    float              to_float()      const;
    double             to_double()     const;
    long double        to_longdouble() const;
    /************************************************/

private:
    DECLARE_IMPL(Number, number_type)
    NumberImpl* pImpl; /// pImpl可能为空
};



class Value;
class Object : public Value_base,
               public std::map<String, Value>
{
public:
    static Object Parse(const JsonString &);
    JsonString Serialize() const;
    using std::map<String, Value>::map;

private:
    DECLARE_IMPL(Object, object_type)

    static std::pair<String, Value>
                        parse_pair(const SubString &);
    JsonString doFormat(unsigned nest,
                        const JsonString &padstr) const;
};



class Array : public Value_base,
              public std::vector<Value>
{
public:
    static Array Parse(const JsonString &);
    JsonString Serialize() const;
    using std::vector<Value>::vector;

private:
    DECLARE_IMPL(Array, array_type)
    JsonString doFormat(unsigned nest,
                        const JsonString &padstr) const;
};



class True : public Value_base
{
public:
    static True Parse(const JsonString &);
    JsonString Serialize() const { return "true"; }
    True() = default;

private:
    DECLARE_IMPL(True, true_type)
};



class False : public Value_base
{
public:
    static False Parse(const JsonString &);
    JsonString Serialize() const { return "false"; }
    False() = default;

private:
    DECLARE_IMPL(False, false_type)
};



class Null : public Value_base
{
public:
    static Null Parse(const JsonString &);
    JsonString Serialize() const { return "null"; }
    Null() = default;

private:
    DECLARE_IMPL(Null, null_type)
};  const Null null;



class Value
{
    friend class Object;
    friend class Array;

public:

    static Value Parse(const JsonString &);
    JsonString Serialize() const;
    JsonType Type() const;
    JsonString Format(const JsonString &padstr = "    ") const;

    ~Value() { delete pbase; }
    Value(const Value &rhs):     pbase(rhs.pbase ? rhs.pbase->clone(): nullptr) {}
    Value(Value &&rhs) noexcept: pbase(rhs.pbase) { rhs.pbase = nullptr; }
    Value &operator=(const Value &rhs);
    Value &operator=(Value &&rhs) noexcept;

    Value():                           pbase(new Null()) {}
    Value(bool b):                     pbase(b ? static_cast<Value_base*>(new True()):
                                                 static_cast<Value_base*>(new False())) {}
    Value(int i):                      pbase(new Number(i)) {}
    Value(unsigned int u):             pbase(new Number(u)) {}
    Value(long l):                     pbase(new Number(l)) {}
    Value(unsigned long ul):           pbase(new Number(ul)) {}
    Value(long long ll):               pbase(new Number(ll)) {}
    Value(unsigned long long ull):     pbase(new Number(ull)) {}
    Value(float f):                    pbase(new Number(f)) {}
    Value(double d):                   pbase(new Number(d)) {}
    Value(long double ld):             pbase(new Number(ld)) {}
    Value(float f, int p):             pbase(new Number(f, p)) {}
    Value(double d, int p):            pbase(new Number(d, p)) {}
    Value(long double ld, int p):      pbase(new Number(ld, p)) {}
    Value(const std::string &s):       pbase(new String(s)) {}
    Value(std::string &&s):            pbase(new String(std::move(s))) {}
    Value(const char *cp):             pbase(new String(cp)) {}
    Value(const Value_base &base):     pbase(base.clone()) {}
    Value(Value_base &&base) noexcept: pbase(std::move(base).clone()) {}
    Value(std::initializer_list
            <Object::value_type> il):  pbase(new Object(il)) {}
    template<typename T = void>
    Value(std::initializer_list
            <Array::value_type> il):   pbase(new Array(il)) {}

    std::string        to_string()     const { auto p = getString(); return p->to_string(); }
    int                to_int()        const { auto p = getNumber(); return p->to_int(); }
    unsigned int       to_uint()       const { auto p = getNumber(); return p->to_uint(); }
    long               to_long()       const { auto p = getNumber(); return p->to_long(); }
    unsigned long      to_ulong()      const { auto p = getNumber(); return p->to_ulong(); }
    long long          to_longlong()   const { auto p = getNumber(); return p->to_longlong(); }
    unsigned long long to_ulonglong()  const { auto p = getNumber(); return p->to_ulonglong(); }
    float              to_float()      const { auto p = getNumber(); return p->to_float(); }
    double             to_double()     const { auto p = getNumber(); return p->to_double(); }
    long double        to_longdouble() const { auto p = getNumber(); return p->to_longdouble(); }

    bool is_String() const { return Type() == string_type; }
    bool is_Number() const { return Type() == number_type; }
    bool is_Object() const { return Type() == object_type; }
    bool is_Array () const { return Type() == array_type; }
    bool is_True  () const { return Type() == true_type; }
    bool is_False () const { return Type() == false_type; }
    bool is_Null  () const { return Type() == null_type; }

    String to_String() const & { auto p = getString(); return *p; }
    String to_String() &&      { auto p = getString(); return std::move(*p); }
    Number to_Number() const & { auto p = getNumber(); return *p; }
    Number to_Number() &&      { auto p = getNumber(); return std::move(*p); }
    Object to_Object() const & { auto p = getObject(); return *p; }
    Object to_Object() &&      { auto p = getObject(); return std::move(*p); }
    Array  to_Array () const & { auto p = getArray (); return *p; }
    Array  to_Array () &&      { auto p = getArray (); return std::move(*p); }
    True   to_True  () const & { auto p = getTrue  (); return *p; }
    True   to_True  () &&      { auto p = getTrue  (); return std::move(*p); }
    False  to_False () const & { auto p = getFalse (); return *p; }
    False  to_False () &&      { auto p = getFalse (); return std::move(*p); }
    Null   to_Null  () const & { auto p = getNull  (); return *p; }
    Null   to_Null  () &&      { auto p = getNull  (); return std::move(*p); }

private:

    String *getString() const;
    Number *getNumber() const;
    Object *getObject() const;
    Array  *getArray () const;
    True   *getTrue  () const;
    False  *getFalse () const;
    Null   *getNull  () const;

    static Value doParse(const SubString &);
    JsonString doFormat(unsigned nest,
                        const JsonString &padstr) const;
    void check() const;

    Value_base* pbase; /// pbase可能为空
};



inline
bool operator==(const String &lhs, const String &rhs)
    { return lhs.str == rhs.str; }

inline
bool operator!=(const String &lhs, const String &rhs)
    { return lhs.str != rhs.str; }

inline
bool operator< (const String &lhs, const String &rhs)
    { return lhs.str <  rhs.str; }

inline
bool operator<=(const String &lhs, const String &rhs)
    { return lhs.str <= rhs.str; }

inline
bool operator> (const String &lhs, const String &rhs)
    { return lhs.str >  rhs.str; }

inline
bool operator>=(const String &lhs, const String &rhs)
    { return lhs.str >= rhs.str; }

_JSON_END
#endif // JSON_TYPE_H
