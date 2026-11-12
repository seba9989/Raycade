/**
 * @file Voice.cpp
 * @brief Polecenia uchwytu Voice - odnajdują głos w mikserze silnika.
 */

#include <raycade/audio/mixer/Voice.hpp>

#include <algorithm>
#include <utility>

#include "AudioMixer.hpp"

namespace {

/** @brief Głos uchwytu albo nullptr (martwy uchwyt albo brak miksera). */
AudioMixer::Slot* SlotOf(const Voice& voice) {
    AudioMixer* mixer = AudioMixer::Current();
    return mixer ? mixer->Find(voice) : nullptr;
}

} // namespace

Voice& Voice::Volume(float volume) {
    if (auto* slot = SlotOf(*this)) slot->volume = std::max(0.0f, volume);
    return *this;
}

float Voice::Volume() const {
    const auto* slot = SlotOf(*this);
    return slot ? slot->volume : 1.0f;
}

Voice& Voice::Pitch(float pitch) {
    if (auto* slot = SlotOf(*this)) slot->pitch = std::max(0.01f, pitch);
    return *this;
}

float Voice::Pitch() const {
    const auto* slot = SlotOf(*this);
    return slot ? slot->pitch : 1.0f;
}

Voice& Voice::Pan(float pan) {
    if (auto* slot = SlotOf(*this)) slot->pan = std::clamp(pan, -1.0f, 1.0f);
    return *this;
}

float Voice::Pan() const {
    const auto* slot = SlotOf(*this);
    return slot ? slot->pan : 0.0f;
}

Voice& Voice::Stop() {
    if (auto* slot = SlotOf(*this)) AudioMixer::Current()->Stop(*slot);
    return *this;
}

Voice& Voice::Play() {
    if (auto* slot = SlotOf(*this)) slot->userPaused = false;
    return *this;
}

Voice& Voice::Pause() {
    if (auto* slot = SlotOf(*this)) slot->userPaused = true;
    return *this;
}

bool Voice::IsPaused() const {
    const auto* slot = SlotOf(*this);
    return slot && (slot->paused || slot->userPaused);
}

Voice& Voice::PlaybackTime(float seconds) {
    if (auto* slot = SlotOf(*this)) AudioMixer::Current()->Seek(*slot, seconds);
    return *this;
}

float Voice::PlaybackTime() const {
    const auto* slot = SlotOf(*this);
    return slot ? AudioMixer::Current()->PlaybackTime(*slot) : 0.0f;
}

float Voice::Duration() const {
    const auto* slot = SlotOf(*this);
    return slot ? AudioMixer::Current()->Duration(*slot) : 0.0f;
}

Voice& Voice::OnFinish(std::function<void(Voice&)> callback) {
    if (auto* slot = SlotOf(*this)) slot->onFinish = std::move(callback);
    return *this;
}

bool Voice::IsPlaying() const {
    const auto* slot = SlotOf(*this);
    return slot && !slot->paused && !slot->userPaused && !slot->stopping;
}

const Audio* Voice::Sound() const {
    const auto* slot = SlotOf(*this);
    return slot ? slot->audio : nullptr;
}
