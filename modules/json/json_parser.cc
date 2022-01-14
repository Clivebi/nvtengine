#include "json_parser.hpp"

#include "../../engine/value.hpp"
namespace json {
#include "json.lex.cpp"
#include "json.tab.cpp"
void parser_json_error(json::JSONParser* parser, const char* s) {
    printf("parser_json error : %s on line:%d column:%d\n", s, yyget_lineno(parser->GetContext()),
           yyget_column(parser->GetContext()));
}
} // namespace json

using namespace Interpreter;

Value JSONValueToValue(json::JSONValue* val, bool unescape) {
    if (val == NULL) {
        return Value();
    }
    Value ret = Value();
    switch (val->Type) {
    case json::JSONValue::ARRAY: {
        ret = Value::MakeArray();
        auto iter = val->Array.begin();
        while (iter != val->Array.end()) {
            ret._array().push_back(JSONValueToValue(*iter, unescape));
            iter++;
        }
        return ret;
    }

    case json::JSONValue::OBJECT: {
        ret = Value::MakeMap();
        auto iter = val->Object.begin();
        while (iter != val->Object.end()) {
            ret._map()[iter->first] = JSONValueToValue(iter->second, unescape);
            iter++;
        }
        return ret;
    }
    case json::JSONValue::NUMBER:
        if (val->Value.find(".") != std::string::npos) {
            return Value(strtod(val->Value.c_str(), NULL));
        }
        return Value(strtoll(val->Value.c_str(), NULL, 0));
    case json::JSONValue::BOOL:
        if (val->Value == "true") {
            return Value(1l);
        }
        return Value(0l);
    case json::JSONValue::NIL:
        return Value();
    case json::JSONValue::STRING: {
        if (unescape) {
            return Value(DecodeJSONString(val->Value));
        }
        return Value(val->Value);
    }
    default:
        return Value();
    }
}

Value ParseJSON(std::string& str, bool unescape) {
    json::YY_BUFFER_STATE bp;
    json::yyscan_t scanner;
    json::yylex_init(&scanner);
    json::JSONParser* parser = new json::JSONParser(scanner);
    bp = json::yy_scan_bytes((char*)str.c_str(), (int)str.size(), scanner);
    json::yy_switch_to_buffer(bp, scanner);
    int error = json::yyparse(parser);
    json::yy_flush_buffer(bp, scanner);
    json::yy_delete_buffer(bp, scanner);
    json::yylex_destroy(scanner);
    if (error) {
        delete parser;
        return Value();
    }
    Value ret = JSONValueToValue(parser->root, unescape);
    delete parser;
    return ret;
}
