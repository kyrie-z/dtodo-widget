# DTodo Widget

基于 DTK6 开发的桌面 Todo 小组件，类似苹果桌面 Widget 效果。

## 功能特性

- 🖥️ 桌面固定：窗口保持在桌面层级，类似 macOS 桌面小组件
- 🌫️ 毛玻璃效果：使用 DBlurEffectWidget 实现背景模糊
- 📏 可调整大小：拖拽边缘调整窗口大小
- 📍 可拖拽定位：拖动小组件到任意位置，位置自动保存
- ✅ 待办管理：添加、完成状态切换、数据持久化
- 🎨 无边框设计：无边框、无标题栏的简洁外观

## 操作说明

- **添加事项**：输入文本后按回车或点击"添加"按钮
- **切换状态**：双击列表项切换完成/未完成（已完成显示删除线）
- **删除事项**：右键点击列表项，选择"删除"
- **移动位置**：拖拽小组件到任意位置
- **调整大小**：拖拽窗口边缘或四角

## DTK 设计规范

本项目遵循 DTK 设计规范：

- **主题适配**：边框、阴影颜色跟随系统主题自动切换
- **毛玻璃效果**：使用 `DBlurEffectWidget` 实现符合规范的模糊效果
- **交互反馈**：列表项支持 hover 状态，右键菜单使用 `DMenu`
- **空态提示**：无待办事项时显示友好的空态界面
- **错误处理**：JSON 数据读写包含完整的错误处理

## 系统要求

- **操作系统**：Deepin 25（或其他基于 Debian 的 Linux 发行版）
- **内核**：6.18+ (amd64)
- **CMake**：3.16+
- **C++ 编译器**：支持 C++17（GCC 12+）

## 依赖

| 依赖库 | 最低版本 | Deepin 25 软件包 |
|--------|---------|-----------------|
| Qt6 (Core, Widgets, Gui) | 6.8.0 | `qt6-base-dev` |
| DTK6 Core | 6.7.34 | `libdtk6core-dev` |
| DTK6 Gui | 6.7.34 | `libdtk6gui-dev` |
| DTK6 Widget | 6.7.37 | `libdtk6widget-dev` |
| X11 | 1.8.7 | `libx11-dev` |
| xkbcommon | 0.5.0 | `libxkbcommon-dev` |

### 安装依赖（Deepin 25）

```bash
sudo apt install qt6-base-dev libdtk6core-dev libdtk6gui-dev libdtk6widget-dev libx11-dev libxkbcommon-dev
```

## 编译

```bash
cd ~/widgets/dtodo-widget
mkdir -p build && cd build
cmake ..
make
```

## 运行

```bash
./build/dtodo-widget
```

## 数据存储

- 待办数据：`~/.local/share/dtodo-widget/todos.json`
- 窗口配置：`~/.config/dtodo-widget.conf`
