#include "renderer.hpp"

// stblib image loading library, single-file, public domain
// https://code.google.com/p/stblib/
#define STBI_HEADER_FILE_ONLY
#include "stb_image.cpp"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <cassert>
#include <cmath>

struct Mesh {
    GLuint vbid;
    GLuint ibid;
    GLsizei numIndices;
};

struct Shader {
    GLuint id;
    std::unordered_map<std::string, GLint> uniforms;
};

struct Texture {
    GLuint id;
    bool isCubemap;
    int width, height;
};

struct Framebuffer {
    GLuint id;
};

struct Renderbuffer {
    GLuint id;
};

void checkGLError(const char* file, int line)
{
    const GLenum error = glGetError();
    if (error == GL_NO_ERROR)
        return;
    std::cout << "OpenGL error (" << file << ":" << line << "): ";
    switch (error) {
        case GL_INVALID_ENUM:      std::cout << "invalid enum"; break;
        case GL_INVALID_VALUE:     std::cout << "invalid value"; break;
        case GL_INVALID_OPERATION: std::cout << "invalid operation"; break;
        case GL_STACK_OVERFLOW:    std::cout << "stack overflow"; break;
        case GL_OUT_OF_MEMORY:     std::cout << "out of memory"; break;
        case GL_TABLE_TOO_LARGE:   std::cout << "table too large"; break;
        default:                   std::cout << "unknown"; break;
    }
    std::cout << std::endl;
    assert(false);
}

