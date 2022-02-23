#include "scene.h"

#include "type.h"
#include "font.h"
#include "settings.h"
#include "sprite.h"

static World::Type worldType;

void Scene::init()
{
  Font::init();
  Sprite::init();
}

void Scene::tick()
{
  Font::tick();
}

void Scene::render()
{
  Font::render();
}
