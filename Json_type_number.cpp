#include <sstream>
#include <string>
#include <stdexcept>
#include "Json_error.h"
#include "Json_string.h"
#include "Json_type.h"

_JSON_BEGIN

class NumberImpl
{
    friend class Number;
    virtual JsonString Serialize() const = 0;
    virtual NumberImpl *clone() const & = 0;
    virtual NumberImpl *clone() && = 0;

    virtual int to_int() const = 0;
    virtual unsigned int to_uint() const = 0;
    virtual long to_long() const = 0;
    virtual unsigned long to_ulong() const = 0;
    virtual long long to_longlong() const = 0;
    virtual unsigned long long to_ulonglong() const = 0;
    virtual float to_float() const = 0;
    virtual double to_double() const = 0;
    virtual long double to_longdouble() const = 0;

protected:
    virtual ~NumberImpl() = default;
};



#define DERIVED_NUMBERIMPL(_ClassName, _Type) \
class _ClassName : public NumberImpl \
{ \
    friend class Number; \
    JsonString Serialize() const; \
    _ClassName *clone() const & { return new _ClassName(*this); } \
    _ClassName *clone() && { return new _ClassName(std::move(*this)); } \
 \
    int                to_int() const        { return static_cast<int>(val); } \
    unsigned int       to_uint() const       { return static_cast<unsigned int>(val); } \
    long               to_long() const       { return static_cast<long>(val); } \
    unsigned long      to_ulong() const      { return static_cast<unsigned long>(val); } \
    long long          to_longlong() const   { return static_cast<long long>(val); } \
    unsigned long long to_ulonglong() const  { return static_cast<unsigned long>(val); } \
    float              to_float() const      { return static_cast<float>(val); } \
    double             to_double() const     { return static_cast<double>(val); } \
    long double        to_longdouble() const { return static_cast<long double>(val); } \
 \
protected: \
    _ClassName(_Type t): val(t) {} \
    _Type val; \
}; \
 \
JsonString _ClassName::Serialize() const \
{ \
    std::ostringstream out; \
    out << val; \
    return out.str(); \
}


#define DECLARE_ENHANCED_FLOATER(_ClassName, _BaseName, _Type) \
class _ClassName : public _BaseName \
{ \
    friend class Number; \
    JsonString Serialize() const; \
    _ClassName *clone() const & { return new _ClassName(*this); } \
    _ClassName *clone() && { return new _ClassName(std::move(*this)); } \
 \
    _ClassName(_Type t, int p):_BaseName(t), prec(p) {} \
    int prec; \
}; \
 \
JsonString _ClassName::Serialize() const \
{ \
    std::ostringstream out; \
    out.precision(prec); \
    out << val; \
    return out.str(); \
}


DERIVED_NUMBERIMPL(INT, int)
DERIVED_NUMBERIMPL(UINT, unsigned int)
DERIVED_NUMBERIMPL(LONG, long)
DERIVED_NUMBERIMPL(ULONG, unsigned long)
DERIVED_NUMBERIMPL(LONGLONG, long long)
DERIVED_NUMBERIMPL(ULONGLONG, unsigned long long)
DERIVED_NUMBERIMPL(FLOAT, float)
DERIVED_NUMBERIMPL(DOUBLE, double)
DERIVED_NUMBERIMPL(LONGDOUBLE, long double)

DECLARE_ENHANCED_FLOATER(EHFLOAT, FLOAT, float)
DECLARE_ENHANCED_FLOATER(EHDOUBLE, DOUBLE, double)
DECLARE_ENHANCED_FLOATER(EHLONGDOUBLE, LONGDOUBLE, long double)


Number::Number(): pImpl(nullptr) {}
Number::~Number() { delete pImpl; }
Number::Number(const Number &rhs):      pImpl(rhs.pImpl ? rhs.pImpl->clone() : nullptr) {}
Number::Number(Number &&rhs) noexcept : pImpl(rhs.pImpl) { rhs.pImpl = nullptr; }

Number &Number::operator=(const Number &rhs)
{
    auto newp = rhs.pImpl ?
        rhs.pImpl->clone() : nullptr;
    delete pImpl;
    pImpl = newp;
    return *this;
}


Number &Number::operator=(Number &&rhs) noexcept
{
    if(this != &rhs)
    {
        delete pImpl;
        pImpl = rhs.pImpl;
        rhs.pImpl = nullptr;
    }
    return *this;
}


