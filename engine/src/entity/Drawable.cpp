/**
 * @file Drawable.cpp
 * @brief Rysowanie obiektu z jego stanem renderera i sortowanie po warstwach.
 */
#include <raycade/entity/Drawable.hpp>

void Drawable::Render(PrimitiveRenderer& renderer) const {
    renderer.Reset().TargetScreen();
    BeginEffects();
    Draw(renderer);
    EndEffects();
}

void Drawable::BeginEffects() const {
    // raylib nie ma stosu trybów mieszania ani shaderów, więc oba dotyczą tylko własnego
    // rysunku obiektu - dzieci rysują się już ze swoimi ustawieniami
    if (blend_ != BLEND_ALPHA) ::BeginBlendMode(blend_);
    if (shader_ && shader_->IsLoaded()) ::BeginShaderMode(shader_->Raw());
}

void Drawable::EndEffects() const {
    if (shader_ && shader_->IsLoaded()) ::EndShaderMode();
    if (blend_ != BLEND_ALPHA) ::EndBlendMode();
}

std::vector<const Drawable*> Drawable::ByLayer(std::vector<const Drawable*> objects) {
    std::stable_sort(objects.begin(), objects.end(), [](const Drawable* a, const Drawable* b) {
        return a->Layer() < b->Layer();
    });
    return objects;
}
