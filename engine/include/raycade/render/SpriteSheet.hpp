/**
 * @file SpriteSheet.hpp
 * @brief Klasa SpriteSheet - arkusz klatek z nazwanymi animacjami, współdzielony przez sprite'y.
 */
#pragma once

#include <raylib.h>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <raycade/render/Bitmap.hpp>

/** @brief Sposób odtwarzania animacji. */
enum class Playback {
    Loop,     /**< W pętli: po ostatniej klatce pierwsza. */
    Once,     /**< Raz: zatrzymanie na ostatniej klatce i zdarzenie OnFinish. */
    PingPong, /**< Tam i z powrotem: 0, 1, 2, 1, 0, 1... */
};

/**
 * @brief Animacja: ciąg klatek arkusza z szybkością, czasami klatek i zdarzeniami.
 * @details Klatki podaje zakres (first, count) albo - gdy lista nie jest pusta - frames,
 * np. {0, 1, 2, 1} albo klatki z różnych wierszy. Indeksy w durations i events to numery
 * klatek w animacji (od 0), a nie w arkuszu.
 */
struct SpriteClip {
    int first = 0;                                   /**< Pierwsza klatka zakresu. */
    int count = 1;                                   /**< Liczba klatek zakresu. */
    std::vector<int> frames;                         /**< Klatki arkusza (zamiast zakresu). */
    float fps = 10.0f;                               /**< Klatki na sekundę. */
    std::vector<float> durations;                    /**< Czas klatek w sekundach (wybrane). */
    Playback mode = Playback::Loop;                  /**< Sposób odtwarzania. */
    std::vector<std::pair<int, std::string>> events; /**< Zdarzenia: (klatka, nazwa). */

    /** @brief Podaje liczbę klatek animacji (co najmniej 1). */
    int Length() const;

    /**
     * @brief Podaje numer klatki arkusza dla klatki animacji.
     * @param index Klatka animacji; spoza zakresu jest przycinana.
     * @return Numer klatki w arkuszu.
     */
    int SheetFrame(int index) const;

    /**
     * @brief Podaje czas wyświetlania klatki animacji.
     * @param index Klatka animacji.
     * @return durations[index], jeśli jest dodatni; inaczej 1 / fps; 0 - animacja stoi.
     */
    float Duration(int index) const;
};

/**
 * @brief Klatka arkusza: prostokąt w bitmapie i jego miejsce w pełnej klatce.
 * @details Atlas z Aseprite albo TexturePackera może zapisywać klatki przycięte do
 * nieprzezroczystych pikseli. Wtedy area jest mniejszy od klatki, offset mówi, gdzie w pełnej
 * klatce leży jego lewy górny róg, a size podaje rozmiar pełnej klatki - dzięki temu Origin()
 * i obszar trafienia sprite'a nie skaczą między klatkami.
 */
struct SpriteFrame {
    Rectangle area{0, 0, 0, 0}; /**< Prostokąt w pikselach bitmapy. */
    Vector2 offset{0, 0};       /**< Położenie area w pełnej klatce. */
    Vector2 size{0, 0};         /**< Rozmiar pełnej klatki; {0, 0} - rozmiar area. */
    float duration = 0.0f;      /**< Czas klatki z pliku w sekundach; 0 - brak. */
    std::string name;           /**< Nazwa klatki z pliku (np. "run_01.png"); może być pusta. */

    /** @brief Podaje rozmiar pełnej klatki (size albo rozmiar area). */
    Vector2 FullSize() const {
        return size.x > 0 && size.y > 0 ? size : Vector2{area.width, area.height};
    }
};

/**
 * @brief Arkusz klatek (bitmapa + prostokąty klatek) z nazwanymi animacjami.
 * @details Opis animacji należy do arkusza, a nie do sprite'a, więc dziesięciu przeciwników
 * dzieli jeden obiekt - definiuje się go raz, a każdy visual::Sprite pamięta tylko własny
 * stan odtwarzania (animacja, klatka, czas). Arkusz przekazuje się jako
 * `std::shared_ptr<const SpriteSheet>`.
 *
 * Klatki to dowolne prostokąty bitmapy. Najczęstszy przypadek - równa siatka, także
 * z marginesem wokół i odstępami między klatkami - tworzy Grid(); pojedyncze prostokąty
 * (np. z atlasu) dopisuje Frame(). Klatki numeruje się od 0, w siatce wierszami.
 * @code{.cpp}
 * auto hero = std::make_shared<SpriteSheet>(
 *     SpriteSheet::Grid(engine.Assets().Bitmap("hero.png"), {32, 32}, {1, 1}, {2, 2}));
 * hero->Clip("stoi", 0, 4, 6)
 *     .Clip("biegnie", 4, 6, 12)
 *     .Clip("skok", 10, 3, 10, Playback::Once)
 *     .Clip("oddech", {0, 1, 2, 1}, 4)
 *     .Durations("skok", {0.05f, 0.3f, 0.1f})  // zawieszenie w powietrzu
 *     .Event("biegnie", 2, "krok")
 *     .Event("biegnie", 5, "krok");
 * for (int i = 0; i < 10; ++i)
 *     scene.Spawn<visual::Sprite>().Sheet(hero).Play("biegnie").Position({i * 40.0f, 0});
 * @endcode
 *
 * Arkusz z programu graficznego wczytuje Load() z pliku JSON - najprościej przez pamięć
 * podręczną zasobów (Assets::Sheet()):
 * @code{.cpp}
 * auto hero = engine.Assets().Sheet("hero.json"); // obraz, klatki i animacje z tagów
 * scene.Spawn<visual::Sprite>().Sheet(hero).Play("run");
 * @endcode
 */
