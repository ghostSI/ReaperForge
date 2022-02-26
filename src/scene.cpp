#include "scene.h"

#include "font.h"
#include "settings.h"
#include "sprite.h"

void Scene::init() {
    Font::init();
    Sprite::init();
}

void Scene::tick() {
    Font::tick();
}

void Scene::render() {
    Font::render();
}
