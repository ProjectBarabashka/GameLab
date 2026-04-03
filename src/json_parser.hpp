// json_parser.hpp - Простой JSON парсер (общий для всего проекта)

#pragma once
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <cctype>

namespace SimpleJSON {
    class Value;
    using Object = std::map<std::string, std::shared_ptr<Value>>;
    using Array  = std::vector<std::shared_ptr<Value>>;

    class Value {
    public:
        enum Type { NULL_T, BOOL, NUMBER, STRING, ARRAY_T, OBJECT };
        Type        type;
        bool        boolVal;
        double      numVal;
        std::string strVal;
        Array       arrayVal;
        Object      objectVal;

        Value() : type(NULL_T), boolVal(false), numVal(0) {}

        bool isObject() const { return type == OBJECT;  }
        bool isArray()  const { return type == ARRAY_T; }
        bool isString() const { return type == STRING;  }
        bool isNumber() const { return type == NUMBER;  }
        bool isBool()   const { return type == BOOL;    }

        std::string asString() const { return strVal; }
        double      asDouble() const { return numVal;  }
        int         asInt()    const { return (int)numVal; }
        bool        asBool()   const { return boolVal; }

        std::shared_ptr<Value> get(const std::string& key) const {
            auto it = objectVal.find(key);
            return it != objectVal.end() ? it->second : nullptr;
        }
        std::shared_ptr<Value> get(size_t idx) const {
            return idx < arrayVal.size() ? arrayVal[idx] : nullptr;
        }
    };

    class Parser {
    public:
        static std::shared_ptr<Value> parse(const std::string& json) {
            size_t pos = 0;
            skipWS(json, pos);
            return parseValue(json, pos);
        }
    private:
        static void skipWS(const std::string& s, size_t& pos) {
            while (pos < s.size() && std::isspace((unsigned char)s[pos])) pos++;
        }
        static std::shared_ptr<Value> parseValue(const std::string& s, size_t& pos) {
            skipWS(s, pos);
            if (pos >= s.size()) return nullptr;
            char c = s[pos];
            if (c == '{') return parseObject(s, pos);
            if (c == '[') return parseArray(s, pos);
            if (c == '"') return parseString(s, pos);
            if (c == 't' || c == 'f') return parseBool(s, pos);
            if (c == 'n') return parseNull(s, pos);
            return parseNumber(s, pos);
        }
        static std::shared_ptr<Value> parseObject(const std::string& s, size_t& pos) {
            auto obj = std::make_shared<Value>(); obj->type = Value::OBJECT;
            pos++;
            skipWS(s, pos);
            while (pos < s.size() && s[pos] != '}') {
                skipWS(s, pos);
                auto key = parseString(s, pos);
                if (!key) return obj;
                skipWS(s, pos);
                if (pos >= s.size() || s[pos] != ':') return obj;
                pos++;
                auto val = parseValue(s, pos);
                obj->objectVal[key->asString()] = val;
                skipWS(s, pos);
                if (pos < s.size() && s[pos] == ',') pos++;
            }
            if (pos < s.size()) pos++;
            return obj;
        }
        static std::shared_ptr<Value> parseArray(const std::string& s, size_t& pos) {
            auto arr = std::make_shared<Value>(); arr->type = Value::ARRAY_T;
            pos++;
            skipWS(s, pos);
            while (pos < s.size() && s[pos] != ']') {
                arr->arrayVal.push_back(parseValue(s, pos));
                skipWS(s, pos);
                if (pos < s.size() && s[pos] == ',') pos++;
            }
            if (pos < s.size()) pos++;
            return arr;
        }
        static std::shared_ptr<Value> parseString(const std::string& s, size_t& pos) {
            auto str = std::make_shared<Value>(); str->type = Value::STRING;
            pos++;
            while (pos < s.size() && s[pos] != '"') {
                if (s[pos] == '\\' && pos + 1 < s.size()) {
                    pos++;
                    if      (s[pos] == 'n') str->strVal += '\n';
                    else if (s[pos] == 't') str->strVal += '\t';
                    else if (s[pos] == 'r') str->strVal += '\r';
                    else                    str->strVal += s[pos];
                } else { str->strVal += s[pos]; }
                pos++;
            }
            if (pos < s.size()) pos++;
            return str;
        }
        static std::shared_ptr<Value> parseNumber(const std::string& s, size_t& pos) {
            auto num = std::make_shared<Value>(); num->type = Value::NUMBER;
            std::string ns;
            while (pos < s.size() && (std::isdigit((unsigned char)s[pos]) ||
                   s[pos]=='.'||s[pos]=='-'||s[pos]=='e'||s[pos]=='E'||s[pos]=='+'))
                ns += s[pos++];
            if (!ns.empty()) num->numVal = std::stod(ns);
            return num;
        }
        static std::shared_ptr<Value> parseBool(const std::string& s, size_t& pos) {
            auto b = std::make_shared<Value>(); b->type = Value::BOOL;
            if (s.substr(pos,4)=="true") { b->boolVal=true;  pos+=4; }
            else                         { b->boolVal=false; pos+=5; }
            return b;
        }
        static std::shared_ptr<Value> parseNull(const std::string& s, size_t& pos) {
            auto n = std::make_shared<Value>(); n->type = Value::NULL_T; pos+=4; return n;
        }
    };
}
