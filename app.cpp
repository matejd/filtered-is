#include "app.hpp"

#include <iostream>
#include <sstream>
#include <random>
#include <chrono>

using namespace glm;

vec3 toVec3(const std::string& str)
{
    std::stringstream ss(str);
    vec3 result;
    ss >> result.x >> result.y >> result.z;
    return result;
}

// Returns a value in [0, 1), inverting digits. See pharr10.
float getRadicalInverse(int n, int base)
{
    assert(n > 0 && base > 1);

    float value = 0.f;
    float invBase = 1.f / base;
    float invBi = invBase;
    while (n > 0) {
        int r  = n % base;
        value += r * invBi;
        invBi *= invBase;
        n     /= base;
    }

    return value;
}

vec3 importanceSampleTrowbridgeReitz(vec2 e12, float roughness)
{
    const float a = roughness;
    const float phi = 2.f*PI*e12.x;
    const float cosTheta = std::sqrt((1.f - e12.y) / (1.f + (a*a-1.f)*e12.y));
    const float sinTheta = std::sqrt(1.f - cosTheta*cosTheta);

    return vec3(sinTheta * std::cos(phi),
                sinTheta * std::sin(phi),
                cosTheta);
}

void App::setValue(const std::string& param, const std::string& value)
{
    if (param == "roughness")
        roughness = std::stof(value);
    else if (param == "F0")
        F0 = toVec3(value);
    else if (param == "kd")
        kd = toVec3(value);
    else if (param == "gamma")
        gamma = std::stof(value);
    else if (param == "mesh")
        currentMeshInd = (value == "Walt")? 0 : 1;
    else if (param == "numSamples")
        numSamples = std::stoi(value);
    else if (param == "lod")
        lod = std::stof(value);

    assert(roughness >= 0.f and roughness <= 1.f);
    assert(gamma >= 1.f     and gamma <= 2.5f);
    assert(numSamples > 0);
    assert(lod >= 0.f       and lod <= 5.f);

    // Compute sample half-vectors wh
    whs.clear();
    whs.reserve(numSamples);
    for (int i = 0; i < numSamples; i++) {
        // Halton quasi-random sequence
        const vec2 halton = vec2(getRadicalInverse(i+1, 2),
                                 getRadicalInverse(i+1, 3));
        whs.push_back(importanceSampleTrowbridgeReitz(halton, roughness));
    }
    assert(whs.size() == numSamples);
}

App::~App()
{
    delete renderer;
}

bool App::checkPlatform()
{
    return true;
}

bool App::setup()
{
    const bool platformOk = checkPlatform();
    if (!platformOk)
        return false;

    renderer   = new Renderer(canvasWidth, canvasHeight);
    meshes[0]  = renderer->addMesh("assets/walt.rawmesh");
    meshes[1]  = renderer->addMesh("assets/icosphere.rawmesh");
    meshShader = renderer->addShader({"assets/mesh.vs"}, {"assets/panorama.part", "assets/mesh.fs"});
    envShader  = renderer->addShader({"assets/env.vs"}, {"assets/panorama.part", "assets/env.fs"});

    envPanorama = renderer->addTexture("assets/grace.tga", PixelFormat::Rgba, PixelFormat::Rgba, PixelType::Ubyte);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    setValue("dummy", "dummy");
    return true;
}

void App::drawFrame()
{
    renderer->liveReloadUpdate();

    const vec3 worldUp = vec3(0.f, 1.f, 0.f);
    const vec3 cameraPosition = cameraR * vec3(
            sin(cameraTheta)*sin(cameraPhi),
            cos(cameraTheta),
            sin(cameraTheta)*cos(cameraPhi)
            );
    const mat4 view = glm::lookAt(cameraPosition,
                             vec3(0.f, 0.f, 0.f),
                             worldUp);
    const float ratio = static_cast<float>(canvasWidth) / canvasHeight;
    const mat4 projection = glm::perspective(60.f, ratio, 0.5f, 100.f);
    const mat4 mvp = projection * view;

    renderer->setViewport(0, 0, canvasWidth, canvasHeight);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    const mat4 modelEnv = glm::scale(mat4(1.f), vec3(10.f));
    const mat4 viewEnv = glm::lookAt(vec3(0.f), -cameraPosition, worldUp);
    const mat4 mvpEnv = projection * viewEnv * modelEnv;

    renderer->setShader(envShader);
    renderer->setUniform4x4fv("mvp", 1, &mvpEnv[0][0]);
    renderer->setTexture(0, envPanorama);
    renderer->setUniform1i("env", 0);
    renderer->setUniform1f("gamma", gamma);
    renderer->drawMesh(meshes[1]); // Icosphere

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    renderer->setShader(meshShader);
    renderer->setUniform4x4fv("mvp", 1, &mvp[0][0]);
    renderer->setUniform3fv("viewOrigin", 1, &cameraPosition[0]);
    renderer->setTexture(0, envPanorama);
    renderer->setUniform1i("env", 0);
    renderer->setUniform3fv("F0", 1, &F0[0]);
    renderer->setUniform3fv("kd", 1, &kd[0]);
    renderer->setUniform1f("roughness", roughness);
    renderer->setUniform1f("lod", lod);
    renderer->setUniform1f("gamma",     gamma);
    renderer->setUniform3fv("whs", numSamples, &whs[0][0]);
    renderer->drawMesh(meshes[currentMeshInd]);
}

void App::onKey(int key, int action)
{
}

void App::onChar(int key, int action)
{
}

void App::onMousePos(int x, int y)
{
    if (dragging) {
        float dx = static_cast<float>(x-mouseStartX);
        float dy = static_cast<float>(y-mouseStartY);
        mouseStartX = x;
        mouseStartY = y;
        cameraPhi   -= (dx / canvasWidth)  * TwoPI;
        cameraTheta -= (dy / canvasHeight) * PI;
        cameraTheta = glm::clamp(cameraTheta, 0.001f, PI-0.001f);
    }
}

void App::onMouseButton(int button, int action)
{
    if (action == GLFW_PRESS and button == GLFW_MOUSE_BUTTON_LEFT) {
        int xpos, ypos;
        glfwGetMousePos(&xpos, &ypos);
        mouseStartX = xpos;
        mouseStartY = ypos;
        dragging = true;
    }
    else {
        dragging = false;
    }
}

void App::onMouseWheel(int pos)
{
}
