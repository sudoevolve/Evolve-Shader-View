// main.cpp - Multi-pass Shader Renderer with Full RAII
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <chrono>
#include <regex>
#include <algorithm>
#include <set>
#include <filesystem>
#include <map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma warning(disable : 4244)
#pragma warning(disable : 4566)

namespace fs = std::filesystem;

struct Texture {
    GLuint id = 0;
    int width = 1;
    int height = 1;

    Texture() = default;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept : id(other.id), width(other.width), height(other.height) {
        other.id = 0;
    }

    Texture& operator=(Texture&& other) noexcept {
        if (this != &other) {
            destroy();
            id = other.id;
            width = other.width;
            height = other.height;
            other.id = 0;
        }
        return *this;
    }

    ~Texture() { destroy(); }

    bool loadFromFile(const std::string& path) {
        destroy();
        stbi_set_flip_vertically_on_load(true);
        int nChannels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nChannels, 0);
        if (!data) {
            std::cerr << "Failed to load texture: " << path << std::endl;
            createEmpty();
            return false;
        }

        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        GLenum format = (nChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        stbi_image_free(data);
        return true;
    }

    void createEmpty() {
        destroy();
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        unsigned char black[4] = { 0, 0, 0, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        width = height = 1;
    }

    void bind(int unit) const {
        if (id) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, id);
        }
    }

    void destroy() {
        if (id) {
            glDeleteTextures(1, &id);
            id = 0;
        }
    }
};

class VertexBuffer {
public:
    GLuint id = 0;
    VertexBuffer() = default;
    VertexBuffer(const float* data, size_t size) {
        glGenBuffers(1, &id);
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&& other) noexcept : id(other.id) { other.id = 0; }
    VertexBuffer& operator=(VertexBuffer&& other) noexcept {
        if (this != &other) { destroy(); id = other.id; other.id = 0; }
        return *this;
    }
    ~VertexBuffer() { destroy(); }
    void bind() const { if (id) glBindBuffer(GL_ARRAY_BUFFER, id); }
    static void unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }
    void destroy() { if (id) { glDeleteBuffers(1, &id); id = 0; } }
};

class VertexArray {
public:
    GLuint id = 0;
    VertexArray() { glGenVertexArrays(1, &id); }
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&& other) noexcept : id(other.id) { other.id = 0; }
    VertexArray& operator=(VertexArray&& other) noexcept {
        if (this != &other) { destroy(); id = other.id; other.id = 0; }
        return *this;
    }
    ~VertexArray() { destroy(); }
    void bind() const { if (id) glBindVertexArray(id); }
    static void unbind() { glBindVertexArray(0); }
    void destroy() { if (id) { glDeleteVertexArrays(1, &id); id = 0; } }
};

class Framebuffer {
public:
    GLuint fbo = 0;
    Texture colorTex;
    Framebuffer() = default;
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept : fbo(other.fbo), colorTex(std::move(other.colorTex)) { other.fbo = 0; }
    Framebuffer& operator=(Framebuffer&& other) noexcept {
        if (this != &other) { destroy(); fbo = other.fbo; colorTex = std::move(other.colorTex); other.fbo = 0; }
        return *this;
    }
    ~Framebuffer() { destroy(); }
    bool create(int w, int h) {
        destroy();
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &colorTex.id);
        glBindTexture(GL_TEXTURE_2D, colorTex.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        colorTex.width = w;
        colorTex.height = h;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex.id, 0);
        bool complete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return complete;
    }
    void bind() const { if (fbo) glBindFramebuffer(GL_FRAMEBUFFER, fbo); }
    static void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }
    void destroy() { if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; } }
};

static GLuint CompileShader(GLenum type, const char* src);
static GLuint CreateProgram(const char* vertSrc, const char* fragSrc);

class GLProgram {
public:
    GLuint id = 0;
    GLProgram() = default;
    explicit GLProgram(const char* vertSrc, const char* fragSrc) {
        id = CreateProgram(vertSrc, fragSrc);
    }
    GLProgram(const GLProgram&) = delete;
    GLProgram& operator=(const GLProgram&) = delete;
    GLProgram(GLProgram&& other) noexcept : id(other.id) { other.id = 0; }
    GLProgram& operator=(GLProgram&& other) noexcept {
        if (this != &other) { destroy(); id = other.id; other.id = 0; }
        return *this;
    }
    ~GLProgram() { destroy(); }
    void use() const { if (id) glUseProgram(id); }
    GLint getUniformLocation(const std::string& name) const {
        return id ? glGetUniformLocation(id, name.c_str()) : -1;
    }
    void destroy() { if (id) { glDeleteProgram(id); id = 0; } }
};

