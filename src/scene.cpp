#include "scene.h"

#include "camera.h"
#include "debug.h"
#include "font.h"
#include "highway.h"
#include "settings.h"
#include "sprite.h"
#include "ui.h"

#include <stdio.h>

void Scene::init() {
    Camera::init();
    Font::init();
    Sprite::init();
    //Ui::init();
}

void Scene::tick() {
    Camera::tick();
    Debug::tick();
    Font::tick();
    //Ui::tick();
}

void Scene::render() {
    Font::render();
    Highway::render();
    //Ui::render();
}
