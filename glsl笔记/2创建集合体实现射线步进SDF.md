现在我们有了相机系统，下一步就是创建几何体并实现射线步进。

## 第一步：创建简单的球体SDF

```glsl
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    // 1. 坐标转换
    vec2 uv = fragCoord / iResolution.xy;
    uv = uv * 2.0 - 1.0;
    uv.x = uv.x * (iResolution.x / iResolution.y);
    
    // 2. 相机设置
    vec3 cameraPos = vec3(0.0, 0.0, -5.0);
    vec3 lookAt = vec3(0.0, 0.0, 0.0);
    vec3 cameraUp = vec3(0.0, 1.0, 0.0);
    
    // 3. 计算相机坐标系
    vec3 forward = normalize(lookAt - cameraPos);
    vec3 right = normalize(cross(forward, cameraUp));
    vec3 up = normalize(cross(right, forward));
    
    // 4. 计算射线方向
    vec3 rayDir = normalize(uv.x * right + uv.y * up + forward * 2.0);
    
    // 5. 创建球体的SDF函数
    float sphereSDF(vec3 point, vec3 center, float radius)
    {
        // 计算点到球心的距离
        float distanceToCenter = length(point - center);
        // 减去半径就是到表面的距离
        return distanceToCenter - radius;
    }
    
    // 6. 射线步进参数
    vec3 rayPos = cameraPos;  // 射线起点
    float totalDistance = 0.0;  // 已经走过的总距离
    float maxDistance = 100.0;  // 最大追踪距离
    int maxSteps = 100;        // 最大步数
    float epsilon = 0.001;     // 碰撞检测的精度
    
    // 7. 射线步进循环
    bool hit = false;  // 是否击中物体
    vec3 hitPoint = vec3(0.0);  // 击中点位置
    
    for(int i = 0; i < maxSteps; i++)
    {
        // 检查是否超出最大距离
        if(totalDistance > maxDistance) break;
        
        // 计算当前点到球体的距离
        float distance = sphereSDF(rayPos, vec3(0.0, 0.0, 0.0), 1.0);
        
        // 如果距离很小，说明击中了物体
        if(distance < epsilon)
        {
            hit = true;
            hitPoint = rayPos;
            break;
        }
        
        // 向前步进
        rayPos += rayDir * distance;
        totalDistance += distance;
    }
    
    // 8. 根据是否击中来设置颜色
    vec3 color;
    if(hit)
    {
        // 击中物体 - 显示白色
        color = vec3(1.0, 1.0, 1.0);
    }
    else
    {
        // 没有击中 - 显示背景色（四个象限）
        if(uv.x > 0.0 && uv.y > 0.0) {
            color = vec3(1.0, 0.0, 0.0);  // 第一象限：红色
        } else if(uv.x < 0.0 && uv.y > 0.0) {
            color = vec3(0.0, 1.0, 0.0);  // 第二象限：绿色
        } else if(uv.x < 0.0 && uv.y < 0.0) {
            color = vec3(0.0, 0.0, 1.0);  // 第三象限：蓝色
        } else {
            color = vec3(1.0, 1.0, 0.0);  // 第四象限：黄色
        }
    }
    
    fragColor = vec4(color, 1.0);
}
```

## SDF（符号距离函数）详细解释：

### SDF是什么？
```glsl
// SDF = Signed Distance Function（符号距离函数）
// 返回值的含义：
// 正数：点在物体外部，数值表示到表面的最短距离
// 负数：点在物体内部，绝对值表示到表面的最短距离  
// 零：点正好在物体表面上
```

### 球体SDF的计算：
```glsl
float sphereSDF(vec3 point, vec3 center, float radius)
{
    // 1. 计算点到球心的距离
    // length() = 计算向量的长度（勾股定理）
    float distanceToCenter = length(point - center);
    
    // 2. 减去半径得到到表面的距离
    // 如果点在球外：距离 > 半径，结果为正
    // 如果点在球内：距离 < 半径，结果为负  
    // 如果点在表面：距离 = 半径，结果为0
    
    return distanceToCenter - radius;
}
```

## 射线步进算法解释：

### 算法思路：
```glsl
// 想象你在黑暗中用手电筒找一个球：

// 1. 你站在起点，用手电筒照向远方
// 2. 你问："我当前位置到球的距离是多少？"
// 3. 球回答："还有3米"
// 4. 你向前走3米（因为最近距离就是3米，再近就可能走过了）
// 5. 重复步骤2-4，直到走到球表面

// 这就是射线步进的核心思想！
```

### 步进过程图解：
```
相机位置 ○--------○--------○--------● 球体表面
        0步      1步      2步      3步(击中)

第0步：距离球体5米 → 向前走5米
第1步：距离球体2米 → 向前走2米  
第2步：距离球体0.5米 → 向前走0.5米
第3步：距离球体0.0001米 → 击中！
```

## 完整的shadertoy工作版本：
在 GLSL ES（Shadertoy 用的就是这个）里，函数必须放在全局作用域，不能嵌套在 mainImage 里。，记住了

```glsl
// 球体SDF函数必须放在全局作用域
float sphereSDF(vec3 p, vec3 center, float radius)
{
    return length(p - center) - radius;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    // 1. 坐标转换
    vec2 uv = (fragCoord / iResolution.xy) * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;
    
    // 2. 相机设置
    vec3 cameraPos = vec3(0.0, 0.0, -5.0);  // 相机位置
    vec3 lookAt = vec3(0.0, 0.0, 0.0);      // 看向的目标点
    vec3 cameraUp = vec3(0.0, 1.0, 0.0);    // 相机上方方向
    
    // 3. 计算相机坐标系
    vec3 forward = normalize(lookAt - cameraPos);         // 前方
    vec3 right   = normalize(cross(forward, cameraUp));   // 右方
    vec3 up      = normalize(cross(right, forward));      // 上方
    
    // 4. 计算射线方向
    vec3 rayDir = normalize(uv.x * right + uv.y * up + forward * 2.0);
    
    // 5. 射线步进
    vec3 rayPos = cameraPos;  // 射线起点
    bool hit = false;
    float minDistance = 0.001;  // 碰撞检测精度
    float maxDistance = 100.0;  // 最大追踪距离
    
    for(int i = 0; i < 100; i++)
    {
        // 计算当前点到球体的距离
        float dist = sphereSDF(rayPos, vec3(0.0, 0.0, 0.0), 1.0);
        
        // 如果距离足够小，说明击中了球体
        if(dist < minDistance)
        {
            hit = true;
            break;
        }
        
        // 如果走得太远，停止追踪
        if(dist > maxDistance) break;
        
        // 沿着射线方向前进
        rayPos += rayDir * dist;
    }
    
    // 6. 颜色输出
    vec3 color;
    if(hit)
    {
        color = vec3(1.0, 1.0, 1.0);  // 击中：白色
    }
    else
    {
        color = vec3(0.0, 0.0, 0.0);  // 未击中：黑色
    }
    
    fragColor = vec4(color, 1.0);
}
```

现在你应该能看到屏幕中心有一个白色的球体！球体外面是黑色背景。

下一步可以添加更多几何体和改进渲染效果。