static GLuint CompileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[10240];
        glGetShaderInfoLog(shader, sizeof(buf), nullptr, buf);
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
        char buf[10240];
        glGetProgramInfoLog(prog, sizeof(buf), nullptr, buf);
        std::cerr << "Program link error:\n" << buf << std::endl;
        glDeleteShader(vs); glDeleteShader(fs); glDeleteProgram(prog);
        return 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

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

std::string LoadShaderFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::map<std::string, Texture> g_globalTextureCache;

Texture* GetTextureForPath(const std::string& path) {
    auto it = g_globalTextureCache.find(path);
    if (it != g_globalTextureCache.end()) return &it->second;
    Texture tex;
    if (tex.loadFromFile(path)) {
        auto result = g_globalTextureCache.emplace(path, std::move(tex));
        return &result.first->second;
    }
    else {
        static Texture empty;
        static bool init = false;
        if (!init) { empty.createEmpty(); init = true; }
        return &empty;
    }
}

std::vector<fs::path> ScanGlobalImages() {
    std::vector<fs::path> images;
    std::vector<std::string> extensions = { ".png", ".jpg", ".jpeg" };
    if (!fs::exists("iChannel") || !fs::is_directory("iChannel")) {
        std::cerr << "Warning: 'iChannel' folder not found.\n";
        return images;
    }
    for (const auto& entry : fs::recursive_directory_iterator("iChannel")) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
            images.push_back(entry.path());
        }
    }
    std::sort(images.begin(), images.end());
    return images;
}

struct ChannelInput {
    enum Type { NONE, IMAGE_GLOBAL, BUFFER } type = NONE;
    int bufferIndex = -1;
    int imageIndex = -1;
};

double g_mouseX = 0.0, g_mouseY = 0.0;
int g_mouseDown = 0;
int g_winWidth = 1280, g_winHeight = 720;
std::chrono::steady_clock::time_point g_start;
int g_frame = 0;

void cursorPosCallback(GLFWwindow* window, double x, double y) {
    g_mouseX = x;
    g_mouseY = y;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    g_mouseDown = (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) ? 1 : 0;
}

void framebufferSizeCallback(GLFWwindow* window, int w, int h) {
    g_winWidth = w;
    g_winHeight = h;
    glViewport(0, 0, w, h);
    auto* pFbos = static_cast<std::vector<Framebuffer>*>(glfwGetWindowUserPointer(window));
    if (pFbos) for (auto& fbo : *pFbos) fbo.create(w, h);
}

const char* vertShaderSrc = R"GLSL(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aTex;
out vec2 vTex;
void main() {
    vTex = aTex;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)GLSL";

std::vector<std::string> ScanShaderFiles() {
    std::vector<std::pair<int, fs::path>> entries;
    for (const auto& entry : fs::directory_iterator("frag")) {
        if (entry.is_regular_file() && entry.path().extension() == ".frag") {
            std::string stem = entry.path().stem().string();
            std::regex numPattern(R"((\d+))");
            std::smatch match;
            if (std::regex_search(stem, match, numPattern)) {
                entries.emplace_back(std::stoi(match[1]), entry.path());
            }
        }
    }
    std::sort(entries.begin(), entries.end());
    std::vector<std::string> result;
    for (auto& [idx, path] : entries) result.push_back(path.string());
    return result;
}

