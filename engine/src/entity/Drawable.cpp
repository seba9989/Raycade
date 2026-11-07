/**
 * @file Drawable.cpp
 * @brief Rysowanie obiektu z jego stanem renderera i sortowanie po warstwach.
 */
#include <raycade/entity/Drawable.hpp>

void Drawable::Render(PrimitiveRenderer& renderer) const {
    renderer.Reset().TargetScreen();
    // raylib nie ma stosu trybów mieszania, więc tryb dotyczy tylko własnego Draw()
    if (blend_ != BLEND_ALPHA) ::BeginBlendMode(blend_);
    Draw(renderer);
    if (blend_ != BLEND_ALPHA) ::EndBlendMode();
}

std::vector<const Drawable*> Drawable::ByLayer(std::vector<const Drawable*> objects) {
    std::stable_sort(objects.begin(), objects.end(),
                     [](const Drawable* a, const Drawable* b) { return a->Layer() < b->Layer(); });
    return objects;
}
