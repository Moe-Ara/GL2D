#include <boost/test/unit_test.hpp>

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Physics/RigidBody.hpp"

#include <memory>
#include <vector>

namespace {
struct FastBodyScene {
    std::vector<std::unique_ptr<Entity>> entities;
    RigidBody* movingBody{nullptr};

    explicit FastBodyScene(CollisionDetection detection) {
        entities.push_back(std::make_unique<Entity>());
        Entity& mover = *entities.back();
        mover.addComponent<TransformComponent>();
        mover.addComponent<ColliderComponent>(
            std::make_unique<CircleCollider>(0.5f));
        auto body = std::make_unique<RigidBody>(1.0f, RigidBodyType::DYNAMIC);
        movingBody = body.get();
        movingBody->setVelocity({100.0f, 0.0f});
        movingBody->setCollisionDetection(detection);
        mover.addComponent<RigidBodyComponent>(std::move(body));

        entities.push_back(std::make_unique<Entity>());
        Entity& wall = *entities.back();
        wall.addComponent<TransformComponent>();
        wall.addComponent<ColliderComponent>(
            std::make_unique<AABBCollider>(
                glm::vec2{5.0f, -2.0f}, glm::vec2{5.5f, 2.0f}));
        wall.addComponent<RigidBodyComponent>(
            std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC));
    }
};
}

BOOST_AUTO_TEST_SUITE(PhysicsEngineTests)

BOOST_AUTO_TEST_CASE(substepped_detection_prevents_fast_body_tunnelling) {
    FastBodyScene scene{CollisionDetection::SUBSTEPPED};
    PhysicsEngine physics{{0.0f, 0.0f}};
    physics.step(0.1f, scene.entities);

    BOOST_TEST(scene.movingBody->getPosition().x <= 4.5001f);
    BOOST_TEST(scene.movingBody->getVelocity().x == 0.0f,
               boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(discrete_detection_remains_available_when_requested) {
    FastBodyScene scene{CollisionDetection::DISCRETE};
    PhysicsEngine physics{{0.0f, 0.0f}};
    physics.step(0.1f, scene.entities);

    BOOST_TEST(scene.movingBody->getPosition().x == 10.0f,
               boost::test_tools::tolerance(0.0001f));
}

namespace {
// A dynamic circle launched at a static wall, used to observe contact response.
struct ContactScene {
    std::vector<std::unique_ptr<Entity>> entities;
    RigidBody* ball{nullptr};
    RigidBody* wall{nullptr};

    explicit ContactScene(glm::vec2 velocity) {
        entities.push_back(std::make_unique<Entity>());
        Entity& mover = *entities.back();
        mover.addComponent<TransformComponent>();
        mover.addComponent<ColliderComponent>(
            std::make_unique<CircleCollider>(0.5f));
        auto body = std::make_unique<RigidBody>(1.0f, RigidBodyType::DYNAMIC);
        ball = body.get();
        ball->setVelocity(velocity);
        mover.addComponent<RigidBodyComponent>(std::move(body));

        entities.push_back(std::make_unique<Entity>());
        Entity& wallEntity = *entities.back();
        wallEntity.addComponent<TransformComponent>();
        wallEntity.addComponent<ColliderComponent>(
            std::make_unique<AABBCollider>(
                glm::vec2{5.0f, -2.0f}, glm::vec2{5.5f, 2.0f}));
        auto wallBody = std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC);
        wall = wallBody.get();
        wallEntity.addComponent<RigidBodyComponent>(std::move(wallBody));
    }
};
}

BOOST_AUTO_TEST_CASE(restitution_reverses_a_head_on_impact) {
    ContactScene scene{{100.0f, 0.0f}};
    scene.ball->setRestitution(1.0f);
    scene.wall->setRestitution(1.0f);
    PhysicsEngine physics{{0.0f, 0.0f}};
    physics.step(0.1f, scene.entities);

    // A perfectly elastic contact sends the ball back the way it came.
    BOOST_TEST(scene.ball->getVelocity().x < -50.0f);
    BOOST_TEST(scene.ball->getPosition().x < 5.0f);
}

BOOST_AUTO_TEST_CASE(inelastic_default_absorbs_a_head_on_impact) {
    ContactScene scene{{100.0f, 0.0f}};
    PhysicsEngine physics{{0.0f, 0.0f}};
    physics.step(0.1f, scene.entities);

    // Default restitution is zero: the ball stops at the wall instead of bouncing.
    BOOST_TEST(scene.ball->getVelocity().x == 0.0f,
               boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(friction_arrests_tangential_sliding_at_a_contact) {
    ContactScene frictional{{100.0f, -30.0f}};
    PhysicsEngine physics{{0.0f, 0.0f}};
    physics.step(0.1f, frictional.entities);

    ContactScene frictionless{{100.0f, -30.0f}};
    frictionless.ball->setFriction(0.0f);
    physics.step(0.1f, frictionless.entities);

    // The frictionless ball keeps sliding down the wall face; the default-
    // material ball has its tangential (vertical) motion damped by friction.
    BOOST_TEST(frictionless.ball->getVelocity().y < -25.0f);
    BOOST_TEST(frictional.ball->getVelocity().y >
               frictionless.ball->getVelocity().y);
}

BOOST_AUTO_TEST_CASE(material_setters_reject_out_of_range_values) {
    RigidBody body{1.0f, RigidBodyType::DYNAMIC};
    BOOST_CHECK_THROW(body.setFriction(-0.1f), std::invalid_argument);
    BOOST_CHECK_THROW(body.setRestitution(-0.1f), std::invalid_argument);
    BOOST_CHECK_THROW(body.setRestitution(1.5f), std::invalid_argument);
    BOOST_CHECK_NO_THROW(body.setFriction(2.0f));
    BOOST_CHECK_NO_THROW(body.setRestitution(0.5f));
}

BOOST_AUTO_TEST_SUITE_END()