std::vector<std::array<ChannelInput, 4>> ConfigureChannelsInteractively(
    const std::vector<std::string>& files,
    const std::vector<fs::path>& globalImages) {

    int N = static_cast<int>(files.size());
    std::vector<std::array<ChannelInput, 4>> configs(N);
    for (auto& chs : configs) for (int i = 0; i < 4; ++i) chs[i].type = ChannelInput::NONE;

    std::cout << "\n=== Interactive iChannel Setup ===\n";
    std::cout << " Press Enter for auto-chain (buffer0 → buffer1 → ...),\n";
    std::cout << " or type a buffer index to configure manually: ";

    std::string line;
    std::getline(std::cin, line);

    if (line.empty()) {
        std::cout << " Auto-chain: ";
        for (int i = 1; i < N; ++i) {
            configs[i][0].type = ChannelInput::BUFFER;
            configs[i][0].bufferIndex = i - 1;
            std::cout << "buffer" << i - 1 << " → ";
        }
        if (N > 0) std::cout << "buffer" << N - 1 << "\n\n";
        return configs;
    }

    while (true) {
        try {
            int idx = std::stoi(line);
            if (idx < 0 || idx >= N) {
                std::cout << " Invalid index. Choose 0-" << N - 1 << ": ";
                std::getline(std::cin, line);
                continue;
            }
            std::string fname = fs::path(files[idx]).filename().string();
            std::cout << "\n Configuring buffer" << idx << " (" << fname << "):\n";
            for (int c = 0; c < 4; ++c) {
                std::cout << "\niChannel" << c << " source:\n";
                std::cout << "  1: none\n";
                std::cout << "  2: self (feedback)\n";
                if (idx > 0) {
                    for (int src = 0; src < idx; ++src) {
                        std::cout << "  " << (src + 3) << ": buffer" << src << "\n";
                    }
                }
                int base = idx > 0 ? idx + 3 : 3;
                if (!globalImages.empty()) {
                    std::cout << "  " << base << "+: image (see list below)\n";
                }
                std::cout << "> ";
                std::getline(std::cin, line);
                int choice;
                try { choice = std::stoi(line); }
                catch (...) { choice = -1; }
                ChannelInput input;
                if (choice == 1) input.type = ChannelInput::NONE;
                else if (choice == 2) { input.type = ChannelInput::BUFFER; input.bufferIndex = idx; }
                else if (idx > 0 && choice >= 3 && choice <= idx + 2) { input.type = ChannelInput::BUFFER; input.bufferIndex = choice - 3; }
                else if (!globalImages.empty() && choice >= base) {
                    int imgChoice = choice - base;
                    if (imgChoice >= 0 && imgChoice < (int)globalImages.size()) {
                        input.type = ChannelInput::IMAGE_GLOBAL;
                        input.imageIndex = imgChoice;
                    }
                    else { std::cout << " Invalid image index. Skipping.\n"; continue; }
                }
                else { std::cout << " Invalid choice. Skipping.\n"; continue; }
                configs[idx][c] = input;
                std::cout << " Set iChannel" << c << " = ";
                if (input.type == ChannelInput::NONE) std::cout << "none\n";
                else if (input.type == ChannelInput::BUFFER && input.bufferIndex == idx) std::cout << "self\n";
                else if (input.type == ChannelInput::BUFFER) std::cout << "buffer" << input.bufferIndex << "\n";
                else if (input.type == ChannelInput::IMAGE_GLOBAL) std::cout << "image: " << globalImages[input.imageIndex].filename().string() << "\n";
                if (c < 3) {
                    std::cout << "1. Continue\n2. Skip\n> ";
                    std::getline(std::cin, line);
                    if (line != "1") break;
                }
            }
            std::cout << "\nEnter another index, or press Enter to finish: ";
        }
        catch (...) { break; }
        std::getline(std::cin, line);
        if (line.empty()) break;
    }
    for (int i = 1; i < N; ++i) {
        bool empty = true;
        for (int c = 0; c < 4; ++c) if (configs[i][c].type != ChannelInput::NONE) { empty = false; break; }
        if (empty) {
            configs[i][0].type = ChannelInput::BUFFER;
            configs[i][0].bufferIndex = i - 1;
            std::cout << "[AUTO] Auto-connected buffer" << i << " <- buffer" << i - 1 << "\n";
        }
    }
    return configs;
}

