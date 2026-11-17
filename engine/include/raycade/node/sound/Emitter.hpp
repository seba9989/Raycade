/**
 * @file Emitter.hpp
 * @brief Klasa node::sound::Emitter - dźwięk w przestrzeni 2D sceny.
 */
#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <raycade/audio/playback/Voice.hpp>
#include <raycade/entity/Entity.hpp>

namespace node::sound {

/** @brief Krzywa tłumienia dźwięku z odległością (Emitter::Attenuation()). */
enum class Falloff {
    None,   /**< Bez tłumienia - tylko panorama. */
    Linear, /**< Głośność maleje liniowo do zera w MaxDistance(). */
    Smooth, /**< Szybszy spadek blisko źródła, łagodny przy MaxDistance() (kwadrat liniowej). */
};

/**
 * @brief Źródło dźwięku w świecie: im dalej od słuchacza, tym ciszej, a lewo/prawo słychać
 * w panoramie.
 * @details Emiter odtwarza dźwięk z Audio::Sound() (z jego plikami i ustawieniami) i co
 * klatkę liczy dla swoich głosów głośność i panoramę z położenia względem słuchacza.
 * Słuchaczem jest node::sound::Listener w tej samej scenie, a bez niego - środek kamery sceny
 * (SceneCamera::Target()). Emiter w układzie ekranu (ScreenSpace()) gra bez tłumienia.
 * @code{.cpp}
 * engine.Audio().Sound("ogien").Files({"fire.wav"}).Looped().Bus("efekty");
 *
 * scene.Spawn<node::sound::Emitter>()
 *     .Sound("ogien")
 *     .MaxDistance(500)
 *     .Attenuation(node::sound::Falloff::Linear)
 *     .Position({750, 580})
 *     .Play();               // ruszy po dodaniu do sceny
 * @endcode
 *
 * Jednorazowy dźwięk w danym miejscu to emiter, który usuwa się po końcu odtwarzania:
 * @code{.cpp}
 * scene.Spawn<node::sound::Emitter>().Sound("wybuch").Position(where).Play()
 *     .OnFinish([](node::sound::Emitter& self) { self.Destroy(); });
 * @endcode
 *
 * Usunięcie emitera zatrzymuje jego głosy (z wyciszeniem Source::FadeOutTime()). Emiter jest
 * niewidoczny i nie przyjmuje kliknięć.
 */
class Emitter : public Entity {
  public:
    /** @brief Zatrzymuje głosy emitera. */
    ~Emitter() override;

    /**
     * @brief Ustawia dźwięk (nazwę z Audio::Sound()), który odtwarza emiter.
     * @param name Nazwa dźwięku.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Sound(this Self& self, std::string name) {
        self.sound_ = std::move(name);
        return self;
    }

    /** @brief Podaje nazwę dźwięku. */
    const std::string& Sound() const { return sound_; }

    /**
     * @brief Ustawia odległość, w której dźwięk cichnie do zera (i pełne wychylenie panoramy).
     * @param distance Odległość w jednostkach świata (pikselach).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& MaxDistance(this Self& self, float distance) {
        self.maxDistance_ = distance > 1.0f ? distance : 1.0f;
        return self;
    }

    /** @brief Podaje odległość, w której dźwięk cichnie do zera. */
    float MaxDistance() const { return maxDistance_; }

    /**
     * @brief Ustawia krzywą tłumienia z odległością.
     * @param falloff Krzywa (domyślnie Falloff::Linear).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Attenuation(this Self& self, Falloff falloff) {
        self.falloff_ = falloff;
        return self;
    }

    /** @brief Podaje krzywą tłumienia. */
    Falloff Attenuation() const { return falloff_; }

    /**
     * @brief Odtwarza dźwięk emitera albo wznawia głosy wstrzymane przez Pause().
     * @details Głos rusza w najbliższej aktualizacji emitera, gdy znane jest już jego położenie
     * z całego łańcucha (także z Position() wywołanego po Play()) - także gdy Play() wywołano
     * przed dodaniem do sceny. Każde kolejne Play() bez pauzy dodaje nowy głos, jak
     * Sound::Play().
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Play(this Self& self) {
        self.PlayVoice();
        return self;
    }

    /**
     * @brief Wstrzymuje głosy emitera (wznawia je Play()).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Pause(this Self& self) {
        self.PauseVoices();
        return self;
    }

    /**
     * @brief Zatrzymuje głosy emitera (z wyciszeniem Source::FadeOutTime()).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Stop(this Self& self) {
        self.StopVoices();
        return self;
    }

    /** @brief Sprawdza, czy gra choć jeden głos emitera (albo czeka na start po Play()). */
    bool IsPlaying() const;

    /**
     * @brief Ustawia zdarzenie końca odtworzenia (każdego głosu emitera).
     * @param callback Funkcja otrzymująca emiter.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& OnFinish(this Self& self, Callback<Self> callback) {
        self.finish_ = [&self, fn = std::move(callback)] { fn(self); };
        return self;
    }

  protected:
    /** @brief Nic nie rysuje (emiter jest niewidoczny). */
    void Draw(PrimitiveRenderer&) const override {}

    /** @brief Uruchamia głosy z Play() i liczy głośność i panoramę z położenia. */
    void Tick(float dt, const Input& input) override;

  private:
    /** @brief Wznawia wstrzymane głosy albo zgłasza nowy (Play()). */
    void PlayVoice();
    /** @brief Wstrzymuje głosy (Pause()). */
    void PauseVoices();
    /** @brief Zatrzymuje głosy (Stop()). */
    void StopVoices();
    /** @brief Odtwarza głosy zgłoszone przez Play(). */
    void StartRequested();
    /** @brief Ustawia głosom tłumienie i panoramę. */
    void Spatialize();

    std::string sound_;                 /**< Nazwa dźwięku z Audio::Sound(). */
    float maxDistance_ = 600.0f;        /**< Zasięg słyszalności. */
    Falloff falloff_ = Falloff::Linear; /**< Krzywa tłumienia. */
    std::vector<audio::Voice> voices_;  /**< Głosy emitera (martwe usuwa Tick()). */
    int requests_ = 0;                  /**< Play() czekające na Tick(). */
    std::function<void()> finish_;      /**< Zdarzenie końca. */
};

} // namespace node::sound
