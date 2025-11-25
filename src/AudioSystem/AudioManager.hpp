//
// Created by Mohamad on 25/11/2025.
//

#ifndef GL2D_AUDIOMANAGER_HPP
#define GL2D_AUDIOMANAGER_HPP
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <glm/vec3.hpp>
#include "third_party/miniaudio.h"
namespace Audio {

    enum class Bus {
        Music,
        SFX,
        VO,
        Ambience
    };

    struct PlayParams {
        float volume = 1.0f;
        float pitch = 1.0f;
        glm::vec3 position{0.0f};
        bool spatial = false;
        float minDistance = 1.0f;
        float maxDistance = 50.0f;
        bool loop = false;
        bool stream = false; // stream long files (music) instead of full decode
    };

    struct PlaylistEntry {
        std::string path;
        bool loop{false};
        int delayAfterMs{0}; // wait this long before starting next entry
    };

    class AudioManager {
    public:
        AudioManager();

        virtual ~AudioManager();

        AudioManager(const AudioManager &other) = delete;

        AudioManager &operator=(const AudioManager &other) = delete;

        AudioManager(AudioManager &&other) = delete;

        AudioManager &operator=(AudioManager &&other) = delete;
        void init();
        void shutdown();

        bool playMusic(const std::string& path, bool loop, int fadeMs);
        void queueNextMusic(const std::string& path, bool loop, int fadeMs);
        void setPlaylist(const std::vector<PlaylistEntry>& tracks);
        bool startPlaylist(int fadeMs);
        void playSFX(const std::string& path, const PlayParams& p);
        void playDialogue(const std::string& path, float duckDb);

        void setBusVolume(Bus b, float gain);
        void setListener(const glm::vec3& pos, const glm::vec3& fwd, const glm::vec3& up);
        void update(); // call periodically to prune finished sounds and restore ducking
        void notifyMusicStopped(ma_sound* who);

        // Trigger registration/firing
        void registerSfxTrigger(const std::string& id, const std::string& path, const PlayParams& params, int maxSimultaneous = 1);
        void triggerSfx(const std::string& id, const glm::vec3* positionOverride = nullptr);

        void registerDialogueTrigger(const std::string& id, const std::string& path, float duckDb, int maxSimultaneous = 1);
        void triggerDialogue(const std::string& id);
    private:
        struct SoundDeleter {
            void operator()(ma_sound* s) const {
                if (s != nullptr) {
                    ma_sound_uninit(s);
                    delete s;
                }
            }
        };

        using SoundPtr = std::unique_ptr<ma_sound, SoundDeleter>;

        static float dbToGain(float db) ;
        void pruneFinishedSounds();
        SoundPtr makeSound(const std::string& path, ma_uint32 flags, ma_sound_group* group);
        void playSfxInternal(const std::string& path, const PlayParams& p, const std::string& triggerId);
        void playDialogueInternal(const std::string& path, float duckDb, const std::string& triggerId);
        ma_sound* initMusicSlot(ma_sound* slot, bool& slotInit, const std::string& path, bool loop, float volume);
        void cleanupSlot(ma_sound* slot, bool& slotInit);

        ma_resource_manager m_resourceManager{};
        ma_engine m_engine{};
        ma_sound_group m_musicGroup{};
        ma_sound_group m_sfxGroup{};
        ma_sound_group m_voGroup{};
        ma_sound_group m_ambGroup{};
        ma_sound m_musicSoundA{};
        ma_sound m_musicSoundB{};
        bool m_musicSlotAInit{false};
        bool m_musicSlotBInit{false};
        std::string m_musicPathA;
        std::string m_musicPathB;
        ma_sound* m_currentMusic{nullptr};
        ma_sound* m_fadingMusic{nullptr};
        bool m_musicActive{false};
        bool m_musicLoop{false};
        bool m_usingA{true};

        // Crossfade / pending music
        std::string m_pendingMusicPath;
        bool m_pendingMusicLoop{false};
        int m_pendingFadeMs{0};
        bool m_hasPendingMusic{false};
        ma_sound* m_pendingSlot{nullptr};
        bool* m_pendingSlotInit{nullptr};
        struct CrossfadeState {
            bool active{false};
            ma_sound* out{nullptr};
            bool* outInit{nullptr};
            ma_sound* in{nullptr};
            std::chrono::steady_clock::time_point endTime{};
            int durationMs{0};
        } m_crossfade;

        float m_musicBaseVolume{1.0f};
        float m_sfxBaseVolume{1.0f};
        float m_voBaseVolume{1.0f};
        float m_ambBaseVolume{1.0f};
        std::vector<PlaylistEntry> m_playlist;
        size_t m_playlistIndex{0};
        bool m_playlistActive{false};
        bool m_playlistWaiting{false};
        std::chrono::steady_clock::time_point m_playlistNextStart{};
        int m_playlistFadeMs{0};
        int m_currentTrackDelayMs{0};

        struct ActiveSfx { SoundPtr sound; std::string triggerId; };
        struct ActiveDialogue { SoundPtr sound; std::string triggerId; };

        std::vector<ActiveSfx> m_activeSfx;
        std::vector<ActiveDialogue> m_activeDialogue;
        std::mutex m_dialogueMutex;

        struct SfxTriggerConfig {
            std::string path;
            PlayParams params;
            int maxSimultaneous{1};
        };
        struct DialogueTriggerConfig {
            std::string path;
            float duckDb{-6.0f};
            int maxSimultaneous{1};
        };
        std::unordered_map<std::string, SfxTriggerConfig> m_sfxTriggers;
        std::unordered_map<std::string, DialogueTriggerConfig> m_dialogueTriggers;

        bool m_initialized{false};
    };

} // Audio

#endif //GL2D_AUDIOMANAGER_HPP
