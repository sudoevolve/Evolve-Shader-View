#🧾 Shadertoy / GLSL 常见缩写对照表

🎨 基础数据类型

缩写	全称	作用

vec2	vector2	存 2 个数 (x,y)，常用来表示 2D 坐标、uv

vec3	vector3	存 3 个数 (x,y,z)，常用来表示 3D 坐标或 RGB 颜色

vec4	vector4	存 4 个数 (x,y,z,w)，常用来表示 RGBA 颜色或齐次坐标

mat2	matrix2x2	2x2 矩阵，用于 2D 旋转、缩放

mat3	matrix3x3	3x3 矩阵，用于 3D 变换

mat4	matrix4x4	4x4 矩阵，用于 3D 投影、相机变换

float	floating point	小数，最基本的数值类型

int	integer	整数




---

🖥️ Shadertoy 内置变量

缩写	含义	作用

fragCoord	fragment coordinate	当前像素的坐标（左下角是 (0,0)）

fragColor	fragment color	当前像素的输出颜色

iResolution	input resolution	屏幕分辨率 (x=宽, y=高, z=像素比)

iTime	input time	从运行开始经过的秒数（做动画）

iMouse	input mouse	鼠标信息 (x,y=位置, z,w=点击时的位置)

iChannel0...3	input channel	输入的纹理/视频/音频，可以采样



---

📐 常见变量命名习惯（缩写）

这些不是固定规则，而是社区习惯。

缩写	常见意思	用途

uv	normalized coords	屏幕归一化坐标，范围 (0~1)

st	same as uv	跟 uv 一样，有人喜欢用 st 表示 screen texcoord

p	point / position	某个点的位置

n	normal	法线（垂直方向向量）

d	direction	光线或射线的方向

t	time	时间变量（常用来替代 iTime）

col	color	颜色向量，一般是 vec3 或 vec4

pos	position	坐标，常见于 3D 场景

ro	ray origin	射线起点 (ray origin)

rd	ray direction	射线方向 (ray direction)

sd	signed distance	有符号距离函数 (SDF)

k	constant	常量，可能用于缩放、曲率等

r	radius	半径，常用于圆、球

a	angle	角度/弧度

s	scale	缩放系数




---

🔧 常用函数缩写

函数	作用

dot(a,b)	点积，计算两个向量的相似度

cross(a,b)	叉积，计算两个向量垂直方向

normalize(v)	把向量变成单位长度（长度=1）

length(v)	计算向量长度

mix(a,b,t)	插值 (1−t)*a + t*b

clamp(x,a,b)	限制数值范围在 [a,b]

smoothstep(a,b,x)	平滑插值（比 step 更柔和）

fract(x)	取小数部分

floor(x)	向下取整

mod(x,y)	取模 (x 除以 y 的余数)

sin(x), cos(x)	三角函数，常用来做波动和旋转

atan(y,x)	反正切，求角度




---

🎯 例子：为什么大家写 uv

vec2 uv = fragCoord / iResolution.xy;

👉 让屏幕坐标 (像素点) 转换为 (0,1) 范围的小数，便于用数学函数画图。
比如：

fragColor = vec4(uv, 0.0, 1.0);

会得到彩色渐变背景。


---

📌 小结：

vec2/3/4 → 向量 (坐标/颜色)

uv/st → 屏幕归一化坐标

iTime / iResolution / iMouse → Shadertoy 内置输入

ro/rd / sd → 常见于光线追踪和 SDF

dot/normalize/mix → 基本数学工具




