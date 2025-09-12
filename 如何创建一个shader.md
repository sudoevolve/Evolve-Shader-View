---

🌌 Shadertoy 创作入门文档

>帮助你从「一个黑屏」到「能做出属于自己的图形创意」。




---

1. 什么是 Shadertoy？

Shadertoy 是一个在线平台，可以用 GLSL（图形着色器语言） 写代码，实时生成画面。

每一帧画面都是由 mainImage 函数决定的，它会在屏幕的每个像素运行一次，输出颜色。

所以，Shadertoy 本质就是：用数学公式画图。



---

2. 基础环境

Shadertoy 默认提供几个全局变量：

fragCoord → 当前像素坐标 (像素点的位置)

iResolution → 屏幕分辨率 (宽, 高, 像素)

iTime → 当前时间（秒），用来做动画


最基本的程序：

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // 归一化像素坐标 (-1 ~ 1)
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;

    // 输出颜色
    fragColor = vec4(uv, 0.0, 1.0); 
}

👉 这个代码会画一个从蓝到红的渐变背景。


---

3. 从点到形状

要画几何图形，我们需要 距离函数（SDF）。

圆

float circle(vec2 p, float r) {
    return length(p) - r; // 到圆心的距离 - 半径
}

正方形

float box(vec2 p, vec2 b) {
    vec2 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0);
}

在 mainImage 里判断：

float d = circle(uv, 0.3);
vec3 col = d < 0.0 ? vec3(1.0, 0.0, 0.0) : vec3(0.0);
fragColor = vec4(col, 1.0);

👉 显示一个红色的圆。


---

4. 动画

使用 iTime 让图形动起来：

float d = circle(uv, 0.2 + 0.1*sin(iTime));

👉 圆会像心脏一样“跳动”。


---

5. 组合图形

用 min/max 拼接形状：

并集（合并）：min(d1, d2)

交集（重叠）：max(d1, d2)

差集（切掉）：max(d1, -d2)


例子：圆和方形的结合：

float d1 = circle(uv, 0.3);
float d2 = box(uv, vec2(0.2));
float d = min(d1, d2);


---

6. 光照和立体感

在 3D 中，我们用 Raymarching + SDF 来画物体。
流程：

1. 定义物体 SDF（球、立方体等）


2. 从相机出发发射射线


3. 每次前进 d = SDF(p)


4. 如果 d < ε，说明撞到物体了



简单球体：

float sphere(vec3 p, float r) {
    return length(p) - r;
}


---

7. 加颜色 & 特效

根据 uv 上色 → 渐变背景

根据 iTime → 动态变化

根据 d → Glow、阴影、反射


例子：发光圆

float d = circle(uv, 0.3);
float glow = exp(-10.0*abs(d));
vec3 col = vec3(0.0, 0.5, 1.0) * glow;
fragColor = vec4(col, 1.0);


---

8. 创作流程总结

1. 想象图像 → 拆成几何 / 函数（圆、方、波纹、重复）


2. 写 SDF → 定义基本形状


3. 组合运算 → min/max 拼接


4. 加动画 → 时间变量 iTime


5. 加光照 → 法线、点光源


6. 加特效 → Glow、反射、折射




---

9. 小练习

1. 用 SDF 画一个“会呼吸的方块”


2. 做一个“无限重复的圆环阵列”


3. 给立方体加蓝色发光边缘




---

📌 一句话总结：
Shadertoy 创作的核心 = 数学（SDF）+ 光线行进（Raymarch）+ 颜色/特效。
把脑海里的画面分解成几何和函数，就能转成代码。


---


