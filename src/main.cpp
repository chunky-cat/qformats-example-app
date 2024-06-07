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

    QuakeMapOptions opts;
    opts.texturePath = "../data/textures/";
    opts.wadPath = "/Users/tinogohlert/workspace/chunkycat/quake_data/wads/";
    opts.showGrid = true;
    opts.inverseScale = 1;
    //"/Users/tinogohlert/workspace/chunkycat/quake_data/maps/1x1.map"
    //"/Users/tinogohlert/workspace/chunkycat/quake_data/maps/orig/START.MAP"
    scene.LoadQuakeMap("/Users/tinogohlert/workspace/chunkycat/quake_data/maps/1x1.map", opts);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = end_time - start_time;
    std::cout << "map compiled in: " << time / std::chrono::milliseconds(1) << " ms\n";

    scene.Run();
    return 0;
}
