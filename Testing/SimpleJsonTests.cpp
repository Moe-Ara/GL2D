#include <boost/test/unit_test.hpp>

#include "Utils/SimpleJson.hpp"

BOOST_AUTO_TEST_SUITE(SimpleJsonTests)

BOOST_AUTO_TEST_CASE(parses_basic_object_structure) {
    const std::string json = R"({
        "name": "hero",
        "hp": 42,
        "alive": true,
        "tags": ["player", "friendly"],
        "nested": { "value": 1.5 }
    })";

    const auto value = Utils::JsonValue::parse(json);
    BOOST_TEST(value.isObject());
    BOOST_TEST(value.at("name").asString() == "hero");
    BOOST_TEST(value.at("hp").asNumber() == 42.0);
    BOOST_TEST(value.at("alive").asBoolean());

    const auto &tags = value.at("tags").asArray();
    BOOST_TEST(tags.size() == 2);
    BOOST_TEST(tags[0].asString() == "player");
    BOOST_TEST(tags[1].asString() == "friendly");

    BOOST_TEST(value.at("nested").asObject().at("value").asNumber() == 1.5);
}

BOOST_AUTO_TEST_CASE(throws_on_invalid_json) {
    const std::string badJson = R"({ "unterminated": [1, 2, })";
    BOOST_CHECK_THROW(Utils::JsonValue::parse(badJson), Utils::JsonParseException);
}

BOOST_AUTO_TEST_SUITE_END()
