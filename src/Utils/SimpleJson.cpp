#include "SimpleJson.hpp"

#include <cctype>
#include <charconv>
#include <cstring>
#include <sstream>

namespace Utils {

    namespace {
        class JsonParser {
        public:
            explicit JsonParser(const std::string &text) : m_text(text) {}

            JsonValue parse() {
                skipWhitespace();
                auto value = parseValue();
                skipWhitespace();
                if (!isEnd()) {
                    throw JsonParseException("Unexpected trailing characters in JSON");
                }
                return value;
            }

        private:
            const std::string &m_text;
            size_t m_index{0};

            [[nodiscard]] bool isEnd() const {
                return m_index >= m_text.size();
            }

            void skipWhitespace() {
                while (!isEnd() && std::isspace(static_cast<unsigned char>(m_text[m_index]))) {
                    ++m_index;
                }
            }

            char peek() const {
                if (isEnd()) {
                    throw JsonParseException("Unexpected end of JSON data");
                }
                return m_text[m_index];
            }

            char get() {
                if (isEnd()) {
                    throw JsonParseException("Unexpected end of JSON data");
                }
                return m_text[m_index++];
            }

            JsonValue parseValue() {
                skipWhitespace();
                if (isEnd()) {
                    throw JsonParseException("Unexpected end of JSON data");
                }
                char ch = peek();
                if (ch == '{') return parseObject();
                if (ch == '[') return parseArray();
                if (ch == '"') return JsonValue(parseString());
                if (ch == 't' || ch == 'f') return JsonValue(parseBoolean());
                if (ch == 'n') {
                    parseNull();
                    return JsonValue(nullptr);
                }
                if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch))) {
                    return JsonValue(parseNumber());
                }
                std::ostringstream oss;
                oss << "Invalid JSON value starting with '" << ch << "'";
                throw JsonParseException(oss.str());
            }

            JsonValue parseObject() {
                JsonValue::Object object;
                expect('{');
                skipWhitespace();
                if (peek() == '}') {
                    get();
                    return JsonValue(object);
                }
                while (true) {
                    skipWhitespace();
                    if (peek() != '"') {
                        throw JsonParseException("Expected string key in JSON object");
                    }
                    std::string key = parseString();
                    skipWhitespace();
                    expect(':');
                    skipWhitespace();
                    object.emplace(std::move(key), parseValue());
                    skipWhitespace();
                    char ch = get();
                    if (ch == '}') break;
                    if (ch != ',') {
                        throw JsonParseException("Expected ',' or '}' in JSON object");
                    }
                    skipWhitespace();
                }
                return JsonValue(object);
            }

            JsonValue parseArray() {
                JsonValue::Array array;
                expect('[');
                skipWhitespace();
                if (peek() == ']') {
                    get();
                    return JsonValue(array);
                }
                while (true) {
                    skipWhitespace();
                    array.push_back(parseValue());
                    skipWhitespace();
                    char ch = get();
                    if (ch == ']') break;
                    if (ch != ',') {
                        throw JsonParseException("Expected ',' or ']' in JSON array");
                    }
                    skipWhitespace();
                }
                return JsonValue(array);
            }

            std::string parseString() {
                expect('"');
                std::string result;
                while (true) {
                    if (isEnd()) {
                        throw JsonParseException("Unterminated string literal in JSON");
                    }
                    char ch = get();
                    if (ch == '"') break;
                    if (ch == '\\') {
                        if (isEnd()) {
                            throw JsonParseException("Invalid escape sequence in JSON string");
                        }
                        char esc = get();
                        switch (esc) {
                            case '"': result.push_back('"'); break;
                            case '\\': result.push_back('\\'); break;
                            case '/': result.push_back('/'); break;
                            case 'b': result.push_back('\b'); break;
                            case 'f': result.push_back('\f'); break;
                            case 'n': result.push_back('\n'); break;
                            case 'r': result.push_back('\r'); break;
                            case 't': result.push_back('\t'); break;
                            case 'u': {
                                result.push_back(parseUnicodeSequence());
                                break;
                            }
                            default: {
                                std::ostringstream oss;
                                oss << "Unsupported escape sequence: \\" << esc;
                                throw JsonParseException(oss.str());
                            }
                        }
                    } else {
                        result.push_back(ch);
                    }
                }
                return result;
            }

            char parseUnicodeSequence() {
                if (m_index + 4 > m_text.size()) {
                    throw JsonParseException("Invalid unicode escape sequence in JSON string");
                }
                int value = 0;
                for (int i = 0; i < 4; ++i) {
                    char ch = m_text[m_index++];
                    value <<= 4;
                    if (ch >= '0' && ch <= '9') value += ch - '0';
                    else if (ch >= 'a' && ch <= 'f') value += ch - 'a' + 10;
                    else if (ch >= 'A' && ch <= 'F') value += ch - 'A' + 10;
                    else throw JsonParseException("Invalid character in unicode escape sequence");
                }
                if (value <= 0x7F) {
                    return static_cast<char>(value);
                }
                // Basic handling: clamp to ASCII range for simplicity.
                return '?';
            }

            double parseNumber() {
                size_t start = m_index;
                if (peek() == '-') {
                    ++m_index;
                }
                auto parseDigits = [&]() {
                    if (isEnd() || !std::isdigit(static_cast<unsigned char>(peek()))) {
                        throw JsonParseException("Invalid numeric literal in JSON");
                    }
                    while (!isEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
                        ++m_index;
                    }
                };
                if (peek() == '0') {
                    ++m_index;
                } else {
                    parseDigits();
                }
                if (!isEnd() && peek() == '.') {
                    ++m_index;
                    parseDigits();
                }
                if (!isEnd() && (peek() == 'e' || peek() == 'E')) {
                    ++m_index;
                    if (!isEnd() && (peek() == '+' || peek() == '-')) {
                        ++m_index;
                    }
                    parseDigits();
                }
                double value{};
                std::string_view sv(m_text.c_str() + start, m_index - start);
                auto result = std::from_chars(sv.data(), sv.data() + sv.size(), value);
                if (result.ec != std::errc()) {
                    std::ostringstream oss;
                    oss << "Failed parsing numeric literal near '" << sv << "'";
                    throw JsonParseException(oss.str());
                }
                return value;
            }

            bool parseBoolean() {
                if (matchLiteral("true")) return true;
                if (matchLiteral("false")) return false;
                throw JsonParseException("Invalid boolean literal in JSON");
            }

            void parseNull() {
                if (!matchLiteral("null")) {
                    throw JsonParseException("Invalid null literal in JSON");
                }
            }

            bool matchLiteral(const char *literal) {
                size_t len = std::strlen(literal);
                if (m_index + len > m_text.size()) {
                    return false;
                }
                if (m_text.compare(m_index, len, literal) == 0) {
                    m_index += len;
                    return true;
                }
                return false;
            }

            void expect(char expected) {
                char ch = get();
                if (ch != expected) {
                    std::ostringstream oss;
                    oss << "Expected '" << expected << "' but found '" << ch << "'";
                    throw JsonParseException(oss.str());
                }
            }
        };
    }

    JsonParseException::JsonParseException(const std::string &message)
            : Engine::GL2DException("JsonParseException: " + message) {
    }

    JsonValue::JsonValue() : m_type(Type::Null), m_storage(nullptr) {}

    JsonValue::JsonValue(std::nullptr_t) : m_type(Type::Null), m_storage(nullptr) {}

    JsonValue::JsonValue(bool boolean) : m_type(Type::Boolean), m_storage(boolean) {}

    JsonValue::JsonValue(double number) : m_type(Type::Number), m_storage(number) {}

    JsonValue::JsonValue(std::string string) : m_type(Type::String), m_storage(std::move(string)) {}

    JsonValue::JsonValue(Array array) : m_type(Type::Array), m_storage(std::move(array)) {}

    JsonValue::JsonValue(Object object) : m_type(Type::Object), m_storage(std::move(object)) {}

    JsonValue::Type JsonValue::getType() const {
        return m_type;
    }

    bool JsonValue::isNull() const { return m_type == Type::Null; }

    bool JsonValue::isBoolean() const { return m_type == Type::Boolean; }

    bool JsonValue::isNumber() const { return m_type == Type::Number; }

    bool JsonValue::isString() const { return m_type == Type::String; }

    bool JsonValue::isArray() const { return m_type == Type::Array; }

    bool JsonValue::isObject() const { return m_type == Type::Object; }

    bool JsonValue::asBoolean() const {
        if (!isBoolean()) throw JsonParseException("JSON value is not a boolean");
        return std::get<bool>(m_storage);
    }

    double JsonValue::asNumber() const {
        if (!isNumber()) throw JsonParseException("JSON value is not a number");
        return std::get<double>(m_storage);
    }

    const std::string &JsonValue::asString() const {
        if (!isString()) throw JsonParseException("JSON value is not a string");
        return std::get<std::string>(m_storage);
    }

    const JsonValue::Array &JsonValue::asArray() const {
        if (!isArray()) throw JsonParseException("JSON value is not an array");
        return std::get<Array>(m_storage);
    }

    const JsonValue::Object &JsonValue::asObject() const {
        if (!isObject()) throw JsonParseException("JSON value is not an object");
        return std::get<Object>(m_storage);
    }

    bool JsonValue::hasKey(const std::string &key) const {
        if (!isObject()) return false;
        const auto &obj = std::get<Object>(m_storage);
        return obj.find(key) != obj.end();
    }

    const JsonValue &JsonValue::at(const std::string &key) const {
        if (!isObject()) {
            throw JsonParseException("JSON value is not an object");
        }
        const auto &obj = std::get<Object>(m_storage);
        auto it = obj.find(key);
        if (it == obj.end()) {
            throw JsonParseException("Key '" + key + "' not found in JSON object");
        }
        return it->second;
    }

    JsonValue JsonValue::parse(const std::string &text) {
        JsonParser parser(text);
        return parser.parse();
    }

} // Utils
