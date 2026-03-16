# Flowchart Editor

基于 Qt 6 / QGraphicsView 的桌面流程图建模工具。项目当前已完成从“课程型流程图编辑器”到“可扩展建模平台”的重构，核心特征是：

- 文档模型、渲染层、命令系统解耦
- 统一图元协议，支持任意造型的自定义组件
- 正交流程线、手动弯折、样式化箭头
- 自动保存、最近文件、崩溃恢复
- 本地运行数据落在可执行文件旁的 `runtime-data/`
- 基于 TCP 的 P2P 直连协同：房主开房，其他人输入 `IP:端口` 直连

## 1. 主要能力

### 编辑器能力

- 新建、打开、保存、另存为、关闭文档
- 欢迎页 + 最近文件入口
- 退出前保存确认
- 自动保存恢复快照，异常退出后可恢复
- 画布缩放、旋转、网格背景、网格吸附
- 节点拖拽创建、移动、缩放、旋转、文本编辑
- 多选、复制、粘贴、剪切、删除、全选
- 撤销 / 重做
- 字体、字号、粗体、斜体、下划线
- 边框颜色、填充颜色、填充透明度、线宽
- 置前 / 置后

### 节点与组件能力

- 内置常用流程图节点
- 箭头不属于组件，连接线为独立编辑对象
- 组件扩展包导入
- 自定义组件创建对话框
- 组件管理面板：查看、编辑 JSON、保存、导出、删除、刷新
- 自定义组件采用统一 `drawSpec + ports + style` 协议，可描述任意造型

### 连线能力

- 独立创建连接线
- 正交折线渲染
- 节点连接端口吸附
- 节点移动后连线自动跟随
- 手动拖拽起点、终点、弯折控制点
- 自动路由 / 纵向折线 / 横向折线
- 箭头头型：`classic`、`stealth`、`diamond`、`circle`
- 线型：`solid`、`dash`、`dot`

### 导入导出与互通

- 文档保存为版本化 JSON
- 导出 PNG
- 导出 SVG
- 导出 Mermaid
- 自定义组件扩展包 JSON 导入导出

### 协同能力

- 一端开房监听端口
- 另一端输入房主 `IP + 端口` 直连
- 初次连接发送整张文档快照
- 编辑过程中按整张文档快照同步
- 当前策略为 `last write wins`

## 2. 架构概览

项目按“模型 -> 会话 -> 视图 -> UI -> 持久化 / 扩展 / 协同”分层：

- `DiagramDocument` / `DiagramItemModel`
  - 负责文档数据和图元数据
- `EditorSession`
  - 负责编辑命令、选择、撤销重做、布局、校验、复制粘贴、场景同步
- `CanvasNodeItem` / `CanvasConnectorItem` / `CanvasView`
  - 负责渲染和交互
- `ShapeRegistry`
  - 负责内置组件和外部组件注册
- `ComponentExtensionManager`
  - 负责组件包导入导出和本地组件目录管理
- `DocumentSerializer`
  - 负责文档 JSON 编解码
- `SessionPersistence`
  - 负责最近文件和恢复快照
- `PeerCollaborationManager`
  - 负责 P2P 协同连接和快照同步
- `MainWindow`
  - 负责欢迎页、属性面板、组件管理、协同面板、菜单和工具栏

详细设计见：

- [项目设计文档](docs/PROJECT_DESIGN.md)
- [实现细节总档](docs/IMPLEMENTATION_DETAILS.md)

## 3. 目录结构

```text
include/                     头文件
src/                         源文件
ui/                          Qt Designer UI
resources/                   图标和 qrc 资源
examples/                    自定义组件示例包
samples/                     示例导出文件
docs/                        项目文档
CMakeLists.txt               CMake 入口
README.md                    项目说明
```

## 4. 构建与运行

### 环境要求

- CMake 3.21+
- C++17 编译器
- Qt 6.9+，模块：
  - `Core`
  - `Gui`
  - `Widgets`
  - `Network`
  - `Svg`
  - `SvgWidgets`
  - `Xml`

### 重要说明

当前 [CMakeLists.txt](CMakeLists.txt) 默认固定使用 `D:/Qt/6.9.3/msvc2022_64`。如果你的 Qt 安装目录不同，请直接修改该路径，或在 CMake 配置时覆盖 `CMAKE_PREFIX_PATH`。

### 参考构建命令

MSVC / Visual Studio：

```powershell
cmake -S . -B out/build-vs -G "Visual Studio 17 2022" -A x64
cmake --build out/build-vs --config Release
```

默认输出程序：

```text
out/build-vs/bin/Release/untitled1.exe
```

### Windows 直接运行说明

项目现在会在 Windows 下自动调用 `windeployqt`，因此正常执行完 `cmake --build ...` 后，`bin/` 目录里的程序应该已经带上所需 Qt 运行时 DLL，可以直接运行，不需要手动到处拷贝依赖。

如果你还需要一份可分发目录，可以执行：

```powershell
cmake --build out/build-vs --config Release --target package-portable
```

输出目录：

```text
out/build-vs/portable/Release/
```

该目录会包含：

- `untitled1.exe`
- Qt 运行时 DLL
- 插件目录
- `docs/`
- `examples/`

## 5. 运行数据目录

项目不会把最近文件、恢复快照、自定义组件等运行数据写入系统用户目录，而是统一写到可执行文件旁边：

```text
<exe-dir>/runtime-data/
```

当前约定：

- `runtime-data/flowchart-editor.ini`
  - 最近文件和设置
- `runtime-data/recovery.json`
  - 崩溃恢复快照
- `runtime-data/components/`
  - 自定义组件定义

## 6. 自定义组件说明

自定义组件不是硬编码子类，而是统一走 JSON 协议：

- `typeId`
- `paletteLabel`
- `defaultSize`
- `drawSpec`
- `ports`
- `style`

`drawSpec` 支持的核心绘制命令：

- `moveTo`
- `lineTo`
- `quadTo`
- `cubicTo`
- `rect`
- `roundedRect`
- `ellipse`
- `polygon`
- `arcMoveTo`
- `arcTo`
- `close`
- `parallelogram`

示例见 [examples/custom-components-pack.json](examples/custom-components-pack.json)。

## 7. 当前限制

- 协同只支持单个对端
- 协同同步粒度是整张文档快照，不是 OT / CRDT
- 跨公网协同需要额外端口映射或 NAT 穿透
- Mermaid 导出当前只覆盖节点和已附着的连接关系，不保留样式细节
- 项目目前没有自动化测试体系

## 8. 适合在简历中描述的关键词

- Qt Widgets
- QGraphicsView / QGraphicsScene
- 文档模型与渲染解耦
- 命令式撤销重做
- 版本化 JSON 文档
- 自定义组件协议
- 正交流程线与交互式弯折
- 本地恢复与最近文件
- P2P 协同同步
