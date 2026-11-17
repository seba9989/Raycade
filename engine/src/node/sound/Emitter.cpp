/**
 * @file Emitter.cpp
 * @brief node::sound::Emitter - odtwarzanie i dźwięk przestrzenny (tłumienie, panorama).
 */

#include <raycade/node/sound/Emitter.hpp>

#include <raycade/scene/Scene.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

#include "../../audio/AudioMixer.hpp"

namespace node::sound {

Emitter::~Emitter() {
    AudioMixer* mixer = AudioMixer::Current();
    if (!mixer) return;
    for (const audio::Voice& voice : voices_)
        if (auto* slot = mixer->Find(voice)) {
            slot->onFinish = nullptr; // zdarzenie wskazywałoby na usunięty emiter
            mixer->Stop(*slot);
        }
}

bool Emitter::IsPlaying() const {
    return requests_ > 0 || std::ranges::any_of(voices_, [](const audio::Voice& voice) {
               return voice.IsPlaying();
           });
}

void Emitter::PlayVoice() {
    bool resumed = false;
    for (audio::Voice& voice : voices_)
        if (voice.IsPaused()) {
            voice.Play();
            resumed = true;
        }
    if (!resumed) ++requests_;
}

void Emitter::StartRequested() {
    AudioMixer* mixer = AudioMixer::Current();
    const int requests = std::exchange(requests_, 0);
    if (!mixer || sound_.empty()) return;
    for (int i = 0; i < requests; ++i) {
        audio::Voice voice = mixer->Sound(sound_).Play();
        if (voice == audio::Voice{}) continue; // brak urządzenia albo MinInterval()
        voice.OnFinish([this](audio::Voice&) {
            if (finish_) finish_();
        });
        voices_.push_back(voice);
    }
}

void Emitter::PauseVoices() {
    for (audio::Voice& voice : voices_)
        voice.Pause();
}

void Emitter::StopVoices() {
    requests_ = 0;
    for (audio::Voice& voice : voices_)
        voice.Stop();
}

void Emitter::Tick(float, const Input&) {
    AudioMixer* mixer = AudioMixer::Current();
    std::erase_if(voices_, [mixer](const audio::Voice& voice) {
        return !mixer || !mixer->Find(voice);
    });
    StartRequested();
    Spatialize(); // przed końcem klatki - nowe głosy ruszają już z właściwą głośnością
}

/**
 * @details Odległość i przesunięcie są liczone w świecie, od słuchacza (node::sound::Listener tej
 * sceny albo środek kamery). Panorama to przesunięcie w poziomie podzielone przez
 * MaxDistance(), a głośność - krzywa Attenuation() z odległości podzielonej przez
 * MaxDistance().
 */
void Emitter::Spatialize() {
    AudioMixer* mixer = AudioMixer::Current();
    const Scene* scene = OwnerScene();
    if (!mixer || !scene || voices_.empty()) return;

    float gain = 1.0f;
    float pan = 0.0f;
    if (!IsScreenSpace()) {
        const Vector2 ear = mixer->ListenerIn(scene).value_or(scene->Camera().Target());
        const Vector2 here = WorldPosition();
        const float dx = here.x - ear.x;
        const float dy = here.y - ear.y;
        const float t = std::min(std::sqrt(dx * dx + dy * dy) / maxDistance_, 1.0f);
        switch (falloff_) {
        case Falloff::None:
            break;
        case Falloff::Linear:
            gain = 1.0f - t;
            break;
        case Falloff::Smooth:
            gain = (1.0f - t) * (1.0f - t);
            break;
        }
        pan = std::clamp(dx / maxDistance_, -1.0f, 1.0f);
    }
    for (const audio::Voice& voice : voices_)
        if (auto* slot = mixer->Find(voice)) {
            slot->spatialGain = gain;
            slot->spatialPan = pan;
        }
}

} // namespace node::sound
