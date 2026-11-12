/**
 * @file Sprite.hpp
 * @brief Obiekt Sprite - klatka z arkusza (SpriteSheet) z animacją poklatkową.
 */
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <raycade/entity/Entity.hpp>
#include <raycade/render/Assets.hpp>
#include <raycade/render/Bitmap.hpp>
#include <raycade/render/SpriteSheet.hpp>

namespace visual {

/**
 * @brief Obraz z arkusza klatek z animacją poklatkową.
 * @details Klatki i nazwane animacje opisuje SpriteSheet - współdzielony, więc wielu
 * przeciwników używa jednego opisu, jednej bitmapy w pamięci i jednej tekstury na karcie
 * graficznej. Sprite pamięta tylko własny stan: bieżącą animację, klatkę i czas.
 * @code{.cpp}
 * auto coin = std::make_shared<SpriteSheet>(
 *     SpriteSheet::Grid(engine.Assets().Bitmap("coin.png"), {16, 16}));
 * coin->Clip("kreci", 0, 8, 8);
 * for (int i = 0; i < 10; ++i)
 *     scene.Spawn<visual::Sprite>().Sheet(coin).Play("kreci").Position({i * 40.0f, 0});
 * @endcode
 *
 * Dla pojedynczego obiektu wystarczy skrót: Sheet() z bitmapą albo nazwą pliku tworzy
 * prywatny arkusz z siatki, a Animation() dopisuje do niego animacje. Animation() na arkuszu
 * współdzielonym robi najpierw prywatną kopię opisu (bitmapa zostaje wspólna), więc nie
 * zmienia innych sprite'ów.
 *
 * Bez wybranej animacji działa animacja domyślna (nazwa "") obejmująca cały arkusz,
 * z szybkością Fps(). Czas animacji płynie w Tick() z dt sceny, więc zgadza się z resztą gry
 * niezależnie od liczby klatek na sekundę okna.
 *
 * Pozycja oznacza punkt Origin() klatki (domyślnie lewy górny róg) - wokół niego działa
 * Rotate() i Scale(). Paint() mnoży kolory klatki (domyślnie WHITE - bez zmian), a Fade()
 * koloru daje półprzezroczystość.
 * @code{.cpp}
 * scene.Spawn<visual::Sprite>()
 *     .Sheet("hero.png", {32, 32})           // klatki 32x32, plik z assets/
 *     .Animation("stoi", 0, 4, 6)            // klatki 0-3, 6 FPS, w pętli
 *     .Animation("biegnie", 4, 6, 12)        // klatki 4-9, 12 FPS
 *     .Animation("skok", 10, 3, 10, false)   // klatki 10-12, raz
 *     .Origin({16, 32})                      // pozycja = środek stóp
 *     .Position({400, 300})
 *     .Play("stoi")
 *     .OnFinish([](visual::Sprite& self, const std::string&) { self.Play("stoi"); })
 *     .OnUpdate([](visual::Sprite& self, float) {
 *         self.Play(IsKeyDown(KEY_D) ? "biegnie" : "stoi");
 *     });
 * @endcode
 */
class Sprite : public Entity {
  public:
    /** @brief Pusty sprite z kolorem mnożącym WHITE (klatki bez zmian). */
    Sprite() { Paint(WHITE); }

