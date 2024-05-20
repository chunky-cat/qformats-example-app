#include "Scene.h"
#include <string>
#include "rlights.h"
#include <raymath.h>

Scene::Scene(int width, int height)
{
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(800, 600, "Quake Map Viewer");
    SetTargetFPS(60);
    camera = {{2.0f, 5.0f, 2.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f, 0};
}

void Scene::LoadQuakeMap(const std::string &fileName, QuakeMapOptions opts)
{
    if (opts.shader.id != 0)
    {
        opts.shader = shader;
    }
    qmap = new QuakeMap(opts);
    qmap->LoadMapFromFile(fileName);
}

void Scene::Run()
{
    DisableCursor();
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        if (IsKeyPressed(KEY_M))
        {
            EnableCursor();
        }

        if (IsCursorHidden())
        {
            UpdateCamera(&camera, CAMERA_FREE);
        }

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        qmap->DrawQuakeSolids();
        DrawGrid(64, 1.0f); // Draw a grid

        EndMode3D();
        DrawFPS(10, 10);

        EndDrawing();
    }
}
