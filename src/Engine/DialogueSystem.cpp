#include "DialogueSystem.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

#include "AudioSystem/AudioManager.hpp"
#include "UI/DialogueBox.hpp"
#include "UI/UIElements.hpp"

void DialogueSystem::enqueue(Line line) {
    m_queue.push_back(std::move(line));
}

void DialogueSystem::clear() {
    m_queue.clear();
    m_hasActive = false;
    m_timer = 0.0f;
}

float DialogueSystem::estimateDuration(const Line& line) {
    const float chars = static_cast<float>(line.text.size());
    return std::clamp(0.8f + chars * 0.045f, 1.5f, 8.0f);
}

void DialogueSystem::beginLine(const Line& line, Audio::AudioManager& audio) {
    m_current = line;
    m_hasActive = true;
    m_timer = (line.duration > 0.0f) ? line.duration : estimateDuration(line);

    if (!line.audioTriggerId.empty()) {
        audio.triggerDialogue(line.audioTriggerId);
    } else if (!line.audioPath.empty()) {
        audio.playDialogue(line.audioPath, line.duckDb);
    }
}

void DialogueSystem::update(float dt, Audio::AudioManager& audio) {
    if (!m_hasActive && !m_queue.empty()) {
        beginLine(m_queue.front(), audio);
        m_queue.pop_front();
    }

    if (!m_hasActive) return;

    m_timer -= dt;
    if (m_timer <= 0.0f) {
        m_hasActive = false;
    }
}

void DialogueSystem::appendCommands(int fbWidth, int fbHeight, std::vector<UI::UIRenderCommand>& out) const {
    if (!m_hasActive) return;

    UI::DialogueBox box("dialogue");
    UI::UITransform tf{};
    tf.position = {24.0f, 24.0f};
    tf.size = {static_cast<float>(fbWidth) - 48.0f, 260.0f};
    tf.anchor = {0.0f, 0.0f};
    tf.pivot = {0.0f, 0.0f};
    box.transform() = tf;
    box.setSpeaker(m_current.speaker);
    box.setText(m_current.text);
    box.setFontScale(2.6f);
    box.setPadding(20.0f);
    box.setSpeakerSpacing(10.0f);
    box.setTextOffset({4.0f, 4.0f});
    box.setColors({0.03f, 0.03f, 0.05f, 0.9f},
                  {0.25f, 0.7f, 1.0f, 1.0f},
                  {0.45f, 0.9f, 1.0f},
                  {0.95f, 0.95f, 0.95f});
    box.buildRenderCommands(out, {static_cast<float>(fbWidth), static_cast<float>(fbHeight)});
}
