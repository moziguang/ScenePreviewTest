# ScenePreviewTest

UScenePreviewWidget Demo

## 项目描述

这是ScenePreview插件的演示项目。ScenePreview是一个Unreal Engine插件，提供了UScenePreviewWidget组件，用于在UI中预览3D场景或Actor。本项目展示了插件的核心功能和用法。


## 功能特性

- 在UI界面中实时预览3D场景
- 支持自定义摄像机角度和位置
- 可配置预览场景的缩放和旋转
- 支持交互式操作（旋转、缩放、平移）
- 与Unreal Engine的渲染系统无缝集成
- **支持HDR渲染**（SCS_SceneColorHDR）
- **支持透明背景**（通过材质后处理或直接渲染）
- **动态材质切换**（运行时通过SetMaterial方法）
- **优化的ShowFlags配置**（禁用大气、雾效等干扰效果）


## 使用方法

### 蓝图使用示例（参考BP_WorldMainWidget.uasset）

1. **创建场景预览Widget**
   - 在蓝图中拖入插件里的 BP_ScenePreviewWidget 类

2. **配置预览材质**
   - 设置PreviewMaterial属性，使用包含"PreviewTexture"参数的材质，可以参考 MAT_ScenePreview.uasset
   - 材质必须包含名为"PreviewTexture"的Texture Parameter

3. **配置预览条目**
   - 在Entries数组中添加FScenePreviewWidgetEntry条目
   - 每个条目包含：
     - ActorClassPtr: 要预览的Actor类引用
     - SpawnTransform: Actor的生成变换矩阵

4. **摄像机配置**
   - CameraTransform: 摄像机位置和旋转
   - CameraProjectionType: 投影类型（透视/正交）
   - CameraFOVAngle: 透视投影时的视野角度
   - CameraOrthoWidth: 正交投影时的宽度

### 代码示例

```cpp
// 创建场景预览组件
UScenePreviewWidget* ScenePreview = CreateWidget<UScenePreviewWidget>(this, ScenePreviewClass);

// 配置预览条目
TArray<FScenePreviewWidgetEntry> Entries;
FScenePreviewWidgetEntry Entry;
Entry.ActorClassPtr = YourActorClass;
Entry.SpawnTransform = FTransform::Identity;
Entries.Add(Entry);
ScenePreview->SetEntries(Entries);

// 配置摄像机
ScenePreview->SetCameraTransform(FTransform(FRotator(-20, 0, 0), FVector(0, 0, 200)));
ScenePreview->SetCameraFOVAngle(60.0f);

// 设置后处理材质（可选，用于透明背景等效果）
UMaterialInterface* PostProcessMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/MAT_ScenePreview"));
ScenePreview->SetMaterial(PostProcessMaterial);

// 添加到视口
ScenePreview->AddToViewport();
```


### 配置选项

- **Entries**: 预览的Actor条目数组，每个条目包含Actor类和生成变换
- **PreviewMaterial**: 预览材质，必须包含"PreviewTexture"参数（用于材质后处理模式）
- **CameraTransform**: 摄像机位置、旋转和缩放
- **CameraProjectionType**: 投影模式（ECameraProjectionMode::Perspective/Orthographic）
- **CameraFOVAngle**: 透视投影视野角度（度）
- **CameraOrthoWidth**: 正交投影宽度
- **TextureWidth/TextureHeight**: 渲染目标分辨率（1-4096，默认1024）



## 安装步骤

1. 克隆或下载本项目到您的Unreal Engine项目目录
2. 在编辑器中打开项目
3. 导入所需的资源文件
4. 在蓝图中引用UScenePreviewWidget组件

## 材质配置

### 创建预览材质（用于后处理效果）
1. 创建新的材质资产
2. 添加Texture Parameter，命名为"PreviewTexture"
3. 将Texture Parameter连接到Base Color或其他材质输入
4. 在UScenePreviewWidget的PreviewMaterial属性中引用此材质

### 示例材质设置
```
材质节点图：
Texture Parameter (PreviewTexture) → Base Color
Texture Parameter (PreviewTexture) → Roughness (可选)
```

### 透明背景材质配置
如果需要透明背景效果，可以创建后处理材质：

