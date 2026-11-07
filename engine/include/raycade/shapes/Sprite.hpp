/**
 * @file Sprite.hpp
 * @brief Kształt Sprite - bitmapa z animacją klatkową z arkusza (sprite sheet).
 */
#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <raycade/entity/Entity.hpp>
#include <raycade/render/Bitmap.hpp>

namespace shapes {

/**
 * @brief Obraz z arkusza klatek z animacją poklatkową.
 * @details Arkusz (sprite sheet) to jedna bitmapa z klatkami jednakowego rozmiaru ułożonymi
 * w wierszach - od lewej do prawej, potem w kolejnym wierszu. Klatki numeruje się od 0.
 *
 * Animacja to nazwany zakres klatek z szybkością (klatki na sekundę) i zapętleniem. Bez
 * zdefiniowanych animacji działa animacja domyślna (nazwa "") obejmująca cały arkusz, z
 * szybkością Fps(). Czas animacji płynie w Tick() z dt sceny, więc zgadza się z resztą gry
 * niezależnie od liczby klatek na sekundę okna.
 *
 * Arkusz jest współdzielony (std::shared_ptr), więc wiele obiektów może używać jednej bitmapy
 * w pamięci i jednej tekstury na karcie graficznej:
 * @code{.cpp}
 * std::shared_ptr<const Bitmap> coin = Shared(Bitmap::Load(engine.AssetPath("coin.png")));
 * for (int i = 0; i < 10; ++i)
 *     scene.Spawn<shapes::Sprite>().Sheet(coin, {16, 16}).Fps(8).Play().Position({i * 40.0f, 0});
 * @endcode
 *
 * Pozycja oznacza punkt Origin() klatki (domyślnie lewy górny róg) - wokół niego działa
 * Rotate() i Scale(). Paint() mnoży kolory klatki (domyślnie WHITE - bez zmian), a Fade()
 * koloru daje półprzezroczystość.
 * @code{.cpp}
 * scene.Spawn<shapes::Sprite>()
 *     .Sheet(engine.AssetPath("hero.png"), {32, 32}) // klatki 32x32
 *     .Animation("stoi", 0, 4, 6)            // klatki 0-3, 6 FPS, w pętli
 *     .Animation("biegnie", 4, 6, 12)        // klatki 4-9, 12 FPS
 *     .Animation("skok", 10, 3, 10, false)   // klatki 10-12, raz
 *     .Origin({16, 32})                      // pozycja = środek stóp
 *     .Position({400, 300})
 *     .Play("stoi")
 *     .OnFinish([](shapes::Sprite& self, const std::string&) { self.Play("stoi"); })
 *     .OnUpdate([](shapes::Sprite& self, float) {
 *         self.Play(IsKeyDown(KEY_D) ? "biegnie" : "stoi");
 *     });
 * @endcode
 */
class Sprite : public Entity {
  public:
    /** @brief Pusty sprite z kolorem mnożącym WHITE (klatki bez zmian). */
    Sprite() { Paint(WHITE); }

    /**
     * @brief Ustawia arkusz klatek.
     * @param sheet Bitmapa (współdzielona); nullptr - nic nie jest rysowane.
     * @param frameSize Rozmiar jednej klatki w pikselach; {0, 0} - cała bitmapa to jedna
     * klatka.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& Sheet(this Self& self, std::shared_ptr<const Bitmap> sheet, Vector2 frameSize = {0, 0}) {
        self.sheet_ = std::move(sheet);
        self.frameSize_ = frameSize;
        self.ClampFrame();
        return self;
    }

    /**
     * @brief Wczytuje arkusz z pliku graficznego (skrót dla Bitmap::Load()).
     * @details Gdy pliku nie da się wczytać, raylib zapisuje przyczynę w logu, a sprite nie ma
     * arkusza (IsLoaded() == false)
     * i niczego nie rysuje. Kilka sprite'ów z tym samym obrazem lepiej utworzyć z jednej
     * bitmapy przez Sheet(std::shared_ptr<const Bitmap>, ...).
     * @param path Ścieżka pliku.
     * @param frameSize Rozmiar jednej klatki; {0, 0} - cały obraz.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& Sheet(this Self& self, const std::string& path, Vector2 frameSize = {0, 0}) {
        return self.Sheet(Shared(Bitmap::Load(path)), frameSize);
    }

    /** @brief Podaje arkusz klatek (albo nullptr). */
    const std::shared_ptr<const Bitmap>& Sheet() const { return sheet_; }

