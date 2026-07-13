#include <boost/test/unit_test.hpp>

#include <limits>
#include <stdexcept>

#include "AISystem/BehaviourTree.hpp"

using AI::BehaviourTree;
using AI::NodeStatus;

struct BTContext {
    int counter{0};
    bool flag{false};
};

BOOST_AUTO_TEST_SUITE(BehaviourTreeTests)

BOOST_AUTO_TEST_CASE(selector_picks_first_success) {
    BehaviourTree<BTContext> bt;
    auto root = BehaviourTree<BTContext>::makeSelector();
    auto failCond = BehaviourTree<BTContext>::makeCondition([](BTContext* ctx) { return ctx && ctx->flag; });
    auto action = BehaviourTree<BTContext>::makeAction([](BTContext* ctx) {
        if (ctx) ++ctx->counter;
        return NodeStatus::Success;
    });
    root->addChild(std::move(failCond));
    root->addChild(std::move(action));
    bt.setRoot(std::move(root));

    BTContext ctx{};
    const auto status = bt.tick(&ctx, 0.0f);
    BOOST_TEST(status == NodeStatus::Success);
    BOOST_TEST(ctx.counter == 1);
}

BOOST_AUTO_TEST_CASE(sequence_resumes_running_child) {
    BehaviourTree<BTContext> bt;
    auto root = BehaviourTree<BTContext>::makeSequence();
    bool firstTick = true;
    auto runner = BehaviourTree<BTContext>::makeAction([&](BTContext* ctx) {
        if (firstTick) {
            firstTick = false;
            return NodeStatus::Running;
        }
        if (ctx) ++ctx->counter;
        return NodeStatus::Success;
    });
    root->addChild(std::move(runner));
    bt.setRoot(std::move(root));

    BTContext ctx{};
    auto status = bt.tick(&ctx, 0.0f);
    BOOST_TEST(status == NodeStatus::Running);
    status = bt.tick(&ctx, 0.0f);
    BOOST_TEST(status == NodeStatus::Success);
    BOOST_TEST(ctx.counter == 1);
}

BOOST_AUTO_TEST_CASE(cooldown_blocks_repeated_success_until_timer_expires) {
    BehaviourTree<BTContext> bt;
    auto action = BehaviourTree<BTContext>::makeAction([](BTContext* ctx) {
        if (ctx) ++ctx->counter;
        return NodeStatus::Success;
    });
    auto root = BehaviourTree<BTContext>::makeCooldown(std::move(action), 0.5f); // 0.5s cooldown
    bt.setRoot(std::move(root));

    BTContext ctx{};
    auto status = bt.tick(&ctx, 0.0f);
    BOOST_TEST(status == NodeStatus::Success);
    BOOST_TEST(ctx.counter == 1);

    status = bt.tick(&ctx, 0.2f);
    BOOST_TEST(status == NodeStatus::Failure); // still on cooldown
    BOOST_TEST(ctx.counter == 1);

    status = bt.tick(&ctx, 0.3f); // total dt 0.5s
    BOOST_TEST(status == NodeStatus::Success);
    BOOST_TEST(ctx.counter == 2);
}

BOOST_AUTO_TEST_CASE(decorators_have_explicit_terminal_semantics) {
    using Tree = BehaviourTree<BTContext>;
    BTContext ctx{};

    Tree inverter;
    inverter.setRoot(Tree::makeInverter(Tree::makeAction(
        [](BTContext*) { return NodeStatus::Failure; })));
    BOOST_TEST(inverter.tick(&ctx) == NodeStatus::Success);

    Tree succeeder;
    succeeder.setRoot(Tree::makeSucceeder(Tree::makeAction(
        [](BTContext*) { return NodeStatus::Failure; })));
    BOOST_TEST(succeeder.tick(&ctx) == NodeStatus::Success);

    Tree failer;
    failer.setRoot(Tree::makeFailer(Tree::makeAction(
        [](BTContext*) { return NodeStatus::Success; })));
    BOOST_TEST(failer.tick(&ctx) == NodeStatus::Failure);
}

