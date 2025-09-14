// main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <chrono>
#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace fs = std::filesystem;

// ---------- Shader helper ----------
static GLuint CompileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[10240]; glGetShaderInfoLog(shader, sizeof(buf), nullptr, buf);
        std::cerr << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment")
            << " shader compile error:\n" << buf << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint CreateProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertSrc);
    if (!vs) return 0;
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!fs) { glDeleteShader(vs); return 0; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[10240]; glGetProgramInfoLog(prog, sizeof(buf), nullptr, buf);
        std::cerr << "Program link error:\n" << buf << std::endl;
        glDeleteShader(vs); glDeleteShader(fs); glDeleteProgram(prog);
        return 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ---------- Wrap Shadertoy Shader ----------
std::string WrapShadertoyShader(const std::string& code) {
    std::string prelude = R"GLSL(
#version 330 core
out vec4 fragColor;
in vec2 vTex;

uniform vec3 iResolution;
uniform float iTime;
uniform float iTimeDelta;
uniform int iFrame;
uniform vec4 iMouse;

uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;
uniform vec3 iChannelResolution[4];
)GLSL";

    std::string postlude = R"GLSL(
void main() {
    vec2 fragCoord = vTex * iResolution.xy;
    mainImage(fragColor, fragCoord);
}
)GLSL";

    return prelude + code + postlude;
}

// ---------- Load shader file ----------
std::string LoadShaderFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) { std::cerr << "Failed to open shader file: " << path << std::endl; return ""; }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// ---------- Texture class ----------
struct Texture {
    GLuint id = 0;
    int width = 1;
    int height = 1;

    bool loadFromFile(const std::string& path) {
        stbi_set_flip_vertically_on_load(true);
        int nChannels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nChannels, 0);
        if (!data) { std::cerr << "Failed to load texture: " << path << std::endl; return false; }

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        GLenum format = (nChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return true;
    }

    void createEmpty() {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        unsigned char black[4] = { 0,0,0,255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        width = height = 1;
    }

    void bind(int unit) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    void destroy() { if (id) glDeleteTextures(1, &id); id = 0; }
};

std::array<Texture, 4> LoadTextures(const std::string& folder) {
    std::array<Texture, 4> textures;
    if (!fs::exists(folder) || !fs::is_directory(folder)) {
        std::cout << "Texture folder not found: " << folder << ", using empty textures.\n";
        for (auto& t : textures) t.createEmpty();
        return textures;
    }

    size_t idx = 0;
    for (auto& entry : fs::directory_iterator(folder)) {
        if (idx >= textures.size()) break;
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        if (ext != ".png" && ext != ".jpg" && ext != ".jpeg") continue;
        if (textures[idx].loadFromFile(entry.path().string())) ++idx;
    }
    for (; idx < textures.size(); ++idx) textures[idx].createEmpty();
    return textures;
}

// ---------- globals ----------
double g_mouseX = 0.0, g_mouseY = 0.0;
int g_mouseDown = 0;
int g_winWidth = 1280, g_winHeight = 720;
std::chrono::steady_clock::time_point g_start;
int g_frame = 0;

// ---------- callbacks ----------
void cursorPosCallback(GLFWwindow* window, double x, double y) { g_mouseX = x; g_mouseY = y; }
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) { g_mouseDown = (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) ? 1 : 0; }
void framebufferSizeCallback(GLFWwindow* window, int w, int h) { g_winWidth = w; g_winHeight = h; glViewport(0, 0, w, h); }

// ---------- main ----------
int main() {
    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(g_winWidth, g_winHeight, "Evolve Shader", nullptr, nullptr);
    if (!window) { std::cerr << "Failed create window\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // -- MODIFIED: Disable VSync
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n"; return -1;
    }

    // Fullscreen quad
    float quad[] = {
        -1.f,-1.f,0.f,0.f,  1.f,-1.f,1.f,0.f,  1.f,1.f,1.f,1.f,
        -1.f,-1.f,0.f,0.f,  1.f,1.f,1.f,1.f,  -1.f,1.f,0.f,1.f
    };
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Load shader
    std::string shaderCode = LoadShaderFile("shader.frag");
    if (shaderCode.empty()) return -1;
    std::string fragStr = WrapShadertoyShader(shaderCode);

    const char* vertShaderSrc = R"GLSL(
    #version 330 core
    layout(location=0) in vec2 aPos;
    layout(location=1) in vec2 aTex;
    out vec2 vTex;
    void main() { vTex=aTex; gl_Position=vec4(aPos,0.0,1.0); }
    )GLSL";

    GLuint prog = CreateProgram(vertShaderSrc, fragStr.c_str());
    if (!prog) return -1;

    // Load textures
    auto textures = LoadTextures("img");

    // Uniform locations
    glUseProgram(prog);
    GLint locResolution = glGetUniformLocation(prog, "iResolution");
    GLint locTime = glGetUniformLocation(prog, "iTime");
    GLint locTimeDelta = glGetUniformLocation(prog, "iTimeDelta");
    GLint locFrame = glGetUniformLocation(prog, "iFrame");
    GLint locMouse = glGetUniformLocation(prog, "iMouse");
    GLint locChannel[4] = {
        glGetUniformLocation(prog,"iChannel0"),
        glGetUniformLocation(prog,"iChannel1"),
        glGetUniformLocation(prog,"iChannel2"),
        glGetUniformLocation(prog,"iChannel3")
    };
    GLint locChannelRes[4] = {
        glGetUniformLocation(prog,"iChannelResolution[0]"),
        glGetUniformLocation(prog,"iChannelResolution[1]"),
        glGetUniformLocation(prog,"iChannelResolution[2]"),
        glGetUniformLocation(prog,"iChannelResolution[3]")
    };
    for (int i = 0; i < 4; ++i) glUniform1i(locChannel[i], i);

    g_start = std::chrono::steady_clock::now();
    float lastTimeVal = 0.f;

    // -- ADDED: Variables for FPS counter
    double lastFPSTime = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window)) {
        // -- ADDED: FPS Counter Logic
        double currentTime = glfwGetTime();
        frameCount++;
        // Update title every second
        if (currentTime - lastFPSTime >= 1.0) {
            std::string title = "Evolve Shader - FPS: " + std::to_string(frameCount);
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0;
            lastFPSTime = currentTime;
        }

        auto now = std::chrono::steady_clock::now();
        float t = std::chrono::duration<float>(now - g_start).count();
        float dt = t - lastTimeVal; lastTimeVal = t;

        glViewport(0, 0, g_winWidth, g_winHeight);
        glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glUniform3f(locResolution, (float)g_winWidth, (float)g_winHeight, 1.f);
        glUniform1f(locTime, t);
        glUniform1f(locTimeDelta, dt);
        glUniform1i(locFrame, g_frame++);
        glUniform4f(locMouse, (float)g_mouseX, (float)(g_winHeight - g_mouseY), (float)g_mouseDown, 0.f);

        for (int i = 0; i < 4; ++i) {
            textures[i].bind(i);
            glUniform3f(locChannelRes[i], (float)textures[i].width, (float)textures[i].height, 1.f);
        }

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (auto& tex : textures) tex.destroy();
    glDeleteProgram(prog); glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(window); glfwTerminate();
    return 0;
}