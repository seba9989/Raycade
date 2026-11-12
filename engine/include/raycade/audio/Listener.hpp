/**
 * @file Listener.hpp
 * @brief Klasa audio::Listener - „uszy” dla emiterów dźwięku w scenie.
 */
#pragma once

#include <raycade/entity/Entity.hpp>

namespace audio {

/**
 * @brief Punkt, z którego słychać emitery (audio::Emitter) tej samej sceny.
 * @details Zwykle dziecko postaci gracza - wtedy dźwięki są głośniejsze, gdy gracz podejdzie
 * do źródła, niezależnie od kamery. Bez słuchacza emitery liczą odległość od środka kamery
 * sceny (SceneCamera::Target()). Działa jeden słuchacz naraz - ostatnio zaktualizowany.
 * @code{.cpp}
 * Player& player = scene.Spawn<Player>();
 * player.Spawn<audio::Listener>();
 * @endcode
 * Słuchacz jest niewidoczny i nie przyjmuje kliknięć.
 */
class Listener : public Entity {
  public:
    /** @brief Wyrejestrowuje słuchacza. */
    ~Listener() override;

  protected:
    /** @brief Nic nie rysuje (słuchacz jest niewidoczny). */
    void Draw(PrimitiveRenderer&) const override {}

    /** @brief Rejestruje słuchacza od razu po dodaniu do sceny. */
    void OnMount() override;

    /** @brief Zapisuje położenie słuchacza w mikserze. */
    void Tick(float dt, const Input& input) override;
};

} // namespace audio
