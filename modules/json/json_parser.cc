#include "json_parser.hpp"

#include "../../engine/value.hpp"
namespace json {
#include "json.lex.cpp"
#include "json.tab.cpp"
void parser_json_error(json::JSONParser* parser, const char* s) {
    printf("parser_json error : %s on line:%d\n", s, yylineno);
}
} // namespace json

using namespace Interpreter;

Value JSONValueToValue(json::JSONValue* val) {
    if (val == NULL) {
        return Value();
    }
    Value ret = Value();
    switch (val->Type) {
    case json::JSONValue::ARRAY: {
        ret = Value::make_array();
        auto iter = val->Array.begin();
        while (iter != val->Array.end()) {
            ret._array().push_back(JSONValueToValue(*iter));
            iter++;
        }
        return ret;
    }

    case json::JSONValue::OBJECT: {
        ret = Value::make_map();
        auto iter = val->Object.begin();
        while (iter != val->Object.end()) {
            ret._map()[iter->first] = JSONValueToValue(iter->second);
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
    case json::JSONValue::STRING:
        return Value(DecodeJSONString(val->Value));
    default:
        return Value();
    }
}

int ParseJSONTest(std::string& str) {
    json::YY_BUFFER_STATE bp;
    json::JSONParser* parser = new json::JSONParser();
    bp = json::yy_scan_bytes(str.c_str(), str.size());
    yy_switch_to_buffer(bp);
    int error = json::yyparse(parser);
    json::yy_flush_buffer(bp);
    json::yy_delete_buffer(bp);
    json::yylex_destroy();
    delete parser;
    return error;
}

Value ParseJSON(std::string& str) {
    json::YY_BUFFER_STATE bp;
    json::JSONParser* parser = new json::JSONParser();
    bp = json::yy_scan_bytes(str.c_str(), str.size());
    yy_switch_to_buffer(bp);
    int error = json::yyparse(parser);
    json::yy_flush_buffer(bp);
    json::yy_delete_buffer(bp);
    json::yylex_destroy();
    if (error) {
        return Value();
    }
    Value ret = JSONValueToValue(parser->root);
    delete parser;
    return ret;
}