    /**
     * @brief Ustawia współdzielony arkusz klatek z animacjami.
     * @details Bieżąca animacja zostaje, jeśli nowy arkusz ma animację o tej nazwie;
     * w przeciwnym razie działa animacja domyślna (cały arkusz).
     * @param sheet Arkusz; nullptr - nic nie jest rysowane.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& Sheet(this Self& self, std::shared_ptr<const SpriteSheet> sheet) {
        self.own_.reset();
        self.sheet_ = std::move(sheet);
        self.SheetChanged();
        return self;
    }

    /**
     * @brief Ustawia arkusz z bitmapy podzielonej na siatkę klatek (prywatny dla sprite'a).
     * @details Animacje zdefiniowane wcześniej przez Animation() zostają.
     * @param sheet Bitmapa (współdzielona); nullptr - nic nie jest rysowane.
     * @param frameSize Rozmiar jednej klatki w pikselach; {0, 0} - cała bitmapa to jedna
     * klatka.
     * @param margin Odstęp siatki od lewej i górnej krawędzi bitmapy.
     * @param spacing Odstęp między klatkami.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& Sheet(
        this Self& self,
        std::shared_ptr<const Bitmap> sheet,
        Vector2 frameSize = {0, 0},
        Vector2 margin = {0, 0},
        Vector2 spacing = {0, 0}
    ) {
        self.SetGrid(std::move(sheet), frameSize, margin, spacing);
        return self;
    }

    /**
     * @brief Ustawia arkusz z pliku przez pamięć podręczną zasobów.
     * @details Plik JSON (Aseprite, TexturePacker - patrz SpriteSheet::Load()) daje gotowy
     * arkusz z klatkami i animacjami, a parametry siatki są pomijane. Plik graficzny jest
     * dzielony na siatkę klatek (prywatny arkusz sprite'a).
     *
     * Plik pobiera Assets z kontekstu (Engine::Assets()), więc wiele sprite'ów z tym samym
     * plikiem dzieli jedną bitmapę (i jeden arkusz JSON), a nazwa względna jest liczona od
     * katalogu assets/. Brakujący plik daje szachownicę zastępczą (Assets::Missing()) i wpis
     * w logu. Bez Assets w kontekście plik jest wczytywany bezpośrednio.
     * @code{.cpp}
     * scene.Spawn<visual::Sprite>().Sheet("hero.json").Play("run");
     * scene.Spawn<visual::Sprite>().Sheet("coin.png", {16, 16}).Play();
     * @endcode
     * @param path Nazwa pliku w assets/ (np. "hero.json", "hero.png") albo pełna ścieżka.
     * @param frameSize Rozmiar jednej klatki obrazu; {0, 0} - cały obraz.
     * @param margin Odstęp siatki od lewej i górnej krawędzi bitmapy.
     * @param spacing Odstęp między klatkami.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& Sheet(
        this Self& self,
        const std::string& path,
        Vector2 frameSize = {0, 0},
        Vector2 margin = {0, 0},
        Vector2 spacing = {0, 0}
    ) {
        Assets* assets = self.template Context<Assets>();
        if (IsJsonPath(path)) {
            if (assets) return self.Sheet(assets->Sheet(path));
            return self.Sheet(std::shared_ptr<const SpriteSheet>{Shared(SpriteSheet::Load(path))});
        }
        if (assets) return self.Sheet(assets->Bitmap(path), frameSize, margin, spacing);
        return self.Sheet(Shared(Bitmap::Load(path)), frameSize, margin, spacing);
    }

    /** @brief Podaje arkusz klatek (albo nullptr). */
    const std::shared_ptr<const SpriteSheet>& Sheet() const { return sheet_; }

    /**
     * @brief Sprawdza, czy sprite ma arkusz z wczytaną bitmapą i klatkami.
     * @details Dla szachownicy zastępczej (brakujący plik) zwraca false, choć sprite ją rysuje.
     */
    bool IsLoaded() const { return sheet_ && sheet_->IsLoaded(); }

    /** @brief Podaje rozmiar wyświetlanej klatki w pikselach. */
    Vector2 FrameSize() const;

    /** @brief Podaje liczbę klatek w arkuszu. */
    int SheetFrames() const { return sheet_ ? sheet_->FrameCount() : 0; }

    /**
     * @brief Definiuje nazwaną animację z kolejnych klatek (albo zmienia istniejącą).
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
        self.MutableSheet().Clip(name, first, count, fps, loop ? Playback::Loop : Playback::Once);
        self.ClipsChanged();
        return self;
    }

    /**
     * @brief Definiuje nazwaną animację z listy klatek (albo zmienia istniejącą).
     * @param name Nazwa.
     * @param frames Klatki arkusza w kolejności odtwarzania, np. {0, 1, 2, 1}.
     * @param fps Szybkość w klatkach na sekundę.
     * @param mode Sposób odtwarzania (pętla, raz, tam i z powrotem).
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& Animation(
        this Self& self,
        const std::string& name,
        std::vector<int> frames,
        float fps,
        Playback mode = Playback::Loop
    ) {
        self.MutableSheet().Clip(name, std::move(frames), fps, mode);
        self.ClipsChanged();
        return self;
    }

    /**
     * @brief Definiuje nazwaną animację z pełnego opisu (czasy klatek, zdarzenia).
     * @param name Nazwa.
     * @param clip Opis animacji.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& Animation(this Self& self, const std::string& name, SpriteClip clip) {
        self.MutableSheet().Clip(name, std::move(clip));
        self.ClipsChanged();
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
     * pierwszej klatki. Nieznana nazwa jest pomijana, a ostrzeżenie trafia do logu (raz na
     * nazwę, żeby wywołanie co klatkę nie zalało logu).
     * @param name Nazwa animacji (z arkusza albo Animation()) albo "" - cały arkusz.
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
        self.Rewind(0);
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
        self.Rewind(frame);
        return self;
    }

    /** @brief Podaje numer klatki w bieżącej animacji (od 0). */
    int Frame() const { return frame_; }

    /** @brief Podaje numer wyświetlanej klatki w całym arkuszu. */
    int SheetFrame() const;

