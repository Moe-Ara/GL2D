#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>

#include "Physics/RigidBody.hpp"
#include "Physics/Collision/ACollider.hpp"
#include "Physics/Collision/AABB.hpp"
#include "Utils/Transform.hpp"

namespace {
    class RecordingCollider : public ACollider {
    public:
        ColliderType getType() const override { return ColliderType::AABB; }
        AABB getAABB() const override { return AABB{{0.0f, 0.0f}, {1.0f, 1.0f}}; }

        void setTransform(Transform *transform) override {
            lastTransform = transform;
            ACollider::setTransform(transform);
        }

        Transform *lastTransform{nullptr};
    };
}

BOOST_AUTO_TEST_SUITE(RigidBodyTests)

BOOST_AUTO_TEST_CASE(dynamic_body_integrates_and_updates_transform) {
    Transform transform{};
    RecordingCollider collider;
    RigidBody body(2.0f, RigidBodyType::DYNAMIC);
    body.setTransform(&transform);
    body.setCollider(&collider);

    body.applyForce(glm::vec2{4.0f, 0.0f});
    body.integrate(0.5f);

    BOOST_TEST(body.getVelocity().x == 1.0f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(body.getVelocity().y == 0.0f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(transform.Position.x == 0.5f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(transform.Position.y == 0.0f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(collider.lastTransform == &transform);
}

BOOST_AUTO_TEST_CASE(static_body_ignores_forces) {
    RigidBody body(1.0f, RigidBodyType::STATIC);
    body.applyForce(glm::vec2{10.0f, 0.0f});
    body.integrate(1.0f);

    BOOST_TEST(body.getVelocity().x == 0.0f);
    BOOST_TEST(body.getVelocity().y == 0.0f);
}

BOOST_AUTO_TEST_CASE(force_generators_remove_themselves) {
    RigidBody body(1.0f, RigidBodyType::DYNAMIC);
    int callCount = 0;
    body.addForceGenerator([&](RigidBody &rb, float /*dt*/) {
        ++callCount;
        rb.applyForce(glm::vec2{1.0f, 0.0f});
        return callCount < 2;
    });

    body.integrate(1.0f);
    body.integrate(1.0f);
    body.integrate(1.0f);

    BOOST_TEST(callCount == 2);
    BOOST_TEST(body.getVelocity().x == 2.0f, boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_SUITE_END()
