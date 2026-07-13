#include <boost/test/unit_test.hpp>

#include "GameObjects/Components/AnimationStateMachineComponent.hpp"
#include "GameObjects/Components/AnimatorComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Sprite.hpp"
#include "Graphics/Animation/AnimationStateMachine.hpp"
#include "Graphics/Animation/Animator.hpp"
#include "Managers/AnimationStateMachineManager.hpp"
#include "Managers/AnimatorManager.hpp"
#include "Managers/ComponentFactory.hpp"
#include "Managers/SpriteManager.hpp"

#include <memory>
#include <stdexcept>

BOOST_AUTO_TEST_SUITE(AssetRegistryTests)

BOOST_AUTO_TEST_CASE(sprite_registry_never_returns_an_expired_asset) {
    SpriteManager::clear();
    auto sprite = std::make_shared<GameObjects::Sprite>(
        glm::vec2{0.0f}, glm::vec2{8.0f, 12.0f}, glm::vec3{1.0f});
    SpriteManager::registerSprite("test.sprite", sprite);
    BOOST_TEST(SpriteManager::contains("test.sprite"));
    BOOST_TEST(SpriteManager::get("test.sprite") == sprite);

    sprite.reset();
    BOOST_TEST(!SpriteManager::get("test.sprite"));
    BOOST_TEST(!SpriteManager::contains("test.sprite"));
    SpriteManager::clear();
}

BOOST_AUTO_TEST_CASE(registries_reject_conflicting_live_ids) {
    SpriteManager::clear();
    const auto first = std::make_shared<GameObjects::Sprite>(
        glm::vec2{0.0f}, glm::vec2{1.0f}, glm::vec3{1.0f});
    const auto second = std::make_shared<GameObjects::Sprite>(
        glm::vec2{0.0f}, glm::vec2{1.0f}, glm::vec3{0.0f});
    SpriteManager::registerSprite("duplicate", first);
    BOOST_CHECK_THROW(
        SpriteManager::registerSprite("duplicate", second),
        std::invalid_argument);
    BOOST_CHECK_THROW(
        SpriteManager::registerSprite("", first), std::invalid_argument);
    SpriteManager::clear();
}

BOOST_AUTO_TEST_CASE(component_factory_retains_animation_assets_safely) {
    AnimatorManager::clear();
    AnimationStateMachineManager::clear();
    auto sprite = std::make_shared<GameObjects::Sprite>(
        glm::vec2{0.0f}, glm::vec2{1.0f}, glm::vec3{1.0f});
    auto animator = std::make_shared<Graphics::Animator>(sprite);
    auto stateMachine = std::make_shared<Graphics::AnimationStateMachine>();
    std::weak_ptr<Graphics::Animator> animatorLifetime = animator;
    std::weak_ptr<Graphics::AnimationStateMachine> stateMachineLifetime =
        stateMachine;
    AnimatorManager::registerAnimator("test.animator", animator);
    AnimationStateMachineManager::registerStateMachine(
        "test.machine", stateMachine);

    ComponentSpec animatorSpec{};
    animatorSpec.type = "Animator";
    animatorSpec.strings["animatorId"] = "test.animator";
    ComponentSpec stateMachineSpec{};
    stateMachineSpec.type = "AnimStateMachine";
    stateMachineSpec.strings["stateMachineId"] = "test.machine";
    auto animatorComponent = ComponentFactory::create(animatorSpec);
    auto stateMachineComponent = ComponentFactory::create(stateMachineSpec);
    BOOST_REQUIRE(dynamic_cast<AnimatorComponent*>(animatorComponent.get()));
    BOOST_REQUIRE(dynamic_cast<AnimationStateMachineComponent*>(
        stateMachineComponent.get()));

    animator.reset();
    stateMachine.reset();
    AnimatorManager::clear();
    AnimationStateMachineManager::clear();
    BOOST_TEST(!animatorLifetime.expired());
    BOOST_TEST(!stateMachineLifetime.expired());

    animatorComponent.reset();
    stateMachineComponent.reset();
    BOOST_TEST(animatorLifetime.expired());
    BOOST_TEST(stateMachineLifetime.expired());
}

BOOST_AUTO_TEST_CASE(component_factory_reports_missing_asset_context) {
    SpriteManager::clear();
    AnimatorManager::clear();
    AnimationStateMachineManager::clear();

    ComponentSpec spriteSpec{};
    spriteSpec.type = "Sprite";
    spriteSpec.strings["spriteId"] = "missing.sprite";
    BOOST_CHECK_THROW(ComponentFactory::create(spriteSpec),
                      std::invalid_argument);

    ComponentSpec animatorSpec{};
    animatorSpec.type = "Animator";
    animatorSpec.strings["animatorId"] = "missing.animator";
    BOOST_CHECK_THROW(ComponentFactory::create(animatorSpec),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
