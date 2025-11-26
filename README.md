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

// 添加到视口
ScenePreview->AddToViewport();
```

### 配置选项

- **Entries**: 预览的Actor条目数组，每个条目包含Actor类和生成变换
- **PreviewMaterial**: 预览材质，必须包含"PreviewTexture"参数
- **CameraTransform**: 摄像机位置、旋转和缩放
- **CameraProjectionType**: 投影模式（ECameraProjectionMode::Perspective/Orthographic）
- **CameraFOVAngle**: 透视投影视野角度（度）
- **CameraOrthoWidth**: 正交投影宽度


## 安装步骤

1. 克隆或下载本项目到您的Unreal Engine项目目录
2. 在编辑器中打开项目
3. 导入所需的资源文件
4. 在蓝图中引用UScenePreviewWidget组件

## 材质配置

### 创建预览材质
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

## 常见问题

### 预览不显示
- 检查PreviewMaterial是否包含"PreviewTexture"参数
- 验证材质是否已正确应用到PreviewImage组件
- 确认Entries数组中包含有效的Actor类引用

### 性能优化
- 限制预览场景中的Actor数量
- 使用较低分辨率的渲染目标
- 适当调整更新频率

### 交互功能
- UScenePreviewWidget支持鼠标交互，可通过蓝图事件处理用户输入
- 可实现旋转、缩放、平移等交互操作


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