class SpriteSheet {
  public:
    /** @brief Pusty arkusz (bez bitmapy i klatek). */
    SpriteSheet() = default;

    /**
     * @brief Arkusz z jedną klatką obejmującą całą bitmapę.
     * @param bitmap Bitmapa (współdzielona); nullptr - arkusz pusty.
     */
    explicit SpriteSheet(std::shared_ptr<const ::Bitmap> bitmap);

    /**
     * @brief Tworzy arkusz z klatek jednakowego rozmiaru ułożonych w siatce.
     * @details Klatki leżą od lewej do prawej, potem w kolejnym wierszu. Liczba kolumn
     * i wierszy wynika z rozmiaru bitmapy - klatka, która nie mieści się w całości, jest
     * pomijana.
     * @param bitmap Bitmapa (współdzielona).
     * @param frameSize Rozmiar klatki; {0, 0} - cała bitmapa to jedna klatka; wymiar większy
     * od bitmapy jest do niej przycinany.
     * @param margin Odstęp siatki od krawędzi bitmapy (lewej i górnej).
     * @param spacing Odstęp między sąsiednimi klatkami.
     * @param count Najwyżej tyle klatek (0 - wszystkie), gdy ostatni wiersz jest niepełny.
     * @return Arkusz.
     */
    static SpriteSheet Grid(
        std::shared_ptr<const ::Bitmap> bitmap,
        Vector2 frameSize,
        Vector2 margin = {0, 0},
        Vector2 spacing = {0, 0},
        int count = 0
    );

    /** @brief Funkcja dostarczająca bitmapę dla ścieżki obrazu (nullptr - błąd). */
    using BitmapLoader = std::function<std::shared_ptr<const ::Bitmap>(const std::string& path)>;

    /**
     * @brief Wczytuje arkusz z pliku JSON wyeksportowanego z Aseprite albo TexturePackera.
     * @details Obsługiwany jest układ `{"frames": ..., "meta": {...}}`:
     * - frames jako tablica (Aseprite "Array", TexturePacker "JSON Array") albo obiekt
     *   (Aseprite "Hash", TexturePacker "JSON Hash") - kolejność klatek jak w pliku,
     * - frame, spriteSourceSize i sourceSize - także klatki przycięte (trimmed),
     * - duration klatki w milisekundach (Aseprite) - czasy klatek animacji,
     * - meta.image - obraz arkusza, ścieżka względem pliku JSON,
     * - meta.frameTags (Aseprite) - animacje: from, to, direction (forward, reverse,
     *   pingpong, pingpong_reverse) i repeat (odtworzenie N razy, potem Playback::Once).
     *
     * Bez frameTags animacje powstają z nazw klatek: "run_01.png", "run_02.png" dają animację
     * "run" (końcowe cyfry i rozszerzenie są odcinane), a pojedyncza klatka "coin.png" -
     * animację "coin". Klatki obrócone w atlasie (TexturePacker "rotated") nie są obsługiwane -
     * wczytanie zapisuje ostrzeżenie; trzeba wyłączyć obracanie przy eksporcie.
     * @code{.cpp}
     * // aseprite -b hero.aseprite --sheet hero.png --data hero.json --format json-array
     * //          --list-tags
     * auto hero = Shared(SpriteSheet::Load(engine.AssetPath("hero.json")));
     * @endcode
     * @param path Ścieżka pliku JSON.
     * @param loadBitmap Skąd wziąć obraz (np. z Assets); domyślnie Bitmap::Load().
     * @return Arkusz albo brak wartości, gdy pliku albo obrazu nie da się wczytać (przyczyna
     * w logu).
     */
    static std::optional<SpriteSheet> Load(
        const std::string& path,
        const BitmapLoader& loadBitmap = {}
    );

    /**
     * @brief Ustawia bitmapę arkusza (klatki zostają).
     * @param bitmap Bitmapa (współdzielona).
     * @return Ten sam arkusz.
     */
    SpriteSheet& Bitmap(std::shared_ptr<const ::Bitmap> bitmap) {
        bitmap_ = std::move(bitmap);
        return *this;
    }

    /** @brief Podaje bitmapę arkusza (albo nullptr). */
    const std::shared_ptr<const ::Bitmap>& Bitmap() const { return bitmap_; }

