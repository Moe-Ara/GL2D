//
// Loads UI screens from JSON into runtime UI elements.
//

#include "UI/UILoader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <glm/common.hpp>

#include "Utils/SimpleJson.hpp"

namespace {
using Utils::JsonValue;

float numberOrDefault(const JsonValue& obj, const std::string& key, float fallback) {
    if (obj.isObject() && obj.hasKey(key) && obj.at(key).isNumber()) {
        return static_cast<float>(obj.at(key).asNumber());
    }
    return fallback;
}

glm::vec2 vec2OrDefault(const JsonValue& obj, const std::string& key, glm::vec2 fallback) {
    if (!obj.isObject() || !obj.hasKey(key)) return fallback;
    const auto& v = obj.at(key);
    if (!v.isArray() || v.asArray().size() != 2) return fallback;
    return glm::vec2(
        static_cast<float>(v.asArray()[0].asNumber()),
        static_cast<float>(v.asArray()[1].asNumber())
    );
}

glm::vec4 vec4OrDefault(const JsonValue& obj, const std::string& key, glm::vec4 fallback) {
    if (!obj.isObject() || !obj.hasKey(key)) return fallback;
    const auto& v = obj.at(key);
    if (!v.isArray() || v.asArray().size() != 4) return fallback;
    return glm::vec4(
        static_cast<float>(v.asArray()[0].asNumber()),
        static_cast<float>(v.asArray()[1].asNumber()),
        static_cast<float>(v.asArray()[2].asNumber()),
        static_cast<float>(v.asArray()[3].asNumber())
    );
}

UI::UITransform parseTransform(const JsonValue& node) {
    UI::UITransform tx{};
    tx.position = vec2OrDefault(node, "position", tx.position);
    tx.size = vec2OrDefault(node, "size", tx.size);
    tx.anchor = vec2OrDefault(node, "anchor", tx.anchor);
    tx.pivot = vec2OrDefault(node, "pivot", tx.pivot);
    return tx;
}

UI::UIEffect parseEffect(const JsonValue& node) {
    UI::UIEffect fx{};
    if (!node.isObject()) return fx;
    auto colorsIt = node.isObject() && node.hasKey("colors") ? node.at("colors") : node;
    if (colorsIt.isObject()) {
        fx.normal = vec4OrDefault(colorsIt, "normal", fx.normal);
        fx.hover = vec4OrDefault(colorsIt, "hover", fx.hover);
        fx.pressed = vec4OrDefault(colorsIt, "pressed", fx.pressed);
        fx.disabled = vec4OrDefault(colorsIt, "disabled", fx.disabled);
    }
    fx.lerpSpeed = numberOrDefault(node, "lerpSpeed", fx.lerpSpeed);
    fx.reset();
    return fx;
}

UI::ButtonMode parseButtonMode(const std::string& modeStr) {
    if (modeStr == "trigger") return UI::ButtonMode::Trigger;
    if (modeStr == "hold") return UI::ButtonMode::Hold;
    return UI::ButtonMode::Click;
}

UI::UIMenu::LayoutDirection parseDirection(const std::string& dir) {
    if (dir == "horizontal") return UI::UIMenu::LayoutDirection::Horizontal;
    return UI::UIMenu::LayoutDirection::Vertical;
}

GameObjects::Texture* resolveTexture(const JsonValue& node,
                                     const UI::UILoader::TextureResolver& resolver) {
    if (!resolver) return nullptr;
    if (!node.isObject() || !node.hasKey("texture")) return nullptr;
    const auto& texNode = node.at("texture");
    if (!texNode.isString()) return nullptr;
    return resolver(texNode.asString());
}

std::shared_ptr<UI::UIElement> buildElement(const JsonValue& node,
                                            const UI::UILoader::TextureResolver& resolver);

std::vector<std::shared_ptr<UI::UIElement>> parseChildren(const JsonValue& node,
                                                          const UI::UILoader::TextureResolver& resolver) {
    std::vector<std::shared_ptr<UI::UIElement>> children;
    if (node.isObject() && node.hasKey("children") && node.at("children").isArray()) {
        for (const auto& child : node.at("children").asArray()) {
            auto element = buildElement(child, resolver);
            if (element) {
                children.push_back(element);
            }
        }
    }
    return children;
}

std::shared_ptr<UI::UIElement> buildElement(const JsonValue& node,
                                            const UI::UILoader::TextureResolver& resolver) {
    if (!node.isObject()) {
        throw std::runtime_error("UI element node must be an object");
    }
    const auto& obj = node.asObject();
    auto idIt = obj.find("id");
    auto typeIt = obj.find("type");
    if (idIt == obj.end() || !idIt->second.isString()) {
        throw std::runtime_error("UI element missing string 'id'");
    }
    if (typeIt == obj.end() || !typeIt->second.isString()) {
        throw std::runtime_error("UI element missing string 'type'");
    }

    const std::string id = idIt->second.asString();
    const std::string type = typeIt->second.asString();
    UI::UITransform tx = parseTransform(node);
    UI::UIEffect fx = parseEffect(node);
    const float zIndex = numberOrDefault(node, "z", 1000.0f);
    GameObjects::Texture* texture = resolveTexture(node, resolver);

    std::shared_ptr<UI::UIElement> element;
    if (type == "button") {
        const std::string modeStr = node.hasKey("mode") && node.at("mode").isString()
                                        ? node.at("mode").asString()
                                        : "click";
        auto btn = std::make_shared<UI::Button<>>(id, tx, nullptr, parseButtonMode(modeStr), texture, fx);
        element = btn;
    } else if (type == "checkbox") {
        auto cb = std::make_shared<UI::UICheckbox>(id, tx, fx, texture);
        cb->checked = node.hasKey("checked") && node.at("checked").isBoolean() ? node.at("checked").asBoolean() : false;
        element = cb;
    } else if (type == "bar") {
        auto bar = std::make_shared<UI::UIBar>(id, tx, fx, texture);
        bar->background = vec4OrDefault(node, "background", bar->background);
        bar->fill = vec4OrDefault(node, "fill", bar->fill);
        bar->setValue(numberOrDefault(node, "value", 0.0f));
        element = bar;
    } else if (type == "icon") {
        auto icon = std::make_shared<UI::UIIcon>(id, tx, texture, fx);
        icon->tint = vec4OrDefault(node, "tint", icon->tint);
        element = icon;
    } else if (type == "item") {
        auto item = std::make_shared<UI::UIItem>(id, tx, fx, texture);
        item->label = node.hasKey("label") && node.at("label").isString() ? node.at("label").asString() : "";
        item->selectable = !node.hasKey("selectable") || (node.at("selectable").isBoolean() && node.at("selectable").asBoolean());
        item->selected = node.hasKey("selected") && node.at("selected").isBoolean() ? node.at("selected").asBoolean() : false;
        element = item;
    } else if (type == "menu") {
        std::string dirStr = node.hasKey("direction") && node.at("direction").isString()
                                 ? node.at("direction").asString()
                                 : "vertical";
        auto menu = std::make_shared<UI::UIMenu>(id, tx, fx, parseDirection(dirStr),
                                                 numberOrDefault(node, "spacing", 6.0f));
        element = menu;
    } else {
        throw std::runtime_error("Unsupported UI element type: " + type);
    }

    element->setZIndex(zIndex);
    if (node.hasKey("enabled") && node.at("enabled").isBoolean()) {
        element->setEnabled(node.at("enabled").asBoolean());
    }
    if (node.hasKey("visible") && node.at("visible").isBoolean()) {
        element->setVisible(node.at("visible").asBoolean());
    }

    // Attach children to any element; menus typically used as containers.
    auto children = parseChildren(node, resolver);
    for (auto& child : children) {
        element->addChild(child);
    }
    if (auto menu = std::dynamic_pointer_cast<UI::UIMenu>(element)) {
        menu->relayout();
    }

    return element;
}

} // namespace

namespace UI {

UIScreen UILoader::loadFromFile(const std::string& path, TextureResolver resolver) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open UI file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    const auto root = Utils::JsonValue::parse(buffer.str());

    if (!root.isObject()) {
        throw std::runtime_error("UI root must be a JSON object");
    }
    UIScreen screen{};
    screen.canvasSize = vec2OrDefault(root, "canvas", screen.canvasSize);
    screen.clearColor = vec4OrDefault(root, "clearColor", screen.clearColor);

    if (root.hasKey("elements") && root.at("elements").isArray()) {
        for (const auto& node : root.at("elements").asArray()) {
            screen.roots.push_back(buildElement(node, resolver));
        }
    } else {
        throw std::runtime_error("UI file missing 'elements' array");
    }

    return screen;
}

} // namespace UI
