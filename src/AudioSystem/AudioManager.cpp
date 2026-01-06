//
// Created by Mohamad on 25/11/2025.
//

#define MINIAUDIO_IMPLEMENTATION
#include "AudioManager.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <filesystem>
#include "Exceptions/SubsystemExceptions.hpp"

namespace Audio {

    namespace {
        void music_end_callback(void* pUserData, ma_sound* pSound) {
            (void)pSound;
            AudioManager* mgr = reinterpret_cast<AudioManager*>(pUserData);
        if (mgr != nullptr) {
                // Only clear active if this is the current music slot.
                mgr->notifyMusicStopped(pSound);
            }
        }
    }

    AudioManager::AudioManager() = default;
    AudioManager::~AudioManager() {
        shutdown();
    }

    void AudioManager::init() {
        if (m_initialized) {
            return;
        }

        ma_resource_manager_config rmConfig = ma_resource_manager_config_init();
        rmConfig.jobThreadCount = 1;

        if (ma_resource_manager_init(&rmConfig, &m_resourceManager) != MA_SUCCESS) {
            throw Engine::AudioException("Failed to init miniaudio resource manager");
        }

        ma_engine_config engineConfig = ma_engine_config_init();
        engineConfig.pResourceManager = &m_resourceManager;

        if (ma_engine_init(&engineConfig, &m_engine) != MA_SUCCESS) {
            ma_resource_manager_uninit(&m_resourceManager);
            throw Engine::AudioException("Failed to init miniaudio engine");
        }

        // Buses
        if (ma_sound_group_init(&m_engine, 0, nullptr, &m_musicGroup) != MA_SUCCESS ||
            ma_sound_group_init(&m_engine, 0, nullptr, &m_sfxGroup) != MA_SUCCESS ||
            ma_sound_group_init(&m_engine, 0, nullptr, &m_voGroup) != MA_SUCCESS ||
            ma_sound_group_init(&m_engine, 0, nullptr, &m_ambGroup) != MA_SUCCESS) {
            ma_engine_uninit(&m_engine);
            ma_resource_manager_uninit(&m_resourceManager);
            throw Engine::AudioException("Failed to init miniaudio sound groups");
        }

        // Apply initial bus gains.
        ma_sound_group_set_volume(&m_musicGroup, m_musicBaseVolume);
        ma_sound_group_set_volume(&m_sfxGroup, m_sfxBaseVolume);
        ma_sound_group_set_volume(&m_voGroup, m_voBaseVolume);
        ma_sound_group_set_volume(&m_ambGroup, m_ambBaseVolume);

        // Listener defaults
        ma_engine_listener_set_position(&m_engine, 0, 0.0f, 0.0f, 0.0f);
        ma_engine_listener_set_direction(&m_engine, 0, 0.0f, 0.0f, -1.0f);
        ma_engine_listener_set_world_up(&m_engine, 0, 0.0f, 1.0f, 0.0f);

        m_initialized = true;
    }

    void AudioManager::shutdown() {
        if (!m_initialized) {
            return;
        }

        // Stop and free active sounds
        m_activeSfx.clear();
        m_activeDialogue.clear();

        cleanupSlot(&m_musicSoundA, m_musicSlotAInit);
        cleanupSlot(&m_musicSoundB, m_musicSlotBInit);
        m_currentMusic = nullptr;
        m_fadingMusic = nullptr;
        m_pendingSlot = nullptr;
        m_pendingSlotInit = nullptr;
        m_musicActive = false;
        m_hasPendingMusic = false;
        m_crossfade = {};
        m_playlistActive = false;
        m_playlistWaiting = false;
        m_playlist.clear();

        ma_sound_group_uninit(&m_musicGroup);
        ma_sound_group_uninit(&m_sfxGroup);
        ma_sound_group_uninit(&m_voGroup);
        ma_sound_group_uninit(&m_ambGroup);

        ma_engine_uninit(&m_engine);
        ma_resource_manager_uninit(&m_resourceManager);
        m_initialized = false;
    }

    float AudioManager::dbToGain(float db) {
        return std::pow(10.0f, db / 20.0f);
    }

    AudioManager::SoundPtr AudioManager::makeSound(const std::string& path, ma_uint32 flags, ma_sound_group* group) {
        auto snd = SoundPtr(new ma_sound{}, SoundDeleter{});
        ma_result res = ma_sound_init_from_file(&m_engine, path.c_str(), flags, group, nullptr, snd.get());
        if (res != MA_SUCCESS) {
            std::cerr << "[Audio] Failed to load sound: " << path << " (ma_result=" << res << ": " << ma_result_description(res) << ")\n";
            return nullptr;
        }
        return snd;
    }

