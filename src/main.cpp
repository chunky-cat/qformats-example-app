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
    scene.LoadQuakeMap("data/maps/dungeon.map", QuakeMapOptions{
                                                    .texturePath = "data/textures/",
                                                    .wadPath = "data/wads/",
                                                });

    scene.Run();
    return 0;
}
