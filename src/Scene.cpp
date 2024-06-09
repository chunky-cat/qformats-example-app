#include "Scene.h"
#include <string>
#include "rlights.h"
#include <raymath.h>

Scene::Scene(int width, int height)
{
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(800, 600, "Quake Map Viewer");
    SetTargetFPS(120);
    camera = {{2.0f, 5.0f, 2.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f, 0};
}

void Scene::LoadQuakeMap(const std::string &fileName, QuakeMapOptions opts)
{
    qmap = new QuakeMap(opts);
    qmap->LoadMapFromFile(fileName);
    auto playerStart = qmap->GetPlayerStart();
    if (playerStart != nullptr)
    {
        camera.position.x = -playerStart->origin.x() / opts.inverseScale;
        camera.position.y = playerStart->origin.z() / opts.inverseScale;
        camera.position.z = playerStart->origin.y() / opts.inverseScale;
        auto angle = (playerStart->angle - 90) * DEG2RAD;
        // Recalculate camera target considering translation and rotation
        auto target = Vector3Transform((Vector3){0, 0, 1}, MatrixRotateZYX((Vector3){0, -angle, 0}));

        camera.target.x = camera.position.x + target.x;
        camera.target.y = camera.position.y + target.y;
        camera.target.z = camera.position.z + target.z;
    }
}

void Scene::Run()
{
    auto lpos = (Vector3){-2, 5, -2};
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
        if (qmap->Opts().showGrid)
        {
            DrawGrid(64, 1.0f); // Draw a grid
        }
        EndMode3D();
        DrawFPS(10, 10);

        EndDrawing();
    }
}