    ma_sound* AudioManager::initMusicSlot(ma_sound* slot, bool& slotInit, const std::string& path, bool loop, float volume) {
        if (slotInit) {
            ma_sound_uninit(slot);
            slotInit = false;
        }
        ma_uint32 flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_ASYNC | MA_SOUND_FLAG_NO_SPATIALIZATION;
        ma_result res = ma_sound_init_from_file(&m_engine, path.c_str(), flags, &m_musicGroup, nullptr, slot);
        if (res != MA_SUCCESS) {
            std::cerr << "[Audio] Failed to init music slot for " << path << " (ma_result=" << res << ": " << ma_result_description(res) << ")\n";
            return nullptr;
        }
        ma_sound_set_looping(slot, loop ? MA_TRUE : MA_FALSE);
        ma_sound_set_volume(slot, volume);
        ma_sound_set_spatialization_enabled(slot, MA_FALSE);
        slotInit = true;
        if (slot == &m_musicSoundA) m_musicPathA = path; else if (slot == &m_musicSoundB) m_musicPathB = path;
        return slot;
    }

    void AudioManager::cleanupSlot(ma_sound* slot, bool& slotInit) {
        if (slotInit) {
            ma_sound_uninit(slot);
            slotInit = false;
        }
    }

    void AudioManager::pruneFinishedSounds() {
        auto removeStoppedSfx = [](const ActiveSfx& s) {
            return s.sound == nullptr || !ma_sound_is_playing(s.sound.get());
        };
        m_activeSfx.erase(std::remove_if(m_activeSfx.begin(), m_activeSfx.end(), removeStoppedSfx), m_activeSfx.end());
        {
            std::lock_guard<std::mutex> lock(m_dialogueMutex);
            auto removeStoppedDialogue = [](const ActiveDialogue& d) {
                return d.sound == nullptr || !ma_sound_is_playing(d.sound.get());
            };
            size_t before = m_activeDialogue.size();
            m_activeDialogue.erase(std::remove_if(m_activeDialogue.begin(), m_activeDialogue.end(), removeStoppedDialogue), m_activeDialogue.end());
            size_t after = m_activeDialogue.size();
            if (before != after && m_activeDialogue.empty()) {
                // restore music volume when no dialogues remain
                ma_sound_group_set_volume(&m_musicGroup, m_musicBaseVolume);
            }
        }
    }

    bool AudioManager::playMusic(const std::string& path, bool loop, int fadeMs) {
        if (!m_initialized) {
            return false;
        }
        std::filesystem::path p(path);
        auto absPath = std::filesystem::absolute(p);
        if (!std::filesystem::exists(p)) {
            std::cerr << "[Audio] Music file not found: " << absPath.string() << "\n";
            return false;
        }
        // Reset pending and current.
        m_hasPendingMusic = false;
        if (m_musicActive && m_currentMusic) {
            // fade out old if needed?
            cleanupSlot(m_currentMusic, m_usingA ? m_musicSlotAInit : m_musicSlotBInit);
            m_currentMusic = nullptr;
            m_musicActive = false;
        }
        if (m_fadingMusic) {
            cleanupSlot(m_fadingMusic, (m_fadingMusic == &m_musicSoundA) ? m_musicSlotAInit : m_musicSlotBInit);
            m_fadingMusic = nullptr;
        }

        ma_sound* slot = m_usingA ? &m_musicSoundA : &m_musicSoundB;
        bool& slotInit = m_usingA ? m_musicSlotAInit : m_musicSlotBInit;
        slot = initMusicSlot(slot, slotInit, path, loop, m_musicBaseVolume);
        if (!slot) {
            return false;
        }
        if (fadeMs > 0) {
            ma_sound_set_fade_in_milliseconds(slot, 0.0f, m_musicBaseVolume, fadeMs);
        }
        ma_result startRes = ma_sound_start(slot);
        if (startRes != MA_SUCCESS) {
            std::cerr << "[Audio] Failed to start playback for " << path << " (ma_result=" << startRes << ": " << ma_result_description(startRes) << ")\n";
            cleanupSlot(slot, slotInit);
            return false;
        }

        ma_uint64 lengthFrames = 0;
        ma_result lenRes = ma_sound_get_length_in_pcm_frames(slot, &lengthFrames);
        ma_uint32 sr = 0;
        ma_uint32 ch = 0;
        ma_format fmt;
        ma_sound_get_data_format(slot, &fmt, &ch, &sr, nullptr, 0);
        double lengthSeconds = (lenRes == MA_SUCCESS && sr > 0) ? static_cast<double>(lengthFrames) / static_cast<double>(sr) : -1.0;

        m_currentMusic = slot;
        m_musicActive = true;
        m_crossfade = {};
        return true;
    }

