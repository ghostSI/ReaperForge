#include "scene.h"

#include "camera.h"
#include "debug.h"
#include "font.h"
#include "highway.h"
#include "settings.h"
#include "ui.h"

#include <stdio.h>

void Scene::init() {
    Camera::init();
    Highway::init();
    Font::init();
    Ui::init();
}

void Scene::tick() {
    Camera::tick();
    //Debug::tick();
    Ui::tick();
}

void Scene::render() {
    Debug::render();
    Highway::render();
    Camera::render();
    Ui::render();
}
