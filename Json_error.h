#ifndef JSON_ERROR_H
#define JSON_ERROR_H

#define _JSON_BEGIN namespace json {
#define _JSON_END   }
#define _JSON   ::json::

#include <string>

_JSON_BEGIN

enum ErrorType
{
    /// ����error��ͷ���쳣����Parse�����п����׳���
    error_empty,    /// �������ַ���Ϊ��
    error_escape,   /// ��Ч��ת���ַ�
    error_quote,    /// ��ƥ���˫����
    error_brack,    /// ��ƥ��ķ�����
    error_brace,    /// ��ƥ��Ļ�����
    error_mismatch, /// ��ƥ���˫����/������/С����/������
    error_comma,    /// ���Ϸ��Ķ��ţ�λ�ã�
    error_pair,     /// ���Ϸ��ļ�ֵ��
    error_badnum,   /// ���Ϸ��������ַ���
    error_literal,  /// ���������ֵ��true/false/null��

    /// ���������쳣������ʹ��Value�����п����׳���
    deref_nullptr,  /// ��ͼʹ��һ����Value���������
    json_bad_cast   /// ��ͼ��Value���ͳ�һ����ƥ���Json��
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