    /**
     * @brief Dopisuje klatkę o podanym prostokącie (np. z atlasu).
     * @param area Prostokąt w pikselach bitmapy.
     * @return Ten sam arkusz.
     */
    SpriteSheet& Frame(Rectangle area) {
        SpriteFrame frame;
        frame.area = area;
        frames_.push_back(std::move(frame));
        return *this;
    }

    /**
     * @brief Dopisuje klatkę z pełnym opisem (przycięcie, czas, nazwa).
     * @param frame Klatka.
     * @return Ten sam arkusz.
     */
    SpriteSheet& Frame(SpriteFrame frame) {
        frames_.push_back(std::move(frame));
        return *this;
    }

    /**
     * @brief Podaje prostokąt klatki.
     * @param index Numer klatki; spoza zakresu jest przycinany.
     * @return Prostokąt w pikselach bitmapy; {0, 0, 0, 0} dla arkusza bez klatek.
     */
    Rectangle Frame(int index) const { return FrameInfo(index).area; }

    /**
     * @brief Podaje pełny opis klatki (przycięcie, czas, nazwa).
     * @param index Numer klatki; spoza zakresu jest przycinany.
     * @return Klatka; pusta dla arkusza bez klatek.
     */
    const SpriteFrame& FrameInfo(int index) const;

    /**
     * @brief Szuka klatki po nazwie z pliku.
     * @param name Nazwa, np. "coin.png".
     * @return Numer klatki albo -1.
     */
    int FindFrame(const std::string& name) const;

    /** @brief Podaje liczbę klatek. */
    int FrameCount() const { return static_cast<int>(frames_.size()); }

    /**
     * @brief Sprawdza, czy arkusz ma wczytaną bitmapę (Bitmap::IsLoaded() - nie zastępczą)
     * i co najmniej jedną klatkę.
     */
    bool IsLoaded() const { return bitmap_ && bitmap_->IsLoaded() && !frames_.empty(); }

    /**
     * @brief Definiuje animację (albo zastępuje istniejącą o tej nazwie).
     * @param name Nazwa, np. "biegnie".
     * @param clip Opis animacji.
     * @return Ten sam arkusz.
     */
    SpriteSheet& Clip(const std::string& name, SpriteClip clip) {
        clips_[name] = std::move(clip);
        return *this;
    }

    /**
     * @brief Definiuje animację z kolejnych klatek arkusza.
     * @param name Nazwa.
     * @param first Pierwsza klatka.
     * @param count Liczba klatek.
     * @param fps Klatki na sekundę.
     * @param mode Sposób odtwarzania.
     * @return Ten sam arkusz.
     */
    SpriteSheet& Clip(
        const std::string& name,
        int first,
        int count,
        float fps,
        Playback mode = Playback::Loop
    );

    /**
     * @brief Definiuje animację z podanej listy klatek arkusza.
     * @param name Nazwa.
     * @param frames Klatki arkusza w kolejności odtwarzania, np. {0, 1, 2, 1}.
     * @param fps Klatki na sekundę.
     * @param mode Sposób odtwarzania.
     * @return Ten sam arkusz.
     */
    SpriteSheet& Clip(
        const std::string& name,
        std::vector<int> frames,
        float fps,
        Playback mode = Playback::Loop
    );

    /**
     * @brief Ustawia czasy kolejnych klatek animacji (zamiast jednakowego 1 / fps).
     * @details Brakujące albo niedodatnie wartości oznaczają 1 / fps. Nieznana nazwa jest
     * pomijana z ostrzeżeniem w logu.
     * @param name Nazwa animacji.
     * @param seconds Czasy klatek w sekundach.
     * @return Ten sam arkusz.
     */
    SpriteSheet& Durations(const std::string& name, std::vector<float> seconds);

    /**
     * @brief Dodaje zdarzenie wejścia w klatkę animacji (np. dźwięk kroku).
     * @details Sprite zgłasza je przez OnFrameEvent(), gdy animacja przejdzie do tej klatki.
     * Nieznana nazwa animacji jest pomijana z ostrzeżeniem w logu.
     * @param name Nazwa animacji.
     * @param frame Klatka animacji (od 0).
     * @param event Nazwa zdarzenia.
     * @return Ten sam arkusz.
     */
    SpriteSheet& Event(const std::string& name, int frame, std::string event);

    /**
     * @brief Szuka animacji.
     * @param name Nazwa animacji.
     * @return Wskaźnik animacji albo nullptr.
     */
    const SpriteClip* FindClip(const std::string& name) const;

    /** @brief Podaje wszystkie animacje według nazw. */
    const std::map<std::string, SpriteClip>& Clips() const { return clips_; }

  private:
    /** @brief Zwraca animację do zmiany albo nullptr z ostrzeżeniem w logu. */
    SpriteClip* EditClip(const std::string& name, const char* function);

    std::shared_ptr<const ::Bitmap> bitmap_;  /**< Bitmapa klatek. */
    std::vector<SpriteFrame> frames_;         /**< Klatki. */
    std::map<std::string, SpriteClip> clips_; /**< Animacje (klucz: nazwa). */
};