1. **材质域**: Post Process
2. **混合模式**: Translucent 或 Opaque
3. **节点设置**:
```
SceneTexture (SceneColor) → 处理逻辑 → Emissive Color
SceneTexture (SceneDepth) → Alpha通道控制 → Opacity
```

### 动态设置材质
```cpp
// 运行时切换材质
ScenePreview->SetMaterial(NewMaterial);

// 清除材质（使用直接渲染模式）
ScenePreview->SetMaterial(nullptr);
```


## API方法

### UScenePreviewWidget公共方法

```cpp
// 设置预览条目
void SetEntries(const TArray<FScenePreviewWidgetEntry>& entries);

// 获取生成的Actor
AActor* GetSpawnedActor(const int32 entryIndex) const;

// 摄像机控制
void SetCameraTransform(const FTransform& InCameraTransform);
void SetCameraProjectionType(TEnumAsByte<ECameraProjectionMode::Type> ProjectionType);
void SetCameraOrthoWidth(float OrthoWidth);
void SetCameraFOVAngle(float FOVAngle);

// 渲染目标控制
void SetTextureSize(int32 Width, int32 Height);
int32 GetTextureWidth() const;
int32 GetTextureHeight() const;

// 材质控制（新增）
void SetMaterial(UMaterialInterface* InMaterial);
```



## 渲染配置

### HDR渲染配置
插件默认使用HDR渲染模式（SCS_SceneColorHDR），配置如下：

```cpp
// 渲染目标格式
RenderTargetFormat = RTF_RGBA16f;  // 支持HDR和Alpha通道
PixelFormat = PF_FloatRGBA;        // 浮点精度
SRGB = false;                       // 线性色彩空间
TargetGamma = 1.0;                  // 无Gamma校正
```

### ShowFlags优化（透明背景）
为实现透明背景，已禁用以下效果：
- Atmosphere（大气效果）
- Fog（雾效）
- VolumetricFog（体积雾）
- VolumetricClouds（体积云）
- SkyLighting（天空光照）
- Skybox（天空盒）

同时启用：
- PostProcessing（后处理）
- Lighting（光照）
- StaticMeshes（静态网格）
- SkeletalMeshes（骨骼网格）
- Tonemapper（色调映射）
- Translucency（半透明）

## 常见问题

### 预览不显示
- 检查PreviewMaterial是否包含"PreviewTexture"参数（如果使用材质模式）
- 验证材质是否已正确应用到PreviewImage组件
- 确认Entries数组中包含有效的Actor类引用
- 检查渲染目标纹理是否成功创建
- 查看日志输出，确认材质初始化是否成功

### 透明背景不生效
- 确认使用的是RTF_RGBA16f格式的渲染目标
- 检查ShowFlags配置，确保禁用了大气、雾效等效果
- 如果使用材质后处理，确保材质正确处理Alpha通道
- 验证ClearColor设置为FLinearColor::Transparent

### HDR渲染问题
- 确认CaptureSource设置为SCS_SceneColorHDR
- 检查SRGB设置为false，TargetGamma设置为1.0
- 确保Tonemapper已启用（ShowFlags.SetTonemapper(true)）
- 验证渲染目标格式为RTF_RGBA16f

### 性能优化
- 限制预览场景中的Actor数量
- 使用较低分辨率的渲染目标（如512x512）
- 适当调整更新频率
- 优先使用直接RT渲染模式（无需材质开销）
- 禁用不必要的ShowFlags效果

### 交互功能
- UScenePreviewWidget支持鼠标交互，可通过蓝图事件处理用户输入
- 可实现旋转、缩放、平移等交互操作
- SScenePreviewImage支持标准的Slate交互事件

### 材质切换
- 使用SetMaterial方法可以在运行时动态切换材质
- 传入nullptr可以清除材质，使用直接渲染模式
- 材质切换会自动重新初始化渲染目标




## MIT License

Copyright (c) 2025 ScenePreview Plugin Project

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---

**注意**: ScenePreviewTest项目是ScenePreview插件的演示项目，主要展示UScenePreviewWidget组件的使用方法。
实际的插件代码位于Plugins/ScenePreview目录下。


