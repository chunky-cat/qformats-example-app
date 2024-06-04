#pragma once

#include <raylib.h>
#include "QuakeMap.h"

class Scene
{
public:
    Scene(int width = 800, int height = 600);
    void LoadQuakeMap(const std::string &fileName, QuakeMapOptions opts);
    void Run();

private:
    Camera camera;
    QuakeMap *qmap;
};