int main() {
    auto g_globalImages = ScanGlobalImages();
    if (!g_globalImages.empty()) {
        std::cout << "\nFound " << g_globalImages.size() << " global image(s):\n";
        for (size_t i = 0; i < g_globalImages.size(); ++i) {
            std::cout << "  [" << i << "] " << g_globalImages[i].filename().string()
                << " (" << g_globalImages[i].parent_path().filename().string() << ")\n";
        }
    }

    if (!fs::exists("frag") || !fs::is_directory("frag")) {
        std::cerr << "Error: 'frag' folder not found!\n";
        return -1;
    }
    auto fragFiles = ScanShaderFiles();
    if (fragFiles.empty()) {
        std::cerr << "No .frag files found!\n";
        return -1;
    }
    std::cout << "\nFound " << fragFiles.size() << " shader(s):\n";
    for (size_t i = 0; i < fragFiles.size(); ++i) {
        std::cout << "  [" << i << "] " << fs::path(fragFiles[i]).filename().string() << "\n";
    }

    auto channelConfig = ConfigureChannelsInteractively(fragFiles, g_globalImages);

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(g_winWidth, g_winHeight, "Evolve Shader", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    if (glfwGetWindowAttrib(window, GLFW_SRGB_CAPABLE)) glEnable(GL_FRAMEBUFFER_SRGB);

    VertexArray vao;
    float quad[] = {
        -1.f, -1.f, 0.f, 0.f,
         1.f, -1.f, 1.f, 0.f,
         1.f,  1.f, 1.f, 1.f,
        -1.f, -1.f, 0.f, 0.f,
         1.f,  1.f, 1.f, 1.f,
        -1.f,  1.f, 0.f, 1.f
    };
    VertexBuffer vbo(quad, sizeof(quad));
    vao.bind(); vbo.bind();
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    VertexArray::unbind();

    std::vector<GLProgram> programs;
    for (const auto& file : fragFiles) {
        std::string code = LoadShaderFile(file);
        if (code.empty()) continue;
        code = WrapShadertoyShader(code);
        programs.emplace_back(vertShaderSrc, code.c_str());
    }
    if (programs.empty()) return -1;

    std::vector<Framebuffer> fbos(programs.size() - 1);
    for (auto& fbo : fbos) if (!fbo.create(g_winWidth, g_winHeight)) return -1;
    glfwSetWindowUserPointer(window, &fbos);

    Texture emptyTex; emptyTex.createEmpty();
    g_start = std::chrono::steady_clock::now();
    float lastTimeVal = 0.0f;
    double lastFPSTime = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        frameCount++;
        if (currentTime - lastFPSTime >= 1.0) {
            std::string title = "Evolve Shader - FPS: " + std::to_string(frameCount);
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0;
            lastFPSTime = currentTime;
        }

        auto now = std::chrono::steady_clock::now();
        float t = std::chrono::duration<float>(now - g_start).count();
        float dt = t - lastTimeVal;
        lastTimeVal = t;

        int width = g_winWidth;
        int height = g_winHeight;

        std::vector<Texture*> bufferOutputs(programs.size(), &emptyTex);

        for (size_t i = 0; i < programs.size(); ++i) {
            programs[i].use();
            glUniform3f(programs[i].getUniformLocation("iResolution"), (float)width, (float)height, 1.0f);
            glUniform1f(programs[i].getUniformLocation("iTime"), t);
            glUniform1f(programs[i].getUniformLocation("iTimeDelta"), dt);
            glUniform1i(programs[i].getUniformLocation("iFrame"), g_frame++);
            glUniform4f(programs[i].getUniformLocation("iMouse"),
                (float)g_mouseX, (float)(height - g_mouseY), (float)g_mouseDown, 0.0f);

            auto& configForThis = channelConfig[i];
            Framebuffer* targetFbo = nullptr;

            for (int c = 0; c < 4; ++c) {
                const ChannelInput& input = configForThis[c];
                std::string name = "iChannel" + std::to_string(c);
                GLint loc = programs[i].getUniformLocation(name);
                if (loc == -1) continue;
                Texture* texToBind = &emptyTex;
                switch (input.type) {
                case ChannelInput::NONE: break;
                case ChannelInput::IMAGE_GLOBAL:
                    if (input.imageIndex >= 0 && input.imageIndex < (int)g_globalImages.size()) {
                        std::string imgPath = g_globalImages[input.imageIndex].string();
                        texToBind = GetTextureForPath(imgPath);
                    }
                    break;
                case ChannelInput::BUFFER:
                    if (input.bufferIndex == (int)i) texToBind = bufferOutputs[i];
                    else if (input.bufferIndex >= 0 && input.bufferIndex < (int)bufferOutputs.size())
                        texToBind = bufferOutputs[input.bufferIndex];
                    break;
                }
                texToBind->bind(c);
                glUniform1i(loc, c);
                std::string resName = "iChannelResolution[" + std::to_string(c) + "]";
                GLint resLoc = programs[i].getUniformLocation(resName);
                if (resLoc != -1) {
                    glUniform3f(resLoc, (float)texToBind->width, (float)texToBind->height, 1.0f);
                }
            }

            if (i == programs.size() - 1) {
                Framebuffer::unbind();
                glViewport(0, 0, width, height);
            }
            else {
                targetFbo = &fbos[i];
                targetFbo->bind();
                glViewport(0, 0, width, height);
            }

            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            vao.bind();
            glDrawArrays(GL_TRIANGLES, 0, 6);
            VertexArray::unbind();

            if (targetFbo) bufferOutputs[i] = &targetFbo->colorTex;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}