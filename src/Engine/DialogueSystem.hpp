#ifndef GL2D_DIALOGUESYSTEM_HPP
#define GL2D_DIALOGUESYSTEM_HPP

#include <deque>
#include <string>
#include <vector>
#include <glm/vec2.hpp>

namespace UI { struct UIRenderCommand; }
namespace Audio { class AudioManager; }

// Lightweight dialogue queue that shows popup text and plays voice/audio.
class DialogueSystem {
public:
    struct Line {
        std::string speaker;
        std::string text;
        std::string audioTriggerId; // optional: AudioManager::triggerDialogue id
        std::string audioPath;      // optional: direct path if trigger not used
        float duckDb{-8.0f};
        float duration{0.0f};       // 0 = auto duration from text length
    };

    void enqueue(Line line);
    void clear();
    void update(float dt, Audio::AudioManager& audio);
    void appendCommands(int fbWidth, int fbHeight, std::vector<UI::UIRenderCommand>& out) const;

    bool isActive() const { return m_hasActive; }

private:
    static float estimateDuration(const Line& line);
    void beginLine(const Line& line, Audio::AudioManager& audio);

    std::deque<Line> m_queue;
    Line m_current{};
    float m_timer{0.0f};
    bool m_hasActive{false};
};

#endif // GL2D_DIALOGUESYSTEM_HPP
