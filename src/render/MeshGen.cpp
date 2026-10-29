#include "render/MeshGen.h"

namespace aethernaut {

void ProceduralModels::init(Shader worldShader) {
    cube = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    sphere = LoadModelFromMesh(GenMeshSphere(1.0f, 14, 14));
    cylinder = LoadModelFromMesh(GenMeshCylinder(1.0f, 1.0f, 14));
    cone = LoadModelFromMesh(GenMeshCone(1.0f, 1.0f, 14));
    torus = LoadModelFromMesh(GenMeshTorus(0.25f, 1.0f, 12, 16));

    cube.materials[0].shader = worldShader;
    sphere.materials[0].shader = worldShader;
    cylinder.materials[0].shader = worldShader;
    cone.materials[0].shader = worldShader;
    torus.materials[0].shader = worldShader;

    loaded = true;
}

void ProceduralModels::unload() {
    if (!loaded) return;
    UnloadModel(cube);
    UnloadModel(sphere);
    UnloadModel(cylinder);
    UnloadModel(cone);
    UnloadModel(torus);
    loaded = false;
}

} // namespace aethernaut
