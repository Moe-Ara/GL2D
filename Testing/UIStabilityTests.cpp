#include <boost/test/unit_test.hpp>

#include <limits>
#include <memory>
#include <stdexcept>

#include "UI/DialogueBox.hpp"
#include "UI/UIElements.hpp"

BOOST_AUTO_TEST_SUITE(UIStabilityTests)

BOOST_AUTO_TEST_CASE(ui_tree_rejects_null_duplicate_and_cyclic_children) {
    auto root = std::make_shared<UI::UIElement>("root");
    auto child = std::make_shared<UI::UIElement>("child");
    auto grandchild = std::make_shared<UI::UIElement>("grandchild");
    auto otherRoot = std::make_shared<UI::UIElement>("other_root");

    BOOST_CHECK_THROW(root->addChild(nullptr), std::invalid_argument);
    root->addChild(child);
    child->addChild(grandchild);
    BOOST_CHECK_THROW(root->addChild(child), std::logic_error);
    BOOST_CHECK_THROW(otherRoot->addChild(child), std::logic_error);
    BOOST_CHECK_THROW(grandchild->addChild(root), std::logic_error);
    BOOST_CHECK_THROW(root->addChild(root), std::logic_error);

    auto retainedChild = std::make_shared<UI::UIElement>("retained_child");
    {
        auto temporaryParent = std::make_shared<UI::UIElement>("temporary_parent");
        temporaryParent->addChild(retainedChild);
    }
    otherRoot->addChild(retainedChild);
}

BOOST_AUTO_TEST_CASE(children_use_parent_bounds_for_layout_and_interaction) {
    UI::UITransform parentTransform{};
    parentTransform.position = {100.0f, 80.0f};
    parentTransform.size = {200.0f, 120.0f};
    auto parent = std::make_shared<UI::UIElement>("parent", parentTransform);

    UI::UITransform childTransform{};
    childTransform.position = {10.0f, 20.0f};
    childTransform.size = {50.0f, 30.0f};
    bool clicked = false;
    auto child = std::make_shared<UI::Button<>>(
        "child", childTransform, [&clicked]() { clicked = true; });
    parent->addChild(child);

    UI::UIScreen screen{};
    screen.canvasSize = {640.0f, 360.0f};
    screen.roots.push_back(parent);

    const auto commands = screen.collectRenderCommands();
    BOOST_REQUIRE_EQUAL(commands.size(), 2U);
    BOOST_CHECK_EQUAL(commands[0].rect.x, 100.0f);
    BOOST_CHECK_EQUAL(commands[0].rect.y, 80.0f);
    BOOST_CHECK_EQUAL(commands[0].rect.z, 300.0f);
    BOOST_CHECK_EQUAL(commands[0].rect.w, 200.0f);
    BOOST_CHECK_EQUAL(commands[1].rect.x, 110.0f);
    BOOST_CHECK_EQUAL(commands[1].rect.y, 100.0f);
    BOOST_CHECK_EQUAL(commands[1].rect.z, 160.0f);
    BOOST_CHECK_EQUAL(commands[1].rect.w, 130.0f);

    UI::UIPointerState pointer{};
    pointer.position = {120.0f, 110.0f};
    pointer.released = true;
    screen.update(1.0f / 60.0f, pointer);
    BOOST_CHECK(clicked);
}

BOOST_AUTO_TEST_CASE(child_anchors_are_relative_to_parent_size) {
    UI::UITransform parentTransform{};
    parentTransform.position = {40.0f, 50.0f};
    parentTransform.size = {200.0f, 100.0f};
    auto parent = std::make_shared<UI::UIElement>("parent", parentTransform);

    UI::UITransform childTransform{};
    childTransform.size = {20.0f, 10.0f};
    childTransform.anchor = {1.0f, 1.0f};
    childTransform.pivot = {1.0f, 1.0f};
    auto child = std::make_shared<UI::UIElement>("child", childTransform);
    parent->addChild(child);

    UI::UIScreen screen{};
    screen.roots.push_back(parent);
    const auto commands = screen.collectRenderCommands();
    BOOST_REQUIRE_EQUAL(commands.size(), 2U);
    BOOST_CHECK_EQUAL(commands[1].rect.x, 220.0f);
    BOOST_CHECK_EQUAL(commands[1].rect.y, 140.0f);
    BOOST_CHECK_EQUAL(commands[1].rect.z, 240.0f);
    BOOST_CHECK_EQUAL(commands[1].rect.w, 150.0f);
}

BOOST_AUTO_TEST_CASE(ui_layout_and_animation_inputs_are_validated) {
    UI::UITransform invalidTransform{};
    invalidTransform.size = {-1.0f, 10.0f};
    BOOST_CHECK_THROW(UI::UIElement("invalid", invalidTransform), std::invalid_argument);
    BOOST_CHECK_THROW(UI::UIElement(""), std::invalid_argument);

    UI::UIElement element("element");
    BOOST_CHECK_THROW(element.setZIndex(std::numeric_limits<float>::quiet_NaN()),
                      std::invalid_argument);
    BOOST_CHECK_THROW(element.updateTree(-0.1f, {}, {1280.0f, 720.0f}),
                      std::invalid_argument);

    UI::UIBar bar("bar");
    BOOST_CHECK_THROW(bar.setValue(std::numeric_limits<float>::infinity()),
                      std::invalid_argument);
    UI::UIMenu menu("menu");
    BOOST_CHECK_THROW(menu.setSpacing(-1.0f), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(dialogue_authoring_values_are_checked) {
    UI::DialogueBox dialogue("dialogue");
    BOOST_CHECK_THROW(dialogue.setPadding(-1.0f), std::invalid_argument);
    BOOST_CHECK_THROW(dialogue.setFontScale(0.0f), std::invalid_argument);
    BOOST_CHECK_THROW(
        dialogue.setTextOffset({std::numeric_limits<float>::infinity(), 0.0f}),
        std::invalid_argument);
    BOOST_CHECK_THROW(dialogue.setSpeakerSpacing(-0.5f), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(screen_rejects_invalid_canvas_dimensions) {
    UI::UIScreen screen{};
    screen.canvasSize = {0.0f, 720.0f};
    BOOST_CHECK_THROW(screen.update(0.016f, {}), std::invalid_argument);
    BOOST_CHECK_THROW(screen.collectRenderCommands(), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