    void AudioManager::setPlaylist(const std::vector<PlaylistEntry>& tracks) {
        m_playlist = tracks;
        m_playlistIndex = 0;
        m_playlistActive = false;
        m_playlistWaiting = false;
    }

    bool AudioManager::startPlaylist(int fadeMs) {
        if (m_playlist.empty()) return false;
        m_playlistIndex = 0;
        m_playlistActive = true;
        m_playlistWaiting = false;
        m_playlistFadeMs = fadeMs;
        return playMusic(m_playlist[m_playlistIndex].path, m_playlist[m_playlistIndex].loop, fadeMs);
    }

    void AudioManager::queueNextMusic(const std::string& path, bool loop, int fadeMs) {
        m_pendingMusicPath = path;
        m_pendingMusicLoop = loop;
        m_pendingFadeMs = fadeMs;
        m_hasPendingMusic = true;
        m_pendingSlot = nullptr;
        m_pendingSlotInit = nullptr;
    }

    void AudioManager::playSFX(const std::string& path, const PlayParams& p) {
        playSfxInternal(path, p, "");
    }

    void AudioManager::playSfxInternal(const std::string& path, const PlayParams& p, const std::string& triggerId) {
        if (!m_initialized) {
            return;
        }

        pruneFinishedSounds();

        ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;
        if (p.stream) {
            flags |= MA_SOUND_FLAG_STREAM;
        }

        auto snd = makeSound(path, flags, &m_sfxGroup);
        if (!snd) {
            return;
        }

        ma_sound_set_volume(snd.get(), m_sfxBaseVolume * p.volume);
        ma_sound_set_pitch(snd.get(), p.pitch);
        ma_sound_set_looping(snd.get(), p.loop ? MA_TRUE : MA_FALSE);

        if (p.spatial) {
            ma_sound_set_spatialization_enabled(snd.get(), MA_TRUE);
            ma_sound_set_position(snd.get(), p.position.x, p.position.y, p.position.z);
            ma_sound_set_min_distance(snd.get(), p.minDistance);
            ma_sound_set_max_distance(snd.get(), p.maxDistance);
        } else {
            ma_sound_set_spatialization_enabled(snd.get(), MA_FALSE);
        }

        ma_sound_start(snd.get());
        m_activeSfx.push_back({ std::move(snd), triggerId });
    }

    void AudioManager::playDialogue(const std::string& path, float duckDb) {
        playDialogueInternal(path, duckDb, "");
    }

    void AudioManager::playDialogueInternal(const std::string& path, float duckDb, const std::string& triggerId) {
        if (!m_initialized) {
            return;
        }

        pruneFinishedSounds();

        ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;
        auto snd = makeSound(path, flags, &m_voGroup);
        if (!snd) {
            return;
        }

        ma_sound_set_volume(snd.get(), m_voBaseVolume);
        ma_sound_set_looping(snd.get(), MA_FALSE);
        ma_sound_start(snd.get());

        {
            std::lock_guard<std::mutex> lock(m_dialogueMutex);
            m_activeDialogue.push_back({ std::move(snd), triggerId });
            float duckGain = dbToGain(duckDb);
            if (duckDb < 0.0f && duckGain < 1.0f) {
                ma_sound_group_set_volume(&m_musicGroup, m_musicBaseVolume * duckGain);
            }
        }
    }

    void AudioManager::setBusVolume(Bus b, float gain) {
        if (!m_initialized) {
            return;
        }
        switch (b) {
            case Bus::Music:
                m_musicBaseVolume = gain;
                ma_sound_group_set_volume(&m_musicGroup, gain);
                break;
            case Bus::SFX:
                m_sfxBaseVolume = gain;
                ma_sound_group_set_volume(&m_sfxGroup, gain);
                break;
            case Bus::VO:
                m_voBaseVolume = gain;
                ma_sound_group_set_volume(&m_voGroup, gain);
                break;
            case Bus::Ambience:
                m_ambBaseVolume = gain;
                ma_sound_group_set_volume(&m_ambGroup, gain);
                break;
            default:
                break;
        }
    }