Renderer::Renderer(int canvasWidth, int canvasHeight)
{
    const float quadIndices[] = {
        0.f, 1.f, 2.f,
        0.f, 2.f, 3.f
    };

    glGenBuffers(1, &quadVB);
    glBindBuffer(GL_ARRAY_BUFFER, quadVB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Renderer::~Renderer()
{
    for (Shader* shader: shaders) {
        delete shader;
    }

    for (Texture* texture: textures) {
        delete texture;
    }

    for (Mesh* mesh: meshes) {
        delete mesh;
    }

    for (Renderbuffer* renderbuffer: renderbuffers) {
        delete renderbuffer;
    }

    for (Framebuffer* framebuffer: framebuffers) {
        delete framebuffer;
    }
}

ShaderID Renderer::addShaderFromSource(const std::string& vsSource, const std::string& fsSource)
{
    assert(fsSource.size() > 0);
    std::string vsSourceFinal = vsSource;
    if (vsSource.size() == 0) {
        vsSourceFinal = std::string("attribute float index;\n") +
                        "uniform vec2 uv[4];\n" +
                        "varying vec2 vuv;\n" +
                        "void main(){\n" +
                        "int iindex = int(index);\n" +
                        "vuv = uv[iindex];\n" +
                        "gl_Position = vec4(vuv*2.0-1.0, 0.5, 1.0);}";
    }
    std::string fsHeader = std::string("#if GL_ES\n") +
        "#ifdef GL_FRAGMENT_PRECISION_HIGH\n" +
        "precision highp float;\n" +
        "#else\n" +
        "precision mediump float;\n" +
        "#endif\n" +
        "#endif\n";
    std::string fsSourceFinal = fsHeader + fsSource;
    GLenum types[] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    GLuint ids[2];
    for (int i = 0; i < 2; i++) {
        ids[i] = glCreateShader(types[i]);
        const char* ptr = (i == 0) ? &vsSourceFinal[0] : &fsSourceFinal[0];
        glShaderSource(ids[i], 1, &ptr, nullptr);
        glCompileShader(ids[i]);
        GLint status = 0;
        glGetShaderiv(ids[i], GL_COMPILE_STATUS, &status);
        if (!status) {
            GLchar info[1024];
            GLsizei length = 0;
            glGetShaderInfoLog(ids[i], sizeof(info), &length, info);
            std::cout << "Failed to compile:" << std::endl << info << std::endl;
            assert(false);
            return -1;
        }
    }

    std::vector<std::string> attributes;
    std::vector<std::string> uniforms;
    for (int i = 0; i < 2; i++) {
        std::stringstream ss;
        if (i == 0)
            ss << vsSourceFinal;
        else
            ss << fsSourceFinal;
        ss.seekp(0);
        std::string token;
        ss >> token;
        // This is very fragile!
        while (token != "main" && !ss.eof()) {
            if (token == "uniform") {
                std::string type, name;
                ss >> type >> name;
                name = name.substr(0, name.find_first_of("[ ;"));
                uniforms.push_back(name);
            }
            else if (token == "attribute") {
                std::string type, name;
                ss >> type >> name;
                name = name.substr(0, name.find_first_of("[ ;"));
                attributes.push_back(name);
            }

            ss >> token;
        }
    }

    Shader* shader = new Shader;
    shader->id = glCreateProgram();
    glAttachShader(shader->id, ids[0]);
    glAttachShader(shader->id, ids[1]);
    for (int i = 0; i < attributes.size(); i++) {
        glBindAttribLocation(shader->id, i, attributes[i].c_str());
    }
    glLinkProgram(shader->id);
    GLint linked = 0;
    glGetProgramiv(shader->id, GL_LINK_STATUS, &linked);
    assert(linked);
    for (std::string name : uniforms) {
        shader->uniforms[name] = glGetUniformLocation(shader->id, name.c_str());
        if (shader->uniforms[name] == -1) {
            std::cout << "Failed to get uniform " << name << " location!" << std::endl;
            assert(false);
        }
    }

    shaders.push_back(shader);
    return shaders.size()-1;
}

ShaderID Renderer::addShader(const std::vector<std::string>& vsFiles, const std::vector<std::string>& fsFiles)
{
    std::stringstream ss;
    ss << "Uploading shaders: ";
    std::copy(vsFiles.begin(), vsFiles.end(), std::ostream_iterator<std::string>(ss, " "));
    ss << "+ ";
    std::copy(fsFiles.begin(), fsFiles.end(), std::ostream_iterator<std::string>(ss, " "));
    std::cout << ss.str() << std::endl;

    ByteBuffer vsSource = "";
    ByteBuffer fsSource = "";
    for (const std::string& file: vsFiles) {
        vsSource += getFileContents(file) + "\n";
    }
    for (const std::string& file: fsFiles) {
        fsSource += getFileContents(file) + "\n";
    }
    const ShaderID id = addShaderFromSource(vsSource, fsSource);
    if (!dontAddToTrackedFiles) {
        ShaderTrackingInfo info;
        info.vsFilenames = vsFiles;
        info.fsFilenames = fsFiles;
        u64 modTime = 0;
        for (const std::string& name: vsFiles) {
            modTime = std::max(modTime, getFileModificationTime(name));
        }
        for (const std::string& name: fsFiles) {
            modTime = std::max(modTime, getFileModificationTime(name));
        }
        info.lastModificationTime = modTime;
        trackedShaderFiles[id] = info;
    }
    return id;
}

void Renderer::setShader(ShaderID shader)
{
    assert(shader >= 0 && shader < shaders.size());
    glUseProgram(shaders[shader]->id);
    currentShader = shader;
}

void Renderer::setUniform1i(const std::string& name, int value)
{
    Shader* shader = shaders[currentShader];
    assert(shader->uniforms.find(name) != shader->uniforms.end());
    glUniform1i(shader->uniforms[name], value);
}

void Renderer::setUniform1f(const std::string& name, float value)
{
    Shader* shader = shaders[currentShader];
    assert(shader->uniforms.find(name) != shader->uniforms.end());
    glUniform1f(shader->uniforms[name], value);
}

void Renderer::setUniform4x4fv(const std::string& name, int count, const float* value)
{
    Shader* shader = shaders[currentShader];
    assert(shader->uniforms.find(name) != shader->uniforms.end());
    glUniformMatrix4fv(shader->uniforms[name], count, GL_FALSE, value);
}

void Renderer::setUniform3fv(const std::string& name, int count, const float* value)
{
    Shader* shader = shaders[currentShader];
    assert(shader->uniforms.find(name) != shader->uniforms.end());
    glUniform3fv(shader->uniforms[name], count, value);
}

void Renderer::setUniform4fv(const std::string& name, int count, const float* value)
{
    Shader* shader = shaders[currentShader];
    assert(shader->uniforms.find(name) != shader->uniforms.end());
    glUniform4fv(shader->uniforms[name], count, value);
}

void Renderer::setUniform2fv(const std::string& name, int count, const float* value)
{
    Shader* shader = shaders[currentShader];
    assert(shader->uniforms.find(name) != shader->uniforms.end());
    glUniform2fv(shader->uniforms[name], count, value);
}

void Renderer::setViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void Renderer::setTexture(int unit, TextureID id)
{
    assert(unit >= 0);
    assert(id >= 0 && id < textures.size());
    Texture* texture = textures[id];
    glActiveTexture(GL_TEXTURE0+unit);
    if (texture->isCubemap)
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture->id);
    else
        glBindTexture(GL_TEXTURE_2D, texture->id);
}

MeshID Renderer::addMesh(const std::string& filename)
{
    std::cout << "Uploading mesh: " << filename << std::endl;
    ByteBuffer buffer = getFileContents(filename);
    int numVertices, numIndices;
    numVertices = *reinterpret_cast<int*>(&buffer[0]);
    numIndices  = *reinterpret_cast<int*>(&buffer[sizeof(int)]);
    std::cout << "numVertices: " << numVertices << std::endl;
    std::cout << "numIndices: " << numIndices << std::endl;

    Mesh* mesh = new Mesh;
    mesh->numIndices = numIndices;
    int vpos = 2*sizeof(int);
    int ipos = 2*sizeof(int)+numVertices*sizeof(Vertex);

    glGenBuffers(1, &mesh->vbid);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbid);
    glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vertex), &buffer[vpos], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &mesh->ibid);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibid);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(Index), &buffer[ipos], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    meshes.push_back(mesh);
    return meshes.size()-1;
}

