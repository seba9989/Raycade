/**
 * @file Drawable.hpp
 * @brief Rysowanie jednego obiektu oraz porządkowanie obiektów według warstw.
 */
#pragma once

#include <raylib.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

#include <raycade/render/PrimitiveRenderer.hpp>
#include <raycade/render/ShaderProgram.hpp>

/**
 * @brief Obiekt, którego własny kształt można narysować.
 * @details Macierz i rysowanie dzieci obsługuje Entity.
 */
class Drawable {
  public:
    virtual ~Drawable() = default;

    /**
     * @brief Resetuje pędzel i rysuje własny kształt obiektu.
     * @details Implementacje Draw() podają rendererowi współrzędne lokalne. Entity nakłada
     * macierz i osobno wywołuje Render() dla dzieci, więc każde zaczyna z czystym pędzlem.
     * @param renderer Renderer prymitywów; metoda ustawia jego cel na ekran.
     */
    virtual void Render(PrimitiveRenderer& renderer) const;

    /**
     * @brief Pobiera warstwę rysowania wśród obiektów tego samego rodzica.
     * @return Numer warstwy; większa wartość oznacza rysowanie później.
     */
    int Layer() const { return layer_; }

    /**
     * @brief Przycina rysunek obiektu i jego dzieci do prostokąta (scissor).
     * @details Prostokąt jest w układzie lokalnym obiektu (jak współrzędne w Draw()), więc
     * przesuwa się razem z obiektem, rodzicami i kamerą. Kliknięcia poza nim nie trafiają ani
     * w obiekt, ani w dzieci. Zagnieżdżone przycięcia się przecinają. Przy obrocie przycinana
     * jest obwiednia prostokąta na ekranie - do dokładnego przycinania obróconej grupy służy
     * visual::RenderGroup. Przydaje się np. w przewijanym panelu, minimapie i polu tekstowym.
     * @code{.cpp}
     * scene.Spawn<Group>()
     *     .Position({50, 50})
     *     .ClipRect({0, 0, 300, 200}) // dzieci widać tylko w prostokącie 300x200
     *     .Children([](Group& panel) { panel.Spawn<shapes::Circle>().Radius(250); });
     * @endcode
     * @param area Prostokąt w układzie lokalnym.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& ClipRect(this Self& self, Rectangle area) {
        self.clip_ = area;
        return self;
    }

    /**
     * @brief Wyłącza przycinanie (stan domyślny): `.ClipRect(std::nullopt)`.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& ClipRect(this Self& self, std::nullopt_t) {
        self.clip_.reset();
        return self;
    }

    /** @brief Podaje prostokąt przycięcia (w układzie lokalnym) albo std::nullopt. */
    const std::optional<Rectangle>& ClipRect() const { return clip_; }

    /**
     * @brief Rysuje obiekt przez shader GPU.
     * @details Jak Blend() - dotyczy tylko własnego rysunku obiektu, bez dzieci. Żeby nałożyć
     * efekt na całą grupę, shader ustawia się na visual::RenderGroup (działa na teksturze z
     * rysunkiem dzieci). Shader niepoprawnie wczytany (ShaderProgram::IsValid() == false) jest
     * pomijany.
     * @code{.cpp}
     * auto glow = Shared(ShaderProgram::Load("", "assets/glow.fs"));
     * scene.Spawn<shapes::Circle>().Radius(40).Filled().Shader(glow);
     * @endcode
     * @param shader Shader (współdzielony) albo nullptr - bez shadera.
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Shader(this Self& self, std::shared_ptr<ShaderProgram> shader) {
        self.shader_ = std::move(shader);
        return self;
    }

    /** @brief Podaje shader obiektu (np. do ustawienia uniformów) albo nullptr. */
    const std::shared_ptr<ShaderProgram>& Shader() const { return shader_; }

    /**
     * @brief Ustawia sposób mieszania kolorów z tym, co już narysowano.
     * @details Dotyczy tylko własnego rysunku obiektu, bez dzieci. Przykłady: BLEND_ADDITIVE
     * (światło, ogień - kolory się sumują), BLEND_MULTIPLIED (cień), BLEND_ALPHA (domyślny).
     * @param mode Tryb raylib (BLEND_*).
     * @return Ten sam obiekt.
     */
    template <typename Self> Self& Blend(this Self& self, int mode) {
        self.blend_ = mode;
        return self;
    }

    /** @brief Zwraca tryb mieszania kolorów (BLEND_*). */
    int Blend() const { return blend_; }

    /**
     * @brief Ustawia warstwę wśród obiektów tego samego rodzica.
     * @param layer Numer warstwy.
     * @return Ten sam obiekt jako jego konkretny typ (deducing this, C++23).
     */
    template <typename Self> Self& Layer(this Self& self, int layer) {
        self.layer_ = layer;
        return self;
    }

    /**
     * @brief Ustawia kolor rysowania.
     * @param color Kolor.
     * @return Ten sam obiekt jako jego konkretny typ.
     */
    template <typename Self> Self& Paint(this Self& self, Color color) {
        self.color_ = color;
        return self;
    }

    /** @brief Podaje kolor obiektu ustawiony przez Paint(). */
    Color Paint() const { return color_; }

    /**
     * @brief Ustawia obiekty od najniższej warstwy, zachowując kolejność dodania.
     * @param objects Obiekty o wspólnym rodzicu.
     * @return Wskaźniki w kolejności rysowania.
     */
    static std::vector<const Drawable*> ByLayer(std::vector<const Drawable*> objects);

  protected:
    /**
     * @brief Włącza tryb mieszania i shader obiektu (Blend(), Shader()).
     * @details Para z EndEffects(). Drawable::Render() otacza nimi Draw(); klasa, która rysuje
     * inaczej (np. visual::RenderGroup - teksturę), otacza nimi własne rysowanie.
     */
    void BeginEffects() const;

    /** @brief Wyłącza to, co włączyło BeginEffects(). */
    void EndEffects() const;

    /** @brief Rysuje sam obiekt bez dzieci. */
    virtual void Draw(PrimitiveRenderer& renderer) const = 0;

    Color color_ = BLACK;                   /**< Kolor obiektu. */
    int layer_ = 0;                         /**< Warstwa wśród rodzeństwa. */
    int blend_ = BLEND_ALPHA;               /**< Tryb mieszania kolorów. */
    std::optional<Rectangle> clip_;         /**< Przycięcie rysunku (układ lokalny). */
    std::shared_ptr<ShaderProgram> shader_; /**< Shader własnego rysunku albo nullptr. */
};