    void AudioManager::setListener(const glm::vec3& pos, const glm::vec3& fwd, const glm::vec3& up) {
        if (!m_initialized) {
            return;
        }
        ma_engine_listener_set_position(&m_engine, 0, pos.x, pos.y, pos.z);
        ma_engine_listener_set_direction(&m_engine, 0, fwd.x, fwd.y, fwd.z);
        ma_engine_listener_set_world_up(&m_engine, 0, up.x, up.y, up.z);
    }

    void AudioManager::registerSfxTrigger(const std::string& id, const std::string& path, const PlayParams& params, int maxSimultaneous) {
        int capped = (maxSimultaneous > 1) ? maxSimultaneous : 1;
        m_sfxTriggers[id] = SfxTriggerConfig{path, params, capped};
    }

    void AudioManager::triggerSfx(const std::string& id, const glm::vec3* positionOverride) {
        auto it = m_sfxTriggers.find(id);
        if (it == m_sfxTriggers.end()) {
            return;
        }
        pruneFinishedSounds();

        int activeCount = 0;
        for (const auto& s : m_activeSfx) {
            if (s.triggerId == id && s.sound && ma_sound_is_playing(s.sound.get())) {
                activeCount++;
            }
        }
        if (activeCount >= it->second.maxSimultaneous) {
            return; // drop if exceeding per-trigger limit
        }

        PlayParams params = it->second.params;
        if (positionOverride != nullptr) {
            params.position = *positionOverride;
            params.spatial = true;
        }
        playSfxInternal(it->second.path, params, id);
    }

    void AudioManager::registerDialogueTrigger(const std::string& id, const std::string& path, float duckDb, int maxSimultaneous) {
        int capped = (maxSimultaneous > 1) ? maxSimultaneous : 1;
        m_dialogueTriggers[id] = DialogueTriggerConfig{path, duckDb, capped};
    }

    void AudioManager::triggerDialogue(const std::string& id) {
        auto it = m_dialogueTriggers.find(id);
        if (it == m_dialogueTriggers.end()) {
            return;
        }
        pruneFinishedSounds();

        int activeCount = 0;
        {
            std::lock_guard<std::mutex> lock(m_dialogueMutex);
            for (const auto& d : m_activeDialogue) {
                if (d.triggerId == id && d.sound && ma_sound_is_playing(d.sound.get())) {
                    activeCount++;
                }
            }
        }
        if (activeCount >= it->second.maxSimultaneous) {
            return; // drop if exceeding per-trigger limit
        }

        playDialogueInternal(it->second.path, it->second.duckDb, id);
    }

    void AudioManager::notifyMusicStopped(ma_sound* who) {
        if (who == m_currentMusic) {
            m_musicActive = false;
        }
    }

    void AudioManager::applyFeeling(const FeelingsSystem::FeelingSnapshot &snapshot) {
        if (!m_initialized) return;

        if (snapshot.musicVolume.has_value()) {
            setBusVolume(Bus::Music, *snapshot.musicVolume);
        }
        if (snapshot.sfxVolumeMul.has_value()) {
            setBusVolume(Bus::SFX, *snapshot.sfxVolumeMul);
        }

        if (snapshot.musicTrackId.has_value() && !snapshot.musicTrackId->empty()) {
            if (m_activeFeelingMusic != *snapshot.musicTrackId) {
                playMusic(*snapshot.musicTrackId, true, static_cast<int>(snapshot.blendInMs));
                m_activeFeelingMusic = *snapshot.musicTrackId;
            }
        }
    }

