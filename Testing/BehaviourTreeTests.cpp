#include <boost/test/unit_test.hpp>

#include "AISystem/BehaviourTree.hpp"

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
    root->children.push_back(std::move(failCond));
    root->children.push_back(std::move(action));
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
    root->children.push_back(std::move(runner));
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

BOOST_AUTO_TEST_SUITE_END()
