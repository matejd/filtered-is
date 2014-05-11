#ifndef __RENDERER_HPP__
#define __RENDERER_HPP__

#include "common.hpp"

#include <GL/glew.h>
#include <GL/glfw.h>
#include <string>
#include <vector>
#include <map>

struct Vertex {
    float px,py,pz;
    float nx,ny,nz;
};

typedef u32 Index;

#define CGLE checkGLError(__FILE__, __LINE__)
void checkGLError(const char* file, int line);

typedef int TextureID;
typedef int ShaderID;
typedef int MeshID;
typedef int FramebufferID;
typedef int RenderbufferID;

struct Texture;
struct Shader;
struct Mesh;
struct Renderbuffer;
struct Framebuffer;

enum class PixelFormat {
    R,
    Rgb,
    Rgba,
    Depth16
};

enum class PixelType {
    Ubyte,
    Float
};

class Renderer {
public:
    Renderer(int canvasWidth, int canvasHeight);
    ~Renderer();

    TextureID addTexture(const std::string& filename, PixelFormat internal, PixelFormat input, PixelType type);
    TextureID addCubemap(const std::string& basefile, PixelFormat internal, PixelFormat input, PixelType type);
    TextureID addEmptyTexture(int width, int height, PixelFormat format, PixelType type);
    ShaderID addShader(const std::vector<std::string>& vsFiles, const std::vector<std::string>& fsFiles);
    ShaderID addShaderFromSource(const std::string& vsSource, const std::string& fsSource);
    MeshID addMesh(const std::string& filename);

    FramebufferID addFramebuffer();
    RenderbufferID addRenderbuffer(int width, int height, PixelFormat format);
    void attachTextureToFramebuffer(FramebufferID framebuffer, TextureID color);
    void attachRenderbufferToFramebuffer(FramebufferID framebuffer, RenderbufferID renderbuffer);

    void setShader(ShaderID shader);
    void setUniform1i(const std::string& name, int value);
    void setUniform1f(const std::string& name, float value);
    void setUniform2fv(const std::string& name, int count, const float* value);
    void setUniform3fv(const std::string& name, int count, const float* value);
    void setUniform4fv(const std::string& name, int count, const float* value);
    void setUniform4x4fv(const std::string& name, int count, const float* value);

    void setFramebuffer(FramebufferID framebuffer);
    void setDefaultFramebuffer();
    void setViewport(int x, int y, int width, int height);
    void setTexture(int unit, TextureID id);

    void drawMesh(MeshID id);
    void drawScreenQuad();

    void liveReloadUpdate();

private:
    std::vector<Texture*> textures;
    std::vector<Shader*> shaders;
    std::vector<Mesh*> meshes;
    std::vector<Renderbuffer*> renderbuffers;
    std::vector<Framebuffer*> framebuffers;

    // Live reload on change
    struct ShaderTrackingInfo {
        std::vector<std::string> vsFilenames;
        std::vector<std::string> fsFilenames;
        u64 lastModificationTime;
    };
    std::map<ShaderID, ShaderTrackingInfo> trackedShaderFiles;
    bool dontAddToTrackedFiles = false;

    ShaderID currentShader;
    GLuint quadVB;
};

#endif