BOOST_AUTO_TEST_CASE(decorators_preserve_running_status) {
    using Tree = BehaviourTree<BTContext>;
    BTContext ctx{};

    Tree inverter;
    inverter.setRoot(Tree::makeInverter(Tree::makeAction(
        [](BTContext*) { return NodeStatus::Running; })));
    BOOST_TEST(inverter.tick(&ctx) == NodeStatus::Running);

    Tree succeeder;
    succeeder.setRoot(Tree::makeSucceeder(Tree::makeAction(
        [](BTContext*) { return NodeStatus::Running; })));
    BOOST_TEST(succeeder.tick(&ctx) == NodeStatus::Running);

    Tree failer;
    failer.setRoot(Tree::makeFailer(Tree::makeAction(
        [](BTContext*) { return NodeStatus::Running; })));
    BOOST_TEST(failer.tick(&ctx) == NodeStatus::Running);
}

BOOST_AUTO_TEST_CASE(repeater_runs_one_child_cycle_per_tick) {
    using Tree = BehaviourTree<BTContext>;
    Tree tree;
    tree.setRoot(Tree::makeRepeater(Tree::makeAction([](BTContext* ctx) {
        ++ctx->counter;
        return NodeStatus::Success;
    }), 3));

    BTContext ctx{};
    BOOST_TEST(tree.tick(&ctx) == NodeStatus::Running);
    BOOST_TEST(tree.tick(&ctx) == NodeStatus::Running);
    BOOST_TEST(tree.tick(&ctx) == NodeStatus::Success);
    BOOST_TEST(ctx.counter == 3);

    // A completed finite repeater starts a fresh cycle on its next tick.
    BOOST_TEST(tree.tick(&ctx) == NodeStatus::Running);
    BOOST_TEST(ctx.counter == 4);
}

BOOST_AUTO_TEST_CASE(reset_clears_composite_and_decorator_runtime_state) {
    using Tree = BehaviourTree<BTContext>;
    Tree tree;
    tree.setRoot(Tree::makeCooldown(Tree::makeAction([](BTContext* ctx) {
        ++ctx->counter;
        return NodeStatus::Success;
    }), 10.0f));

    BTContext ctx{};
    BOOST_TEST(tree.tick(&ctx) == NodeStatus::Success);
    BOOST_TEST(tree.tick(&ctx, 0.1f) == NodeStatus::Failure);
    tree.reset();
    BOOST_TEST(tree.tick(&ctx) == NodeStatus::Success);
    BOOST_TEST(ctx.counter == 2);
}

BOOST_AUTO_TEST_CASE(invalid_callbacks_decorators_and_time_are_rejected) {
    using Tree = BehaviourTree<BTContext>;
    BOOST_CHECK_THROW(Tree::makeAction({}), std::invalid_argument);
    BOOST_CHECK_THROW(Tree::makeCondition({}), std::invalid_argument);
    BOOST_CHECK_THROW(Tree::makeInverter(nullptr), std::invalid_argument);
    BOOST_CHECK_THROW(Tree::makeCooldown(Tree::makeSelector(), -0.1f), std::invalid_argument);
    BOOST_CHECK_THROW(Tree::makeRepeater(Tree::makeSelector(), -2), std::invalid_argument);
    auto leaf = Tree::makeAction([](BTContext*) { return NodeStatus::Success; });
    BOOST_CHECK_THROW(leaf->addChild(Tree::makeSelector()), std::logic_error);

    Tree tree;
    tree.setRoot(Tree::makeSelector());
    BOOST_CHECK_THROW(tree.tick(nullptr, -0.1f), std::invalid_argument);
    BOOST_CHECK_THROW(tree.tick(nullptr, std::numeric_limits<float>::quiet_NaN()),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
