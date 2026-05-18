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
- **移动位置**：拖拽小组件到任意位置
- **调整大小**：拖拽窗口边缘或四角

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

## 依赖

- Qt6 (Core, Widgets, Gui)
- DTK6 (Core, Gui, Widget)
