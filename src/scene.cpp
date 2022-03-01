#include "scene.h"

#include "font.h"
#include "settings.h"
#include "sprite.h"
#include "ui.h"

#include <stdio.h>

void Scene::init() {
    Font::init();
    Sprite::init();
    Ui::init();
}

void Scene::tick() {
    Font::tick();
    Ui::tick();
}

void Scene::render() {
    Font::render();
    Ui::render();
}
