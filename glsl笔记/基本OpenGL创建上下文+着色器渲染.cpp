// main.cpp - 最基本的 OpenGL 上下文窗口 (带顶点 & 片段着色器标注)
#include <iostream>
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>

// ██████████████████████████████████████████████████████████████████████████████████████████████████████████
// ███ 顶点着色器 ████████████████████████████████████████████████████████████████████████████████████████████
// ██████████████████████████████████████████████████████████████████████████████████████████████████████████
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;   // 顶点位置属性

void main()
{
    gl_Position = vec4(aPos, 1.0);    // 直接输出裁剪空间坐标
}
)";

// ██████████████████████████████████████████████████████████████████████████████████████████████████████████
// ███ 片段着色器 ████████████████████████████████████████████████████████████████████████████████████████████
// ██████████████████████████████████████████████████████████████████████████████████████████████████████████
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;                   // 输出颜色

void main()
{
    FragColor = vec4(0.2, 0.7, 0.9, 1.0);  // 青色
}
)";

// 编译着色器的辅助函数
static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// 创建着色器程序
static GLuint createShaderProgram(const char* vShader, const char* fShader) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vShader);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteProgram(shaderProgram);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main() {
    // 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // 设置 OpenGL 版本和核心模式
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "Basic OpenGL Window", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);

    // 启用垂直同步
    glfwSwapInterval(1);

    // 编译并链接着色器程序
    GLuint shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (!shaderProgram) {
        std::cerr << "Failed to create shader program" << std::endl;
        return -1;
    }

    // 定义一个三角形的顶点数据（在裁剪空间中）
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  // 左下
         0.5f, -0.5f, 0.0f,  // 右下
         0.0f,  0.5f, 0.0f   // 顶部
    };

    // 创建并绑定 VAO 和 VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 设置顶点属性指针
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 主渲染循环
    while (!glfwWindowShouldClose(window)) {
        // 清屏
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 使用着色器程序
        glUseProgram(shaderProgram);

        // 绑定并绘制
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 交换缓冲 & 处理事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
