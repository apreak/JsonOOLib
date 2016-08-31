#include "Json_error.h"

_JSON_BEGIN

std::string JsonError::What() const
{
    std::string ret;

    switch(type)
    {
    case ErrorType::error_empty:
        ret = "JsonError(error_empty): "
              "The expression is empty.";
        break;

    case ErrorType::error_escape:
        ret = "JsonError(error_escape): "
              "The expression contained an invalid escaped character, "
              "or contained an unescaped quote/control-character.";
        break;

    case ErrorType::error_quote:
        ret = "JsonError(error_quote): "
              "The expression contained mismatched \" and \".";
        break;

    case ErrorType::error_brack:
        ret = "JsonError(error_brack): "
              "The expression contained mismatched [ and ].";
        break;

    case ErrorType::error_brace:
        ret = "JsonError(error_brace): "
              "The expression contained mismatched { and }.";
        break;

    case ErrorType::error_mismatch:
        ret = "JsonError(error_mismatch): "
              "The expression contained mismatched "
              "\" and \", or [ and ], or ( and ), or { and }.";
              break;

    case ErrorType::error_comma:
        ret = "JsonError(error_comma): "
              "The expression contained an invalid comma.";
        break;

    case ErrorType::error_pair:
        ret = "JsonError(error_pair): "
              "The expression contained an invalid key-value pair.";
        break;

    case ErrorType::error_badnum:
        ret = "JsonError(error_badnum): "
              "The expression can not be explained as a number.";
        break;

    case ErrorType::error_literal:
        ret = "JsonError(error_literal): "
              "The expression cotained invalid literal value.";
        break;

    case ErrorType::deref_nullptr:
        ret = "JsonError(deref_nullptr): "
              "Try to use an empty Value.";
        break;

    case ErrorType::json_bad_cast:
        ret = "JsonError(json_bad_cast): "
              "Using dynamic_cast to cast Value to an incompatible JsonType.";
        break;
    }

    return ret;
}

_JSON_END