    /** @brief Sprawdza, czy sprite ma niepusty arkusz. */
    bool IsLoaded() const { return sheet_ && !sheet_->IsEmpty(); }

    /** @brief Podaje rozmiar klatki w pikselach (cała bitmapa, gdy nie ustawiono). */
    Vector2 FrameSize() const;

    /** @brief Podaje liczbę klatek w arkuszu. */
    int SheetFrames() const;

    /**
     * @brief Definiuje nazwaną animację (albo zmienia istniejącą).
     * @param name Nazwa, np. "biegnie".
     * @param first Numer pierwszej klatki w arkuszu.
     * @param count Liczba kolejnych klatek.
     * @param fps Szybkość w klatkach na sekundę.
     * @param loop true - w pętli, false - raz, potem zatrzymanie na ostatniej klatce i OnFinish.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& Animation(
        this Self& self,
        const std::string& name,
        int first,
        int count,
        float fps,
        bool loop = true
    ) {
        self.animations_[name] = {first, count, fps, loop};
        return self;
    }

    /** @brief Podaje nazwę bieżącej animacji ("" - cały arkusz). */
    const std::string& Animation() const { return current_; }

    /**
     * @brief Ustawia szybkość animacji domyślnej (całego arkusza).
     * @param fps Klatki na sekundę (domyślnie 10).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Fps(this Self& self, float fps) {
        self.fps_ = fps;
        return self;
    }

    /** @brief Podaje szybkość animacji domyślnej. */
    float Fps() const { return fps_; }

    /**
     * @brief Odtwarza animację o podanej nazwie.
     * @details Jeśli ta animacja już trwa, nic się nie dzieje - dzięki temu Play() można
     * wołać co klatkę (np. w OnUpdate() zależnie od klawiszy). Inna animacja zaczyna się od
     * pierwszej klatki. Nieznana nazwa jest pomijana.
     * @param name Nazwa animacji (z Animation()) albo "" - cały arkusz.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Play(this Self& self, const std::string& name) {
        self.Start(name);
        return self;
    }

    /**
     * @brief Wznawia bieżącą animację (po Pause() albo Stop()).
     * @details Animacja jednorazowa zakończona na ostatniej klatce zaczyna się od początku.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Play(this Self& self) {
        self.Resume();
        return self;
    }

    /**
     * @brief Zatrzymuje animację na bieżącej klatce.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Pause(this Self& self) {
        self.playing_ = false;
        return self;
    }

    /**
     * @brief Zatrzymuje animację i wraca do jej pierwszej klatki.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Stop(this Self& self) {
        self.playing_ = false;
        self.frame_ = 0;
        self.time_ = 0.0f;
        return self;
    }

    /** @brief Sprawdza, czy animacja jest odtwarzana. */
    bool IsPlaying() const { return playing_; }

    /**
     * @brief Ustawia klatkę bieżącej animacji.
     * @param frame Numer klatki w animacji (od 0); spoza zakresu jest przycinany.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Frame(this Self& self, int frame) {
        self.frame_ = frame;
        self.time_ = 0.0f;
        self.ClampFrame();
        return self;
    }

    /** @brief Podaje numer klatki w bieżącej animacji (od 0). */
    int Frame() const { return frame_; }

    /** @brief Podaje numer wyświetlanej klatki w całym arkuszu. */
    int SheetFrame() const;