Number::Number(int i):                     pImpl(new INT(i)) {}
Number::Number(unsigned int u):            pImpl(new UINT(u)) {}
Number::Number(long l):                    pImpl(new LONG(l)) {}
Number::Number(unsigned long ul):          pImpl(new ULONG(ul)) {}
Number::Number(long long ll):              pImpl(new LONGLONG(ll)) {}
Number::Number(unsigned long long ull):    pImpl(new ULONGLONG(ull)) {}
Number::Number(float f):               pImpl(new FLOAT(f)) {}
Number::Number(double d):              pImpl(new DOUBLE(d)) {}
Number::Number(long double ld):        pImpl(new LONGDOUBLE(ld)) {}
Number::Number(float f, int p):        pImpl(new EHFLOAT(f, p)) {}
Number::Number(double d, int p):       pImpl(new EHDOUBLE(d, p)) {}
Number::Number(long double ld, int p): pImpl(new EHLONGDOUBLE(ld, p)) {}


///注意pImpl为空时的默认处理
int                Number::to_int() const        { return pImpl ? pImpl->to_int() : 0; }
unsigned int       Number::to_uint() const       { return pImpl ? pImpl->to_uint() : 0U; }
long               Number::to_long() const       { return pImpl ? pImpl->to_long() : 0L; }
unsigned long      Number::to_ulong() const      { return pImpl ? pImpl->to_ulong() : 0UL; }
long long          Number::to_longlong() const   { return pImpl ? pImpl->to_longlong() : 0LL; }
unsigned long long Number::to_ulonglong() const  { return pImpl ? pImpl->to_ulonglong() : 0ULL; }
float              Number::to_float() const      { return pImpl ? pImpl->to_float() : 0.0f; }
double             Number::to_double() const     { return pImpl ? pImpl->to_double() : 0.0; }
long double        Number::to_longdouble() const { return pImpl ? pImpl->to_longdouble() : 0.0L; }


Number Number::Parse(const JsonString &js)
{
    auto ret = erase_head_tail_ws(js);
    SubString subStr(ret.c_str(), ret.c_str() + ret.size());
    return doParse(subStr);
}


/**************************************
 Number::Parse算法说明：
 1、如果字符串为空，抛出 error_empty；
 2、如果字符串是单个字符（"+", "-", ".", "e", "E"）, 则抛出 error_badnum;
 3、检查字符串中是否包含「.eE」，从而确定是整数还是浮点数
    （3.1）如果是整数，检查第一个字符是否为「-」，从而确定是非负数还是负数
        （3.1.1）如果是非负数，调用stoull将其转换为unsigned long long类型的值
        （3.1.2）如果是负数，调用stoll将其转换为long long类型的值
    （3.2）如果是浮点数，调用stold将其转换为long double类型的值，
           并统计有效数字个数，从而保持精度。
 4、结束，返回结果

**************************************/
Number Number::doParse(const SubString &subStr)
{
    if(subStr.length() == 0)
        throw JsonError(error_empty);

    static const std::string sym = "+-.eE";
    static const auto npos = std::string::npos;

    if(subStr.length() == 1
       && sym.find(*subStr.first) != npos)
        throw JsonError(error_badnum);

    std::string num(subStr.first, subStr.second);
    std::size_t idx = 0;
    auto expIdx = num.find_first_of("eE");
    auto dotIdx = num.find('.');

    if(expIdx == npos && dotIdx == npos) /// 整数
    {
        if(num[0] != '-') /// 非负数
        {
            unsigned long long ull = 0;
            try{
                ull = std::stoull(num, &idx);
            }
            catch(std::exception &e){
                throw JsonError(error_badnum);
            }

            if(idx != num.size())
                throw JsonError(error_badnum);
            return Number(ull);
        }
        else              /// 负数
        {
            long long ll = 0;
            try{
                ll = std::stoll(num, &idx);
            }
            catch(std::exception &e){
                throw JsonError(error_badnum);
            }

            if(idx != num.size())
                throw JsonError(error_badnum);
            return Number(ll);
        }
    }
    else                                      /// 浮点数
    {
        long double ld = 0.0;
        try{
            ld = std::stold(num, &idx);
        }
        catch(std::exception &e){
            throw JsonError(error_badnum);
        }

        if(idx != num.size())
            throw JsonError(error_badnum);

        int p = 0;
        for(decltype(dotIdx) b = 0,
            e = (expIdx == npos ? num.size() : expIdx);
            b != e; ++b)
            if(std::isdigit(num[b])) ++p;
        return Number(ld, p);
    }
}


JsonString Number::Serialize() const
{
    return pImpl ? pImpl->Serialize() : JsonString("0");
}


_JSON_END
