#include <boost/test/unit_test.hpp>

#include <glm/vec4.hpp>

#include <cmath>
#include <limits>
#include <stdexcept>

#include "Engine/Scene.hpp"
#include "Engine/VehicleMountComponent.hpp"
#include "Debug/DebugOverlay.hpp"
#include "GameObjects/Components/HingeComponent.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "Utils/Transform.hpp"

namespace {
class DestroyOnUpdate final : public IUpdatableComponent {
public:
    explicit DestroyOnUpdate(Scene& scene) : m_scene(scene) {}

    void update(Entity& owner, double) override {
        m_scene.destroyEntity(owner);
    }

private:
    Scene& m_scene;
};
}

BOOST_AUTO_TEST_SUITE(StabilityTests)

BOOST_AUTO_TEST_CASE(transform_uses_translation_rotation_scale_order) {
    Transform transform{};
    transform.Position = {10.0f, 20.0f};
    transform.Scale = {2.0f, 3.0f};
    transform.Rotation = 90.0f;

    const glm::vec4 world = transform.getModelMatrix() * glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
    BOOST_TEST(world.x == 10.0f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(world.y == 22.0f, boost::test_tools::tolerance(1e-5f));

    // Direct field writes are part of the existing API and must invalidate the cache.
    transform.Position = {5.0f, 7.0f};
    const glm::vec4 moved = transform.getModelMatrix() * glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
    BOOST_TEST(moved.x == 5.0f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(moved.y == 7.0f, boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_CASE(camera_sanitizes_zero_viewport_and_preserves_base_zoom) {
    Camera camera{0.0f, 0.0f};
    BOOST_TEST(camera.getViewportSize().x == 1.0f);
    BOOST_TEST(camera.getViewportSize().y == 1.0f);

    camera.setZoom(2.0f);
    FeelingsSystem::FeelingSnapshot feeling{};
    feeling.zoomMul = 0.5f;
    camera.applyFeeling(feeling);
    BOOST_TEST(camera.getZoom() == 1.0f);
    camera.resetFeelingOverrides();
    BOOST_TEST(camera.getZoom() == 2.0f);

    camera.setViewportSize(800.0f, 600.0f);
    camera.getTransform().setPos({25.0f, -10.0f});
    const glm::vec2 screenCenter = camera.worldToScreen({25.0f, -10.0f});
    BOOST_TEST(screenCenter.x == 400.0f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(screenCenter.y == 300.0f, boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_CASE(camera_follow_delay_samples_target_history_without_delaying_gameplay) {
    Camera camera{200.0f, 120.0f};
    camera.setFollowMode(CameraFollowMode::HardLock);
    camera.setFollowDelay(0.1f);

    Transform target{};
    target.setPos({0.0f, 0.0f});
    camera.setTarget(&target);
    camera.update(0.0);

    target.setPos({100.0f, 0.0f});
    camera.update(0.05);
    BOOST_TEST(camera.getTransform().Position.x == 0.0f,
               boost::test_tools::tolerance(1e-5f));
    camera.update(0.05);
    BOOST_TEST(camera.getTransform().Position.x == 0.0f,
               boost::test_tools::tolerance(1e-5f));
    camera.update(0.025);
    BOOST_TEST(camera.getTransform().Position.x == 50.0f,
               boost::test_tools::tolerance(1e-4f));
    BOOST_TEST(camera.getDebugData().delayedTargetPosition.x == 50.0f,
               boost::test_tools::tolerance(1e-4f));
}

BOOST_AUTO_TEST_CASE(camera_hard_lock_is_immediate_and_zoom_transitions_are_eased) {
    Camera camera{200.0f, 120.0f};
    camera.setDamping(0.01f);
    camera.setFollowMode(CameraFollowMode::HardLock);
    Transform target{};
    target.setPos({80.0f, -30.0f});
    camera.setTarget(&target);
    camera.update(1.0f / 60.0f);
    BOOST_TEST(camera.getTransform().Position.x == 80.0f);
    BOOST_TEST(camera.getTransform().Position.y == -30.0f);

    camera.transitionToZoom(2.0f, 1.0f);
    camera.update(0.5f);
    BOOST_TEST(camera.getZoom() == 1.5f,
               boost::test_tools::tolerance(1e-5f));
    camera.update(0.5f);
    BOOST_TEST(camera.getZoom() == 2.0f,
               boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_CASE(camera_shake_is_bounded_and_expires_cleanly) {
    Camera camera{200.0f, 120.0f};
    camera.setShakeLimits({2.0f, 3.0f}, 1.0f);
    camera.shake(100.0f, 0.1f, 20.0f, {1.0f, 2.0f});
    camera.update(0.01f);
    CameraDebugData debug = camera.getDebugData();
    BOOST_TEST(std::abs(debug.effectOffset.x) <= 2.0f);
    BOOST_TEST(std::abs(debug.effectOffset.y) <= 3.0f);
    BOOST_TEST(std::abs(debug.effectRotationDegrees) <= 1.0f);

    camera.update(0.2f);
    debug = camera.getDebugData();
    BOOST_TEST(debug.effectOffset.x == 0.0f);
    BOOST_TEST(debug.effectOffset.y == 0.0f);
    BOOST_TEST(debug.effectRotationDegrees == 0.0f);
}

BOOST_AUTO_TEST_CASE(camera_cinematic_inputs_are_validated) {
    Camera camera{200.0f, 120.0f};
    BOOST_CHECK_THROW(camera.setFollowDelay(-0.1f), std::invalid_argument);
    BOOST_CHECK_THROW(camera.setZoom(std::numeric_limits<float>::quiet_NaN()),
                      std::invalid_argument);
    BOOST_CHECK_THROW(camera.setDamping(-1.0f), std::invalid_argument);
    BOOST_CHECK_THROW(camera.setLookAheadMultiplier(-1.0f), std::invalid_argument);
    BOOST_CHECK_THROW(camera.shake(1.0f, 0.0f), std::invalid_argument);
    BOOST_CHECK_THROW(camera.transitionToZoom(0.0f, 1.0f),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(scene_defers_destruction_during_component_updates) {
    Scene scene;
    Entity& entity = scene.createEntity();
    entity.addComponent<DestroyOnUpdate>(scene);

    BOOST_CHECK_NO_THROW(scene.update(1.0f / 60.0f));
    BOOST_TEST(scene.getEntities().empty());
}

BOOST_AUTO_TEST_CASE(scene_detaches_hinges_before_destroying_their_target) {
    Scene scene;
    Entity& target = scene.createEntity();
    Entity& owner = scene.createEntity();
    auto& hinge = owner.addComponent<HingeComponent>(&target);

    scene.destroyEntity(target);

    BOOST_TEST(hinge.target() == nullptr);
    BOOST_TEST(scene.getEntities().size() == 1u);
}

BOOST_AUTO_TEST_CASE(hinge_rejects_invalid_constraint_parameters) {
    HingeComponent hinge;
    BOOST_CHECK_THROW(hinge.setLimitRange(1.0f, -1.0f),
                      std::invalid_argument);
    BOOST_CHECK_THROW(hinge.setMotorParameters(-1.0f, 2.0f),
                      std::invalid_argument);
    BOOST_CHECK_THROW(hinge.setReferenceAngle(
                          std::numeric_limits<float>::infinity()),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(scene_rejects_invalid_ambient_light) {
    Scene scene;
    scene.setAmbientLight({0.1f, 0.2f, 0.3f});
    BOOST_TEST(scene.ambientLight().y == 0.2f);
    BOOST_CHECK_THROW(scene.setAmbientLight({-0.1f, 0.2f, 0.3f}),
                      std::invalid_argument);
    BOOST_CHECK_THROW(scene.setAmbientLight({
        std::numeric_limits<float>::quiet_NaN(), 0.2f, 0.3f}),
        std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(debug_overlay_is_explicitly_controllable) {
    DebugOverlay::setEnabled(false);
    BOOST_TEST(!DebugOverlay::enabled());
    DebugOverlay::toggle();
    BOOST_TEST(DebugOverlay::enabled());
    DebugOverlay::setEnabled(false);
}

BOOST_AUTO_TEST_CASE(vehicle_mount_rejects_invalid_authoring_values) {
    InputService input;
    VehicleMountComponent mount(input);
    BOOST_CHECK_THROW(mount.setMountRadius(-1.0f), std::invalid_argument);
    BOOST_CHECK_THROW(mount.setMountRadius(std::numeric_limits<float>::infinity()),
                      std::invalid_argument);
    BOOST_CHECK_THROW(mount.setSeatOffset({0.0f,
                                          std::numeric_limits<float>::quiet_NaN()}),
                      std::invalid_argument);
    BOOST_CHECK_THROW(mount.setInteractActionName(""), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
