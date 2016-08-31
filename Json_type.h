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
class Object : public Value_base /// It's a std::map!
{
public:
    static Object Parse(const JsonString &);
    JsonString Serialize() const;

    typedef std::map<String, Value> _Type;
    typedef _Type::iterator iterator;
    typedef _Type::const_iterator const_iterator;
    typedef _Type::size_type size_type;
    typedef _Type::difference_type difference_type;
    typedef _Type::value_type value_type;
    typedef _Type::reference reference;
    typedef _Type::const_reference const_reference;
    typedef _Type::key_type key_type;
    typedef _Type::mapped_type mapped_type;

    /************************************************/
    Object() = default;
    Object(std::initializer_list<value_type> il);
    template<typename _InputIterator>
    Object(_InputIterator b, _InputIterator e);
    /************************************************/


    /// 以下为map兼容的操作，与map的操作一一对应
    bool empty() const;
    size_type size() const;
    void swap(Object &);
    void clear();

    iterator begin() { return obj.begin(); }
    iterator end() { return obj.end(); }
    const_iterator begin() const { return obj.begin(); }
    const_iterator end() const { return obj.end(); }
    const_iterator cbegin() const { return obj.cbegin(); }
    const_iterator cend() const { return obj.cend(); }

    std::pair<iterator, bool> insert(const value_type &);
    template<typename _Pair>
    std::pair<iterator, bool> insert(_Pair &&p);
    void insert(std::initializer_list<value_type> il);
    template<typename _InputIterator>
    void insert(_InputIterator, _InputIterator);
    iterator insert(const_iterator _position, const value_type &);
    template<typename _Pair>
    iterator insert(const_iterator _position, _Pair &&p);

    size_type erase(const key_type &);
    iterator erase(const_iterator p);
    iterator erase(const_iterator b, const_iterator e);

    mapped_type &operator[](const key_type &);
    mapped_type &at(const key_type &);
    const mapped_type &at(const key_type &) const;

    /// find和一些成员函数根据对象是否为const进行重载
    iterator find(const key_type &);
    const_iterator find(const key_type &) const;
    size_type count(const key_type &) const;

    iterator lower_bound(const key_type &);
    const_iterator lower_bound(const key_type &) const;
    iterator upper_bound(const key_type &);
    const_iterator upper_bound(const key_type &) const;
    std::pair<iterator, iterator>
        equal_range(const key_type &);
    std::pair<const_iterator, const_iterator>
        equal_range(const key_type &) const;


private:
    DECLARE_IMPL(Object, object_type)

    static std::pair<String, Value>
                        parse_pair(const SubString &);
    JsonString doFormat(unsigned nest,
                        const JsonString &padstr) const;

    _Type obj; /// json::members -> std::map;
};



class Array : public Value_base /// It's a std::vector!
{
public:
    static Array Parse(const JsonString &);
    JsonString Serialize() const;

    typedef std::vector<Value> _Type;
    typedef _Type::iterator iterator;
    typedef _Type::const_iterator const_iterator;
    typedef _Type::size_type size_type;
    typedef _Type::difference_type difference_type;
    typedef _Type::value_type value_type;
    typedef _Type::reference reference;
    typedef _Type::const_reference const_reference;

    /************************************************/
    Array() = default;
    Array(std::initializer_list<value_type> il);
    template<typename _InputIterator>
    Array(_InputIterator b, _InputIterator e);
    explicit
    Array(size_type n);
    Array(size_type n, const value_type &);
    /************************************************/


    /// 以下为vector兼容的操作，与vector的操作一一对应
    bool empty() const;
    size_type size() const;
    void swap(Array &);
    void clear();

    iterator begin() { return arr.begin(); }
    iterator end() { return arr.end(); }
    const_iterator begin() const { return arr.begin(); }
    const_iterator end() const { return arr.end(); }
    const_iterator cbegin() const { return arr.cbegin(); }
    const_iterator cend() const { return arr.cend(); }

    void push_back(const value_type &);
    void push_back(value_type &&);
    /// 在C++11标准下，insert的位置参数可以用const_iterator来指示
    /// insert有左值和右值两个版本
    iterator insert(const_iterator p, const value_type &);
    iterator insert(const_iterator p, value_type &&);
    iterator insert(const_iterator p, size_type n, const value_type &);
    template<typename _InputIterator>
    iterator insert(const_iterator p, _InputIterator b, _InputIterator e);
    iterator insert(const_iterator p, std::initializer_list<value_type> il);

    void pop_back();
    iterator erase(const_iterator p);
    iterator erase(const_iterator b, const_iterator e);

    value_type &back();
    const value_type &back() const;
    value_type &front();
    const value_type &front() const;
    value_type &operator[](size_type n);
    const value_type &operator[](size_type n) const;
    value_type &at(size_type n);
    const value_type &at(size_type n) const;

    void resize(size_type n);
    void resize(size_type n, const value_type &);
    void shrink_to_fit();
    size_type capacity() const;
    void reserve(size_type n);

private:
    DECLARE_IMPL(Array, array_type)
    JsonString doFormat(unsigned nest,
                        const JsonString &padstr) const;

