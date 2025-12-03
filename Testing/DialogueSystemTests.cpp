#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "Engine/DialogueSystem.hpp"
#include "AudioSystem/AudioManager.hpp"
#include "UI/UIElements.hpp"

BOOST_AUTO_TEST_SUITE(DialogueSystemTests)

BOOST_AUTO_TEST_CASE(append_commands_handles_long_text_without_crash) {
    DialogueSystem dialogue;
    Audio::AudioManager audio; // Not initialized; safe because we don't request audio playback.

    std::string longText(8000, 'A'); // long payload to exercise wrapping/buffer sizing
    dialogue.enqueue(DialogueSystem::Line{
        .speaker = "Tester",
        .text = longText,
        .audioTriggerId = "",
        .audioPath = "",
        .duckDb = -8.0f,
        .duration = 0.0f});

    dialogue.update(0.016f, audio);

    std::vector<UI::UIRenderCommand> cmds;
    dialogue.appendCommands(1920, 1080, cmds);

    BOOST_TEST(!cmds.empty());
    const bool hasText = std::any_of(cmds.begin(), cmds.end(), [](const UI::UIRenderCommand& c) {
        return !c.text.empty();
    });
    BOOST_TEST(hasText);
}

BOOST_AUTO_TEST_CASE(append_commands_produces_panel_and_text) {
    DialogueSystem dialogue;
    Audio::AudioManager audio;

    dialogue.enqueue(DialogueSystem::Line{
        .speaker = "NPC",
        .text = "Hello there!",
        .audioTriggerId = "",
        .audioPath = "",
        .duckDb = -8.0f,
        .duration = 0.0f});

    dialogue.update(0.016f, audio);

    std::vector<UI::UIRenderCommand> cmds;
    dialogue.appendCommands(1280, 720, cmds);

    BOOST_TEST(cmds.size() >= 3); // border + background + text (speaker/body)
    const bool hasBackground = std::any_of(cmds.begin(), cmds.end(), [](const UI::UIRenderCommand& c) {
        return c.text.empty();
    });
    const bool hasText = std::any_of(cmds.begin(), cmds.end(), [](const UI::UIRenderCommand& c) {
        return !c.text.empty();
    });
    BOOST_TEST(hasBackground);
    BOOST_TEST(hasText);
}

BOOST_AUTO_TEST_CASE(update_must_run_before_commands) {
    DialogueSystem dialogue;
    Audio::AudioManager audio;

    // Append without update: should yield no commands since no active line.
    std::vector<UI::UIRenderCommand> cmds;
    dialogue.appendCommands(800, 600, cmds);
    BOOST_TEST(cmds.empty());

    // After update, commands should be produced.
    dialogue.enqueue(DialogueSystem::Line{.speaker = "S", .text = "---"});
    dialogue.update(0.016f, audio);
    cmds.clear();
    dialogue.appendCommands(800, 600, cmds);
    BOOST_TEST(!cmds.empty());
    const bool hasText = std::any_of(cmds.begin(), cmds.end(), [](const UI::UIRenderCommand& c) { return !c.text.empty(); });
    BOOST_TEST(hasText);
}

BOOST_AUTO_TEST_SUITE_END()
