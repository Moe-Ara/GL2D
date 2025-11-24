#include <boost/test/unit_test.hpp>

#include "InputSystem/InputEventBus.hpp"

BOOST_AUTO_TEST_SUITE(InputEventBusTests)

BOOST_AUTO_TEST_CASE(publishes_to_registered_listeners) {
    InputEventBus bus;
    int callCount = 0;
    const ListenerID id = bus.addListener("jump", [&](const ActionEvent &evt) {
        ++callCount;
        BOOST_TEST(evt.actionName == "jump");
        BOOST_TEST(static_cast<int>(evt.eventType) == static_cast<int>(InputEventType::ButtonPressed));
    });

    ActionEvent evt{};
    evt.actionName = "jump";
    evt.eventType = InputEventType::ButtonPressed;
    bus.publish(evt);

    BOOST_TEST(id != 0);
    BOOST_TEST(callCount == 1);
}

BOOST_AUTO_TEST_CASE(removing_listener_stops_callbacks) {
    InputEventBus bus;
    int callCount = 0;
    const ListenerID id = bus.addListener("shoot", [&](const ActionEvent &) { ++callCount; });
    bus.removeListener(id);

    ActionEvent evt{};
    evt.actionName = "shoot";
    bus.publish(evt);

    BOOST_TEST(callCount == 0);
}

BOOST_AUTO_TEST_SUITE_END()
