# 🖥️ OpenGL 渲染流程


## 1. 创建窗口 & 上下文

Window & OpenGL Context

工具：GLFW / SDL / Win32 等

作用：提供显示区域并建立 GPU 渲染环境



## 2. 编写 Shader（GLSL）

Shader (GLSL)

顶点着色器 Vertex Shader (VS)：控制顶点在屏幕位置

片段着色器 Fragment Shader (FS)：控制每个像素颜色


作用：告诉 GPU 顶点如何变换，片段如何着色



## 3. 编译 & 链接 Shader → Shader Program

Shader Program

作用：GPU 可执行程序，知道如何执行顶点和片段计算



## 4. 创建顶点数据（VBO）

Vertex Buffer Object (VBO)

顶点坐标、颜色、法线、UV 等

作用：把模型或全屏矩形数据上传到 GPU



## 5. 创建顶点数组对象（VAO）并绑定 VBO

Vertex Array Object (VAO)

配置顶点属性：位置、步长、偏移

作用：告诉 GPU 数据如何解析、顶点属性如何对应 Shader



## 6. 传递 Uniform 数据

Uniforms

时间（iTime）、分辨率（iResolution）、矩阵、纹理等

作用：动态参数传给 Shader，使每帧计算不同效果



## 7. 渲染循环（每帧执行）

Render Loop / Frame Loop

更新 Uniform（时间、鼠标等）

绑定 VAO

调用 glDrawArrays / glDrawElements



## 8. GPU 渲染流程

GPU Pipeline


1. 顶点着色器 Vertex Shader：每个顶点运行一次 → 输出屏幕坐标 + 传递属性


2. 光栅化 Rasterization：把三角形拆成片段（像素候选）


3. 片段着色器 Fragment Shader：每个片段运行一次 → 计算最终颜色


4. 写入帧缓冲 Framebuffer → 输出屏幕




## 9. 显示到窗口

Framebuffer → Screen / Swap Buffers

GPU 输出帧缓冲内容到屏幕

形成最终图像





---

# 🔹 核心概念对照（英文全称）

对象 / 阶段	英文全称	作用

VBO	Vertex Buffer Object	存顶点数据（位置、颜色、UV）

VAO	Vertex Array Object	告诉 GPU 顶点数据如何解析、对应 Shader 属性

Uniform	Uniform Variable	给 Shader 传入每帧参数（时间、分辨率、纹理）

顶点着色器	Vertex Shader (VS)	控制顶点位置和传递顶点属性

光栅化	Rasterization	将三角形转换为片段（像素）

片段着色器	Fragment Shader (FS)	控制每个像素的颜色，生成图像

帧缓冲	Framebuffer	GPU 存储最终颜色 → 输出到屏幕

渲染循环	Render Loop / Frame Loop	每帧执行渲染流程，更新 Uniform 并绘制

## 示例

```cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// 顶点着色器（Vertex Shader）——固定全屏矩形
const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)glsl";

// 片段着色器（Fragment Shader）——全屏渐变
const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
uniform vec2 iResolution; // 分辨率
void main()
{
    // 归一化坐标 uv (0~1)
    vec2 uv = gl_FragCoord.xy / iResolution;
    // 渐变色
    FragColor = vec4(uv, 0.5, 1.0);
}
)glsl";

// 错误检查函数
void checkCompileErrors(GLuint shader, std::string type)
{
    GLint success;
    GLchar infoLog[1024];
    if(type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                      << infoLog << "\n";
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                      << infoLog << "\n";
        }
    }
}

int main()
{
    // 1. 初始化 GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Simple GLSL Example", nullptr, nullptr);
    if (!window) { std::cout << "Failed to create GLFW window\n"; return -1; }
    glfwMakeContextCurrent(window);

    // 2. 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    // 3. 编译顶点着色器
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    checkCompileErrors(vertexShader, "VERTEX");

    // 4. 编译片段着色器
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    checkCompileErrors(fragmentShader, "FRAGMENT");

    // 5. 链接 Shader Program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkCompileErrors(shaderProgram, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 6. 顶点数据（全屏矩形）
    float vertices[] = {
        -1.0f, -1.0f, 0.0f, // 左下
         1.0f, -1.0f, 0.0f, // 右下
        -1.0f,  1.0f, 0.0f, // 左上
         1.0f,  1.0f, 0.0f  // 右上
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 7. 渲染循环
    while(!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // 设置分辨率 Uniform
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glUniform2f(glGetUniformLocation(shaderProgram, "iResolution"), (float)width, (float)height);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 8. 清理资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
```

