#ifndef __APP_HPP__
#define __APP_HPP__

#include "renderer.hpp"

#include <GL/glew.h>
#include <GL/glfw.h>
#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class App {
public:
    App(int canvasWidth, int canvasHeight): canvasWidth(canvasWidth), canvasHeight(canvasHeight) {}
    ~App();

    bool checkPlatform();
    bool setup();
    void drawFrame();

    void onKey(int key, int action);
    void onChar(int key, int action);
    void onMousePos(int x, int y);
    void onMouseButton(int button, int action);
    void onMouseWheel(int pos);

    void setValue(const std::string& param, const std::string& value);

private:
    int canvasWidth, canvasHeight;
    int mouseStartX, mouseStartY;
    bool dragging = false;
    float cameraPhi   = 0.f;
    float cameraTheta = PI2;
    float cameraR     = 3.f;

    Renderer* renderer = nullptr;
    MeshID meshes[3];
    int currentMeshInd = 0;
    ShaderID meshShader;
    ShaderID envShader;
    //TextureID envCubemap;
    TextureID envPanorama;
    float roughness = 0.05f;
    float lod = 0.5f;
    glm::vec3 F0 = glm::vec3(0.03f);
    glm::vec3 kd = glm::vec3(0.42f, 0.008f, 0.008f);
    float gamma = 1.f;
    int numSamples = 30;
    std::vector<glm::vec3> whs;

    std::string cmd, previousCmd;
};

#endif