    /**
     * @brief Mnoży szybkość wszystkich animacji (np. 0.5 - zwolnienie, 2 - przyspieszenie).
     * @param multiplier Mnożnik (domyślnie 1); 0 albo mniej zatrzymuje upływ czasu.
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
     * @brief Ustawia zdarzenie końca animacji jednorazowej (Playback::Once).
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

    /**
     * @brief Ustawia zdarzenie klatki (SpriteSheet::Event()), np. dźwięk kroku.
     * @details Zachodzi, gdy animacja przejdzie do klatki ze zdarzeniem - także przy
     * zapętleniu na klatkę 0, ale nie przy samym Play().
     * @param callback Funkcja otrzymująca sprite i nazwę zdarzenia.
     * @return Ten sam obiekt.
     */
    template <typename Self>
    Self& OnFrameEvent(this Self& self, Callback<Self, const std::string&> callback) {
        self.frameEvent_ = [&self, fn = std::move(callback)](const std::string& event) {
            fn(self, event);
        };
        return self;
    }

    /** @copydoc Entity::Contains */
    bool Contains(Vector2 point) const override;

  protected:
    /** @brief Przesuwa animację o czas dt. */
    void Tick(float dt, const Input& input) override;

    /** @copydoc Drawable::Draw */
    void Draw(PrimitiveRenderer& renderer) const override;

  private:
    /** @brief Sprawdza, czy jest co rysować (także szachownicę zastępczą). */
    bool HasFrames() const;

    /** @brief Sprawdza, czy ścieżka to plik JSON (rozszerzenie bez względu na wielkość liter). */
    static bool IsJsonPath(const std::string& path);

    /** @brief Bieżąca animacja (dla "" i nazwy spoza arkusza - cały arkusz z Fps()). */
    const SpriteClip& ActiveClip() const;

    /** @brief Ustawia prywatny arkusz z siatki, zachowując animacje z Animation(). */
    void SetGrid(
        std::shared_ptr<const Bitmap> bitmap,
        Vector2 frame,
        Vector2 margin,
        Vector2 spacing
    );

    /** @brief Arkusz do zmiany: prywatny, w razie potrzeby kopia współdzielonego. */
    SpriteSheet& MutableSheet();

    /** @brief Odświeża wskaźnik bieżącej animacji po zmianie arkusza. */
    void SheetChanged();

    /** @brief Odświeża wskaźnik bieżącej animacji po zmianie animacji arkusza. */
    void ClipsChanged();

    /** @brief Przełącza na animację, jeśli to inna niż bieżąca, i ją odtwarza. */
    void Start(const std::string& name);

    /** @brief Wznawia bieżącą animację. */
    void Resume();

    /** @brief Ustawia klatkę animacji (przyciętą) i zeruje jej czas. */
    void Rewind(int frame);

    /**
     * @brief Przechodzi do następnej klatki zgodnie z Playback.
     * @return false, gdy animacja jednorazowa się skończyła.
     */
    bool Advance(const SpriteClip& clip);

    std::shared_ptr<const SpriteSheet> sheet_; /**< Arkusz (może być współdzielony). */
    std::shared_ptr<SpriteSheet> own_;         /**< Arkusz prywatny (== sheet_) albo nullptr. */
    const SpriteClip* clip_ = nullptr;         /**< Bieżąca animacja w sheet_ albo nullptr. */
    mutable SpriteClip defaultClip_;           /**< Animacja domyślna (cały arkusz). */
    std::string current_;                      /**< Nazwa bieżącej animacji. */
    std::uint32_t generation_ = 0;             /**< Licznik zmian animacji (dla callbacków). */
    float fps_ = 10.0f;                        /**< Szybkość animacji domyślnej. */
    float speed_ = 1.0f;                       /**< Mnożnik szybkości. */
    float time_ = 0.0f;                        /**< Czas od zmiany klatki. */
    int frame_ = 0;                            /**< Klatka w bieżącej animacji. */
    int direction_ = 1;                        /**< Kierunek dla Playback::PingPong. */
    bool playing_ = false;                     /**< Czy animacja trwa. */
    bool flipX_ = false;                       /**< Odbicie w poziomie. */
    bool flipY_ = false;                       /**< Odbicie w pionie. */
    Vector2 origin_{0, 0};                     /**< Punkt zaczepienia w klatce. */
    std::function<void(const std::string&)> finish_;     /**< Koniec animacji jednorazowej. */
    std::function<void(const std::string&)> frameEvent_; /**< Zdarzenie klatki. */
    std::string missing_; /**< Ostatnia nieznana nazwa z Play() (ostrzeżenie tylko raz). */
};

} // namespace visual