void Renderer::drawMesh(MeshID id)
{
    assert(id >= 0 && id < meshes.size());

    Mesh* mesh = meshes[id];
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibid);
    glBindBuffer(GL_ARRAY_BUFFER,         mesh->vbid);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(0));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<GLvoid*>(3*sizeof(float)));
    glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void Renderer::drawScreenQuad()
{
    const float uv[] = {
        0.f, 0.f,
        1.f, 0.f,
        1.f, 1.f,
        0.f, 1.f
    };

    setUniform2fv("uv", 4, uv);
    glBindBuffer(GL_ARRAY_BUFFER, quadVB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 1*sizeof(float), reinterpret_cast<GLvoid*>(0));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

TextureID Renderer::addEmptyTexture(int width, int height, PixelFormat format, PixelType type)
{
    int numChannels = 1;
    GLenum glFormat;
    if (format == PixelFormat::R) {
        numChannels = 1;
        glFormat = GL_LUMINANCE;
    }
    else if (format == PixelFormat::Rgb) {
        numChannels = 3;
        glFormat = GL_RGB;
    }
    else if (format == PixelFormat::Rgba) {
        numChannels = 4;
        glFormat = GL_RGBA;
    }
    else
        assert(false);

    GLenum glType;
    if (type == PixelType::Float)
        glType = GL_FLOAT;
    else if (type == PixelType::Ubyte)
        glType = GL_UNSIGNED_BYTE;
    else
        assert(false);

    Texture* tex = new Texture;
    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);
#ifdef EMSCRIPTEN
    glTexImage2D(GL_TEXTURE_2D, 0, glFormat, width, height, 0, glFormat, glType, nullptr);
#else
    GLenum glInternal = glFormat;
    //assert(type == PixelType::Float and format == PixelFormat::Rgb);
    if (type == PixelType::Float and format == PixelFormat::Rgb)
        glInternal = GL_RGB32F;
    glTexImage2D(GL_TEXTURE_2D, 0, glInternal, width, height, 0, glFormat, glType, nullptr);
#endif

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    textures.push_back(tex);
    return textures.size()-1;
}

