#include "app.hpp"

#include <GL/glew.h>
#include <GL/glfw.h>
#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include <iostream>
#include <cassert>

// We can't use C++ methods as GLFW callbacks. Using a global variable and 
// callback chaining as a workaround.
App* gApp = nullptr;

#ifndef EMSCRIPTEN
std::string gCmd = "";
std::string gPreviousCmd = "";
#endif

// We'll export this function to JavaScript (see index.html for usage).
// Its purpose is to be a dat.gui.js <---> C++ bridge.
// When offline, we'll just capture input and call setAppValue on Enter (see onKey and onChar),
// getting stuff done as simply as possible!
extern "C"
void setAppValue(const char* param, const char* value)
{
    assert(gApp != nullptr);
    gApp->setValue(param, value);
}

void glfwOnMousePos(int x, int y)
{
    gApp->onMousePos(x, y);
}

void glfwOnMouseButton(int button, int action)
{
    gApp->onMouseButton(button, action);
}

void glfwOnMouseWheel(int pos)
{
    gApp->onMouseWheel(pos);
}

void glfwOnKey(int key, int action)
{
#ifndef EMSCRIPTEN
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_BACKSPACE && gCmd.length() > 0) {
            std::cout << '\b' << ' ' << '\b';
            std::cout.flush();
            gCmd.pop_back();
        }
        else if (key == GLFW_KEY_UP) {
            const std::string manySpaces(42, ' ');
            std::cout << '\r' << manySpaces << '\r' << gPreviousCmd;
            std::cout.flush();
            gCmd = gPreviousCmd;
        }
        else if (key == GLFW_KEY_ENTER) {
            std::cout << std::endl;
            gPreviousCmd = gCmd;
            const size_t loc = gCmd.find(' ');
            if (loc == std::string::npos) {
                gCmd.clear();
                return;
            }
            gApp->setValue(gCmd.substr(0, loc).c_str(), gCmd.substr(loc+1).c_str());
            gCmd.clear();
        }
    }
#endif

    gApp->onKey(key, action);
}

void glfwOnChar(int key, int action)
{
#ifndef EMSCRIPTEN
    if (action == GLFW_PRESS && key < 256) {
        // Key should be Latin-1
        std::cout << static_cast<char>(key);
        std::cout.flush();
        gCmd.append(1, key);
    }
#endif

    gApp->onChar(key, action);
}

void glfwDrawFrame()
{
    gApp->drawFrame();
}

int main()
{
    if (glfwInit() != GL_TRUE) {
        std::cout << "Failed to init glfw!" << std::endl;
        return 1;
    }

    glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
    if (glfwOpenWindow(1024,512, 8,8,8,0, 24,0,GLFW_WINDOW) != GL_TRUE) {
        std::cout << "Failed to open a window!" << std::endl;
        glfwTerminate();
        return 2;
    }

    int width, height;
    glfwGetWindowSize(&width, &height);
    gApp = new App(width, height);

    glewInit();
    glfwSetWindowTitle("Filtered Importance Sampling");
    glfwSetKeyCallback(glfwOnKey);
    glfwSetCharCallback(glfwOnChar);
    glfwSetMousePosCallback(glfwOnMousePos);
    glfwSetMouseButtonCallback(glfwOnMouseButton);
    glfwSetMouseWheelCallback(glfwOnMouseWheel);

    if (!gApp->setup()) {
        delete gApp;
        glfwTerminate();
        return 3;
    }

#ifdef EMSCRIPTEN
    emscripten_set_main_loop(glfwDrawFrame, 0, 1);
#else
    while (true) {
        gApp->drawFrame();
        glfwSwapBuffers();

        if (glfwGetKey(GLFW_KEY_ESC) || !glfwGetWindowParam(GLFW_OPENED))
            break;
    }
#endif

    std::cout << "Terminating..." << std::endl;
    delete gApp;
    glfwTerminate();
    return 0;
}
