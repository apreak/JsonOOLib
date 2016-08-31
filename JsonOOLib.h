#ifndef JSON_INCLUDED_H
#define JSON_INCLUDED_H

#include "Json_error.h"
#include "Json_string.h"
#include "Json_type.h"


#define USING_JSON_UTILITIES \
using json::String; \
using json::Number; \
using json::Object; \
using json::Array; \
using json::True; \
using json::False; \
using json::Null; \
using json::Value; \
using json::JsonString; \
using json::JsonError; \
using json::ErrorType; \
using json::JsonType;


#endif // JSON_INCLUDED_H
