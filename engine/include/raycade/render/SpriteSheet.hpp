/**
 * @file SpriteSheet.hpp
 * @brief Klasa SpriteSheet - arkusz klatek z nazwanymi animacjami, współdzielony przez sprite'y.
 */
#pragma once

#include <raylib.h>

#include <map>
#include <memory>
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
 * @brief Arkusz klatek (bitmapa + prostokąty klatek) z nazwanymi animacjami.
 * @details Opis animacji należy do arkusza, a nie do sprite'a, więc dziesięciu przeciwników
 * dzieli jeden obiekt - definiuje się go raz, a każdy shapes::Sprite pamięta tylko własny
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
 *     scene.Spawn<shapes::Sprite>().Sheet(hero).Play("biegnie").Position({i * 40.0f, 0});
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
        frames_.push_back(area);
        return *this;
    }

    /**
     * @brief Podaje prostokąt klatki.
     * @param index Numer klatki; spoza zakresu jest przycinany.
     * @return Prostokąt w pikselach bitmapy; {0, 0, 0, 0} dla arkusza bez klatek.
     */
    Rectangle Frame(int index) const;

    /** @brief Podaje liczbę klatek. */
    int FrameCount() const { return static_cast<int>(frames_.size()); }

    /** @brief Sprawdza, czy arkusz ma niepustą bitmapę i co najmniej jedną klatkę. */
    bool IsLoaded() const { return bitmap_ && !bitmap_->IsEmpty() && !frames_.empty(); }

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
    std::vector<Rectangle> frames_;           /**< Prostokąty klatek. */
    std::map<std::string, SpriteClip> clips_; /**< Animacje według nazw. */
};
