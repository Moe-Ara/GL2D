#include <boost/test/unit_test.hpp>

#include "Engine/Scene.hpp"
#include "FeelingsSystem/FeelingsController.hpp"
#include "FeelingsSystem/FeelingsLoader.hpp"
#include "FeelingsSystem/FeelingsManager.hpp"

#include <limits>
#include <unordered_map>

namespace {
FeelingsSystem::FeelingSnapshot richFeeling() {
    FeelingsSystem::FeelingSnapshot feeling{};
    feeling.id = "rich";
    feeling.blendInMs = 100.0f;
    feeling.colorGrade = glm::vec4(0.5f, 0.8f, 1.2f, 1.0f);
    feeling.vignette = 0.8f;
    feeling.bloomStrength = 2.0f;
    feeling.zoomMul = 2.0f;
    feeling.offset = glm::vec2(10.0f, -4.0f);
    feeling.shakeMagnitude = 6.0f;
    feeling.shakeRoughness = 20.0f;
    feeling.followSpeedMul = 0.5f;
    feeling.timeScale = 0.4f;
    feeling.entitySpeedMul = 1.4f;
    feeling.animationSpeedMul = 0.8f;
    feeling.accelerationSpeedMul = 1.6f;
    feeling.damageMul = 1.5f;
    feeling.armorMul = 0.7f;
    feeling.lightIntensityMul = 2.0f;
    feeling.lightRadiusMul = 1.5f;
    feeling.lightColorMul = glm::vec3(0.8f, 0.9f, 1.2f);
    feeling.ambientLightMul = glm::vec3(0.6f);
    feeling.ambientLightAdd = glm::vec3(0.2f, -0.1f, 0.0f);
    feeling.musicVolume = 0.2f;
    feeling.sfxVolumeMul = 0.6f;
    feeling.reverbSend = 0.5f;
    feeling.uiTint = glm::vec4(0.6f, 0.8f, 1.0f, 1.0f);
    return feeling;
}
}

BOOST_AUTO_TEST_SUITE(FeelingsSystemTests)

BOOST_AUTO_TEST_CASE(all_numeric_overrides_blend_from_field_neutral_values) {
    FeelingsSystem::FeelingsManager manager;
    manager.setFeeling(richFeeling());
    manager.update(50.0f);
    const auto& current = manager.getSnapshot();

    BOOST_TEST(*current.vignette == 0.4f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(*current.bloomStrength == 1.0f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(*current.zoomMul == 1.5f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(current.offset->x == 5.0f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(*current.accelerationSpeedMul == 1.3f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(*current.damageMul == 1.25f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(*current.lightIntensityMul == 1.5f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(current.ambientLightAdd->x == 0.1f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(*current.musicVolume == 0.6f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(current.uiTint->x == 0.8f,
               boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_CASE(overrides_fade_to_neutral_before_becoming_unset) {
    FeelingsSystem::FeelingsManager manager;
    manager.setFeeling(richFeeling(), 0.0f);
    FeelingsSystem::FeelingSnapshot neutral{};
    neutral.id = "neutral";
    manager.setFeeling(neutral, 100.0f);
    manager.update(50.0f);

    BOOST_TEST(*manager.getSnapshot().zoomMul == 1.5f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(*manager.getSnapshot().lightIntensityMul == 1.5f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(*manager.getSnapshot().vignette == 0.4f,
               boost::test_tools::tolerance(1e-5f));

    manager.update(50.0f);
    BOOST_TEST(!manager.isBlending());
    BOOST_TEST(!manager.getSnapshot().zoomMul.has_value());
    BOOST_TEST(!manager.getSnapshot().lightIntensityMul.has_value());
    BOOST_TEST(!manager.getSnapshot().vignette.has_value());
}

BOOST_AUTO_TEST_CASE(scene_advances_blends_once_in_unscaled_frame_time) {
    Scene scene;
    scene.configureFixedStep({0.01, 0.25, 32});
    FeelingsSystem::FeelingSnapshot stopped{};
    stopped.id = "stopped";
    stopped.blendInMs = 100.0f;
    stopped.timeScale = 0.0f;
    scene.feelings().setFeeling(stopped);

    const auto first = scene.advance(0.05f);
    BOOST_TEST(*scene.feelings().getSnapshot().timeScale == 0.5f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(first.steps == 2u);
    BOOST_TEST(first.interpolationAlpha == 0.5,
               boost::test_tools::tolerance(1e-4));

    const auto second = scene.advance(0.05f);
    BOOST_TEST(*scene.feelings().getSnapshot().timeScale == 0.0f);
    BOOST_TEST(second.steps == 0u);
}

BOOST_AUTO_TEST_CASE(timed_controller_uses_authored_blend_out) {
    FeelingsSystem::FeelingsManager manager;
    FeelingsSystem::FeelingsController controller(manager);
    FeelingsSystem::FeelingSnapshot base{};
    base.id = "default";
    base.timeScale = 1.0f;
    FeelingsSystem::FeelingSnapshot slow{};
    slow.id = "slow";
    slow.timeScale = 0.5f;
    slow.blendOutMs = 200.0f;
    controller.setDefinitions({{"default", base}, {"slow", slow}});

    BOOST_REQUIRE(controller.setFeeling("slow", 50.0f, 0.0f));
    controller.update(50.0f);
    BOOST_TEST(manager.isBlending());
    manager.update(100.0f);
    BOOST_TEST(*manager.getSnapshot().timeScale == 0.75f,
               boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(!controller.setFeeling("missing"));
}

BOOST_AUTO_TEST_CASE(loader_reads_gameplay_and_lighting_fields) {
    const auto definitions = FeelingsSystem::FeelingsLoader::loadMap(
        "assets/config/feelings.json");
    const auto& base = definitions.at("default");
    BOOST_REQUIRE(base.accelerationSpeedMul.has_value());
    BOOST_REQUIRE(base.damageMul.has_value());
    BOOST_REQUIRE(base.lightIntensityMul.has_value());
    BOOST_REQUIRE(base.lightColorMul.has_value());
    BOOST_TEST(*base.accelerationSpeedMul == 1.0f);
    BOOST_TEST(*base.damageMul == 1.0f);
    BOOST_TEST(*base.lightIntensityMul == 1.0f);
    BOOST_TEST(base.lightColorMul->z == 1.0f);
}

BOOST_AUTO_TEST_CASE(shared_validation_rejects_invalid_values) {
    FeelingsSystem::FeelingSnapshot invalid{};
    invalid.id = "invalid";
    invalid.delayFeedback = 1.1f;
    BOOST_REQUIRE(FeelingsSystem::validationError(invalid).has_value());

    FeelingsSystem::FeelingsManager manager;
    BOOST_CHECK_THROW(manager.setFeeling(invalid), std::invalid_argument);
    invalid.delayFeedback.reset();
    invalid.ambientLightAdd = glm::vec3(
        std::numeric_limits<float>::quiet_NaN());
    BOOST_REQUIRE(FeelingsSystem::validationError(invalid).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