TextureID Renderer::addTexture(const std::string& filename, PixelFormat internal, PixelFormat input, PixelType type)
{
    std::cout << "Uploading texture: " << filename << std::endl;
    assert(internal == input);

    int numChannels = 1;
    GLenum glInternal;
    GLenum glInput;
    if (internal == PixelFormat::R) {
        numChannels = 1;
        glInternal = GL_LUMINANCE;
    }
    else if (internal == PixelFormat::Rgb) {
        numChannels = 3;
        glInternal = GL_RGB;
    }
    else if (internal == PixelFormat::Rgba) {
        numChannels = 4;
        glInternal = GL_RGBA;
    }
    else
        assert(false);

    glInput = glInternal;

    GLenum glType;
    if (type == PixelType::Float)
        glType = GL_FLOAT;
    else if (type == PixelType::Ubyte)
        glType = GL_UNSIGNED_BYTE;
    else
        assert(false);

    int width, height, n;
    u8* data = nullptr;
    // Delegate all the hard work to the fantastic stb_image
    data = stbi_load(filename.c_str(), &width, &height, &n, numChannels);
    if (data == nullptr) {
        std::cout << stbi_failure_reason() << std::endl;
        assert(false);
    }
    assert(n == numChannels);

    Texture* tex = new Texture;
    tex->isCubemap = false;
    tex->width = width;
    tex->height = height;
    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glTexImage2D(GL_TEXTURE_2D, 0, glInternal, width, height, 0, glInput, glType, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(data);
    textures.push_back(tex);
    return textures.size()-1;
}

TextureID Renderer::addCubemap(const std::string& basefile, PixelFormat internal, PixelFormat input, PixelType type)
{
    //std::cout << "Uploading texture: " << filename << std::endl;
    assert(internal == input);

    int numChannels = 1;
    GLenum glInternal;
    GLenum glInput;
    if (internal == PixelFormat::R) {
        numChannels = 1;
        glInternal = GL_LUMINANCE;
    }
    else if (internal == PixelFormat::Rgb) {
        numChannels = 3;
        glInternal = GL_RGB;
    }
    else if (internal == PixelFormat::Rgba) {
        numChannels = 4;
        glInternal = GL_RGBA;
    }
    else
        assert(false);

    glInput = glInternal;

    GLenum glType;
    if (type == PixelType::Float)
        glType = GL_FLOAT;
    else if (type == PixelType::Ubyte)
        glType = GL_UNSIGNED_BYTE;
    else
        assert(false);

    Texture* tex = new Texture;
    tex->isCubemap = true;
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex->id);

    // Go through mip levels and faces for each mip
    for (int m = 0; m < 6; m++) {
        for (int f = 0; f < 6; f++) {
            std::string filename = basefile + "_m0" + std::to_string(m) +
                                              "_c0" + std::to_string(f) + ".png";
            std::cout << "Uploading texture: " << filename << std::endl;
            int width, height, n;
            u8* data = nullptr;
            data = stbi_load(filename.c_str(), &width, &height, &n, numChannels);
            if (data == nullptr) {
                std::cout << stbi_failure_reason() << std::endl;
                assert(false);
            }
            assert(n == numChannels);
            std::cout << width << " " << height << std::endl;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, m, glInternal, width, height, 0, glInput, glType, data);
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 5);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    CGLE;
    textures.push_back(tex);
    return textures.size()-1;
}

FramebufferID Renderer::addFramebuffer()
{
    Framebuffer* framebuffer = new Framebuffer;
    glGenFramebuffers(1, &framebuffer->id);
    framebuffers.push_back(framebuffer);
    CGLE;
    return framebuffers.size()-1;
}

RenderbufferID Renderer::addRenderbuffer(int width, int height, PixelFormat format)
{
    Renderbuffer* renderbuffer = new Renderbuffer;
    glGenRenderbuffers(1, &renderbuffer->id);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer->id);
    assert(format == PixelFormat::Depth16);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    renderbuffers.push_back(renderbuffer);
    CGLE;
    return renderbuffers.size()-1;
}

void Renderer::attachTextureToFramebuffer(FramebufferID framebuffer, TextureID color)
{
    assert(framebuffer >= 0 && framebuffer < framebuffers.size());
    assert(color >= 0 && color < textures.size());

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[framebuffer]->id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[color]->id, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Failed to attach a color texture to the framebuffer: " << status << "!" << std::endl;
    }
    CGLE;
}

void Renderer::attachRenderbufferToFramebuffer(FramebufferID framebuffer, RenderbufferID buffer)
{
    assert(framebuffer >= 0 && framebuffer < framebuffers.size());
    assert(buffer >= 0 && buffer < renderbuffers.size());

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[framebuffer]->id);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffers[buffer]->id);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Failed to attach a depth buffer to the framebuffer: " << status << "!" << std::endl;
    }
    CGLE;
}

void Renderer::setDefaultFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setFramebuffer(FramebufferID framebuffer)
{
    assert(framebuffer >= 0 && framebuffer < framebuffers.size());
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[framebuffer]->id);
}

void Renderer::liveReloadUpdate()
{
#ifndef EMSCRIPTEN
    dontAddToTrackedFiles = true;
    for (auto it = trackedShaderFiles.begin(); it != trackedShaderFiles.end(); ++it) {
        const ShaderID id        = it->first;
        ShaderTrackingInfo& info = it->second;
        u64 modTime = 0;
        for (const std::string& name: info.vsFilenames) {
            modTime = std::max(modTime, getFileModificationTime(name));
        }
        for (const std::string& name: info.fsFilenames) {
            modTime = std::max(modTime, getFileModificationTime(name));
        }

        if (modTime > info.lastModificationTime) {
            info.lastModificationTime = modTime;

            const ShaderID newId = addShader(info.vsFilenames, info.fsFilenames);
            if (newId != -1) {
                Shader* previousVersion = shaders[id];
                delete previousVersion;
                shaders[id] = shaders[newId];
                shaders.pop_back();
            }
        }
    }
    dontAddToTrackedFiles = false;
#endif
}
