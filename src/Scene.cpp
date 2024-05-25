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
    shader = opts.shader;
    qmap = new QuakeMap(opts);
    qmap->LoadMapFromFile(fileName);
}

void Scene::Run()
{
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    // Ambient light level (some basic lighting)
    int ambientLoc = GetShaderLocation(shader, "ambient");
    SetShaderValue(shader, ambientLoc, (float[4]){0.2f, 0.3f, 0.3f, 1.0f}, SHADER_UNIFORM_VEC4);
    auto lpos = (Vector3){-2, 5, -2};
    auto light = CreateLight(LIGHT_POINT, lpos, Vector3Zero(), RED, shader);

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
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        qmap->DrawQuakeSolids();
        DrawSphereWires(lpos, 2, 2, 2, BLUE);
        DrawGrid(64, 1.0f); // Draw a grid

        EndMode3D();
        DrawFPS(10, 10);

        EndDrawing();
    }
}
