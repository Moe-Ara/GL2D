#ifndef GL2D_SIMPLEJSON_HPP
#define GL2D_SIMPLEJSON_HPP

#include <map>
#include <string>
#include <variant>
#include <vector>
#include "Exceptions/Gl2DException.hpp"

namespace Utils {

    class JsonParseException : public Engine::GL2DException {
    public:
        explicit JsonParseException(const std::string &message);
    };

    class JsonValue {
    public:
        enum class Type {
            Null,
            Boolean,
            Number,
            String,
            Array,
            Object
        };

        using Array = std::vector<JsonValue>;
        using Object = std::map<std::string, JsonValue>;

        JsonValue();
        explicit JsonValue(std::nullptr_t);
        explicit JsonValue(bool boolean);
        explicit JsonValue(double number);
        explicit JsonValue(std::string string);
        explicit JsonValue(Array array);
        explicit JsonValue(Object object);

        [[nodiscard]] Type getType() const;

        [[nodiscard]] bool isNull() const;
        [[nodiscard]] bool isBoolean() const;
        [[nodiscard]] bool isNumber() const;
        [[nodiscard]] bool isString() const;
        [[nodiscard]] bool isArray() const;
        [[nodiscard]] bool isObject() const;

        [[nodiscard]] bool asBoolean() const;
        [[nodiscard]] double asNumber() const;
        [[nodiscard]] const std::string &asString() const;
        [[nodiscard]] const Array &asArray() const;
        [[nodiscard]] const Object &asObject() const;

        [[nodiscard]] bool hasKey(const std::string &key) const;
        [[nodiscard]] const JsonValue &at(const std::string &key) const;

        static JsonValue parse(const std::string &text);

    private:
        Type m_type;
        std::variant<std::nullptr_t, bool, double, std::string, Array, Object> m_storage;
    };

} // Utils

#endif //GL2D_SIMPLEJSON_HPP