    void AudioManager::update() {
        if (!m_initialized) {
            return;
        }
        pruneFinishedSounds();

        auto now = std::chrono::steady_clock::now();

        if (m_crossfade.active && now >= m_crossfade.endTime) {
            if (m_crossfade.out && m_crossfade.outInit && *m_crossfade.outInit) {
                cleanupSlot(m_crossfade.out, *m_crossfade.outInit);
            }
        m_crossfade = {};
    }

        if (!m_crossfade.active && m_hasPendingMusic) {
            bool shouldCrossfadeNow = false;
            if (m_musicActive && m_currentMusic != nullptr) {
                ma_uint64 cursor = 0;
                ma_uint64 lengthFrames = 0;
                ma_result lenRes = ma_sound_get_length_in_pcm_frames(m_currentMusic, &lengthFrames);
                ma_sound_get_cursor_in_pcm_frames(m_currentMusic, &cursor);
                ma_uint32 sr = 0;
                ma_uint32 ch = 0;
                ma_format fmt;
                ma_sound_get_data_format(m_currentMusic, &fmt, &ch, &sr, nullptr, 0);
                bool lengthKnown = (lenRes == MA_SUCCESS && lengthFrames > 0 && sr > 0);
                if (!lengthKnown) {
                    shouldCrossfadeNow = true; // unknown length → crossfade immediately
                } else if (m_pendingFadeMs > 0) {
                    ma_uint64 fadeFrames = (static_cast<ma_uint64>(sr) * static_cast<ma_uint64>(m_pendingFadeMs)) / 1000;
                    ma_uint64 remaining = (cursor >= lengthFrames) ? 0 : (lengthFrames - cursor);
                    if (remaining <= fadeFrames) shouldCrossfadeNow = true;
                }
            } else {
                shouldCrossfadeNow = true;
            }

            if (shouldCrossfadeNow) {
                ma_sound* nextSlot = m_usingA ? &m_musicSoundB : &m_musicSoundA;
                bool& nextInit = m_usingA ? m_musicSlotBInit : m_musicSlotAInit;
                nextSlot = initMusicSlot(nextSlot, nextInit, m_pendingMusicPath, m_pendingMusicLoop, 0.0001f);
                if (nextSlot == nullptr) {
                    m_hasPendingMusic = false;
                } else {
                    ma_sound_seek_to_pcm_frame(nextSlot, 0);
                    ma_result startRes = ma_sound_start(nextSlot);
                    if (startRes != MA_SUCCESS) {
                        std::cerr << "[Audio] Failed to start pending music during crossfade (ma_result=" << startRes << ": " << ma_result_description(startRes) << ")\n";
                        m_hasPendingMusic = false;
                    } else {
                        if (!ma_sound_is_playing(nextSlot)) {
                            std::cerr << "[Audio] Warning: pending music not playing after start\n";
                        }
                        ma_sound_set_volume(nextSlot, m_musicBaseVolume);
                        ma_uint64 nLenFrames = 0;
                        ma_result nLenRes = ma_sound_get_length_in_pcm_frames(nextSlot, &nLenFrames);

                        if (m_currentMusic != nullptr) {
                            ma_sound_set_fade_in_milliseconds(m_currentMusic, m_musicBaseVolume, 0.0f, m_pendingFadeMs);
                        }
                        m_crossfade.active = true;
                        m_crossfade.in = nextSlot;
                        m_crossfade.out = m_currentMusic;
                        m_crossfade.outInit = m_currentMusic ? (m_currentMusic == &m_musicSoundA ? &m_musicSlotAInit : &m_musicSlotBInit) : nullptr;
                        m_crossfade.durationMs = m_pendingFadeMs;
                        m_crossfade.endTime = now + std::chrono::milliseconds(static_cast<long long>(m_pendingFadeMs));
                        m_currentMusic = nextSlot;
                        m_musicActive = true;
                        m_musicLoop = m_pendingMusicLoop;
                        m_usingA = (nextSlot == &m_musicSoundA);
                        m_hasPendingMusic = false;
                        bool curPlaying = ma_sound_is_playing(m_currentMusic) == MA_TRUE;
                        float curVol = ma_sound_get_volume(m_currentMusic);

                    }
                }
            }
        }

        if (m_musicActive && m_currentMusic != nullptr) {
            ma_uint64 lengthFrames = 0;
            ma_result lenRes = ma_sound_get_length_in_pcm_frames(m_currentMusic, &lengthFrames);
            ma_bool32 atEnd = ma_sound_at_end(m_currentMusic);
            ma_bool32 playing = ma_sound_is_playing(m_currentMusic);
            if (lenRes == MA_SUCCESS && lengthFrames > 0 &&
                playing == MA_FALSE && atEnd == MA_TRUE) {
                m_musicActive = false;

            } else if (playing == MA_FALSE) {
                float vol = ma_sound_get_volume(m_currentMusic);

            }
        }

        if (m_playlistActive && !m_playlist.empty() && m_musicActive == false) {
            m_playlistIndex = (m_playlistIndex + 1) % m_playlist.size();
            m_playlistWaiting = true;
            m_currentTrackDelayMs = m_playlist[m_playlistIndex == 0 ? m_playlist.size() - 1 : m_playlistIndex - 1].delayAfterMs;
            m_playlistNextStart = std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<long long>(m_currentTrackDelayMs));
        }
        if (m_playlistActive && m_playlistWaiting) {
            if (std::chrono::steady_clock::now() >= m_playlistNextStart) {
                m_playlistWaiting = false;
                const auto& entry = m_playlist[m_playlistIndex];
                playMusic(entry.path, entry.loop, m_playlistFadeMs);
            }
        }
    }


} // Audio