    /**
     * @brief Mnoży szybkość wszystkich animacji (np. 0.5 - zwolnienie, 2 - przyspieszenie).
     * @param multiplier Mnożnik (domyślnie 1).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Speed(this Self& self, float multiplier) {
        self.speed_ = multiplier;
        return self;
    }

    /** @brief Podaje mnożnik szybkości. */
    float Speed() const { return speed_; }

    /**
     * @brief Odbija klatkę w poziomie (np. postać idąca w lewo).
     * @details Odbicie dotyczy samego obrazu - Origin() i obszar trafienia zostają.
     * @param value Czy odbić.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& FlipX(this Self& self, bool value = true) {
        self.flipX_ = value;
        return self;
    }

    /**
     * @brief Odbija klatkę w pionie.
     * @param value Czy odbić.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& FlipY(this Self& self, bool value = true) {
        self.flipY_ = value;
        return self;
    }

    /** @brief Sprawdza, czy klatka jest odbita w poziomie. */
    bool IsFlipX() const { return flipX_; }

    /** @brief Sprawdza, czy klatka jest odbita w pionie. */
    bool IsFlipY() const { return flipY_; }

    /**
     * @brief Ustawia punkt zaczepienia w klatce (piksele od jej lewego górnego rogu).
     * @details Pozycja obiektu wskazuje ten punkt, a obrót i skala działają wokół niego,
     * np. {16, 16} dla środka klatki 32x32.
     * @param origin Punkt w pikselach klatki.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Origin(this Self& self, Vector2 origin) {
        self.origin_ = origin;
        return self;
    }

    /** @brief Podaje punkt zaczepienia w klatce. */
    Vector2 Origin() const { return origin_; }

    /**
     * @brief Ustawia zdarzenie końca animacji jednorazowej (loop = false).
     * @param callback Funkcja otrzymująca sprite i nazwę zakończonej animacji.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& OnFinish(this Self& self, Callback<Self, const std::string&> callback) {
        self.finish_ = [&self, fn = std::move(callback)](const std::string& name) {
            fn(self, name);
        };
        return self;
    }

    /** @copydoc Updatable::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @brief Przesuwa animację o czas dt. */
    void Tick(float dt, const Input& input) override;

    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    /** @brief Zakres klatek animacji. */
    struct AnimationClip {
        int first = 0;     /**< Pierwsza klatka w arkuszu. */
        int count = 1;     /**< Liczba klatek. */
        float fps = 10.0f; /**< Klatki na sekundę. */
        bool loop = true;  /**< Czy w pętli. */
    };

    /** @brief Bieżąca animacja (dla "" i nieznanej nazwy - cały arkusz). */
    AnimationClip Current() const;

    /** @brief Przełącza na animację, jeśli to inna niż bieżąca, i ją odtwarza. */
    void Start(const std::string& name);

    /** @brief Wznawia bieżącą animację. */
    void Resume();

    /** @brief Przycina numer klatki do bieżącej animacji. */
    void ClampFrame();

    std::shared_ptr<const Bitmap> sheet_;             /**< Arkusz klatek. */
    Vector2 frameSize_{0, 0};                         /**< Rozmiar klatki; 0 - cały arkusz. */
    std::map<std::string, AnimationClip> animations_; /**< Animacje według nazwy. */
    std::string current_;                             /**< Bieżąca animacja. */
    float fps_ = 10.0f;                               /**< Szybkość animacji domyślnej. */
    float speed_ = 1.0f;                              /**< Mnożnik szybkości. */
    float time_ = 0.0f;                               /**< Czas od zmiany klatki. */
    int frame_ = 0;                                   /**< Klatka w bieżącej animacji. */
    bool playing_ = false;                            /**< Czy animacja trwa. */
    bool flipX_ = false;                              /**< Odbicie w poziomie. */
    bool flipY_ = false;                              /**< Odbicie w pionie. */
    Vector2 origin_{0, 0};                            /**< Punkt zaczepienia w klatce. */
    std::function<void(const std::string&)> finish_;  /**< Koniec animacji jednorazowej. */
};

} // namespace shapes