    _Type arr; /// json::elements -> std::vector
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


inline Object::
    Object(std::initializer_list<value_type> il):
    obj(il) {}


template<typename _InputIterator>
inline Object::
    Object(_InputIterator b, _InputIterator e):
    obj(b, e) {}


inline bool
    Object::empty() const { return obj.empty(); }


inline Object::size_type
    Object::size() const { return obj.size(); }


inline void
    Object::swap(Object &rhs) { obj.swap(rhs.obj); }


inline void
    Object::clear() { return obj.clear(); }


inline std::pair<Object::iterator, bool>
    Object::insert(const value_type &v)
    { return obj.insert(v); }


template<typename _Pair>
inline std::pair<Object::iterator, bool>
    Object::insert(_Pair &&p)
    { return obj.insert(std::forward<_Pair>(p)); }


inline void
    Object::insert(std::initializer_list<value_type> il)
    { obj.insert(il); }


template<typename _InputIterator>
inline void
    Object::insert(_InputIterator b, _InputIterator e)
    { obj.insert(b, e); }


inline Object::iterator
    Object::insert(const_iterator _position, const value_type &v)
    { return obj.insert(_position, v); }


template<typename _Pair>
inline Object::iterator
    Object::insert(const_iterator _position, _Pair &&p)
    { return obj.insert(_position, std::forward<_Pair>(p)); }


inline Object::size_type
    Object::erase(const key_type &k)
    { return obj.erase(k); }


inline Object::iterator
    Object::erase(const_iterator p)
    { return obj.erase(p); }


inline Object::iterator
    Object::erase(const_iterator b, const_iterator e)
    { return obj.erase(b, e); }


inline Object::mapped_type &
    Object::operator[](const key_type &k)
    { return obj.operator[](k); }


inline Object::mapped_type &
    Object::at(const key_type &k) { return obj.at(k); }


inline const Object::mapped_type &
    Object::at(const key_type &k) const { return obj.at(k); }


inline Object::iterator
    Object::find(const key_type &k) { return obj.find(k); }


inline Object::const_iterator
    Object::find(const key_type &k) const { return obj.find(k); }


inline Object::size_type
    Object::count(const key_type &k) const { return obj.count(k); }


inline Object::iterator
    Object::lower_bound(const key_type &k)
    { return obj.lower_bound(k); }


inline Object::const_iterator
    Object::lower_bound(const key_type &k) const
    { return obj.lower_bound(k); }


inline Object::iterator
    Object::upper_bound(const key_type &k)
    { return obj.upper_bound(k); }


inline Object::const_iterator
    Object::upper_bound(const key_type &k) const
    { return obj.upper_bound(k); }


inline
std::pair<Object::iterator, Object::iterator>
    Object::equal_range(const key_type &k)
    { return obj.equal_range(k); }


inline
std::pair<Object::const_iterator, Object::const_iterator>
    Object::equal_range(const key_type &k) const
    { return obj.equal_range(k); }


inline
void swap(Object &lhs, Object &rhs)
    { lhs.swap(rhs); }




inline Array::
    Array(std::initializer_list<value_type> il):
    arr(il) {}


template<typename _InputIterator>
inline Array::
    Array(_InputIterator b, _InputIterator e):
    arr(b, e) {}


inline Array::
    Array(size_type n): arr(n) {}


inline Array::
    Array(size_type n, const value_type &v):
    arr(n, v) {}


inline bool
    Array::empty() const{ return arr.empty(); }


inline Array::size_type
    Array::size() const { return arr.size(); }


inline void
    Array::swap(Array &rhs) { arr.swap(rhs.arr); }


inline void
    Array::clear() { arr.clear(); }


inline void
Array::push_back(const value_type &v)
    { arr.push_back(v); }


inline void
    Array::push_back(value_type &&v)
    { arr.push_back(std::move(v)); }


inline Array::iterator
    Array::insert(const_iterator p, const value_type &v)
    { return arr.insert(p, v); }


inline Array::iterator
    Array::insert(const_iterator p, value_type &&v)
    { return arr.insert(p, std::move(v)); }


inline Array::iterator
    Array::insert(const_iterator p,
                  size_type n, const value_type &v)
                  { return arr.insert(p, n, v); }


template<typename _InputIterator>
inline Array::iterator
    Array::insert(const_iterator p,
                  _InputIterator b, _InputIterator e)
                  { return arr.insert(p, b, e); }


inline Array::iterator
    Array::insert(const_iterator p,
                  std::initializer_list<value_type> il)
                  { return arr.insert(p, il); }


inline void
    Array::pop_back(){ arr.pop_back(); }


inline Array::iterator
    Array::erase(const_iterator p)
    { return arr.erase(p); }


inline Array::iterator
    Array::erase(const_iterator b, const_iterator e)
    { return arr.erase(b, e); }


inline Array::value_type &
    Array::back() { return arr.back(); }


inline const Array::value_type &
    Array::back() const { return arr.back(); }


inline Array::value_type &
    Array::front() { return arr.front(); }


inline const Array::value_type &
    Array::front() const { return arr.front(); }


inline Array::value_type &
    Array::operator[](size_type n)
    { return arr.operator[](n); }


inline const Array::value_type &
    Array::operator[](size_type n) const
    { return arr.operator[](n); }


inline Array::value_type &
    Array::at(size_type n) { return arr.at(n); }


inline const Array::value_type &
    Array::at(size_type n) const
    { return arr.at(n); }


inline void Array::resize(size_type n)
    { arr.resize(n); }


inline void
    Array::resize(size_type n, const value_type &v)
    { arr.resize(n, v); }


inline void Array::shrink_to_fit()
    { arr.shrink_to_fit(); }


inline
Array::size_type Array::capacity() const
    { return arr.capacity(); }


inline void
    Array::reserve(size_type n)
    { arr.reserve(n); }

_JSON_END
#endif // JSON_TYPE_H
