#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <filesystem>

#include <qformats/wad/palette.h>
#include <qformats/wad/wad.h>

#include "Scene.h"
#include <map>

using std::cout;
using std::endl;

void banner()
{
    cout << "QFORMAT test app\n"
         << "----------------"
         << endl;
}

int main()
{
    banner();
    auto scene = Scene();
    auto start_time = std::chrono::high_resolution_clock::now();

    Shader shader = LoadShader("../shaders/lighting.vs", "../shaders/lighting.fs");
    int ambientLoc = GetShaderLocation(shader, "ambient");

    scene.LoadQuakeMap("../data/maps/qstart.map", QuakeMapOptions{
                                                      .shader = shader,
                                                      .texturePath = "../data/textures/",
                                                      .wadPath = "../data/wads/",
                                                  });

    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = end_time - start_time;
    std::cout << "map compiled in: " << time / std::chrono::milliseconds(1) << " ms\n";

    scene.Run();
    return 0;
}
