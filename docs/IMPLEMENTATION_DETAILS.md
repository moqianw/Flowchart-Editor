# 实现细节总档

本文档按“入口 -> 模块 -> 协议 -> 关键交互 -> 文件职责”的顺序，记录当前项目已经实现的细节，方便交接、二次开发和简历答辩。

## 1. 技术栈与工程入口

### 1.1 技术栈

- C++17
- Qt 6 Widgets
- QGraphicsScene / QGraphicsView
- QUndoStack
- JSON 持久化
- Qt Network TCP 直连

### 1.2 工程入口

- `CMakeLists.txt`
  - 定义 Qt 模块依赖
  - 开启 `AUTOMOC` / `AUTOUIC` / `AUTORCC`
  - 定义目标程序 `FlowchartEditor`
  - 输出名固定为 `untitled1`

- `src/main.cpp`
  - 创建 `QApplication`
  - 设置应用名和组织名
  - 将 `QSettings` 指向本地 `runtime-data`
  - 创建并显示 `MainWindow`

## 2. 模块职责

## 2.1 文档模型

### `include/diagramtypes.h` / `src/diagramtypes.cpp`

负责最底层的数据结构：

- `ItemKind`
- `ItemStyle`
- `EndpointRef`
- `DiagramItemModel`
- `NodeModel`
- `ConnectorModel`
- `createItemId()`
- `modelsEqual()`
- `cloneModels()`

实现细节：

- `ItemStyle` 承载公共样式
- `NodeModel` 和 `ConnectorModel` 都继承自 `DiagramItemModel`
- 克隆能力是命令系统和复制粘贴的基础

### `include/diagramdocument.h` / `src/diagramdocument.cpp`

负责内存中的文档容器。

能力：

- `contains`
- `item / node / connector`
- `upsert`
- `take`
- `itemIds`
- `connectorsForNode`
- `cloneItem / cloneItems / cloneAll`

实现方式：

- 使用 `std::map<QString, std::unique_ptr<DiagramItemModel>>`
- 以 `id` 为唯一键

## 2.2 编辑会话

### `include/editorsession.h` / `src/editorsession.cpp`

这是项目的中枢模块。

它负责：

- 维护 `DiagramDocument`
- 维护 `QGraphicsScene`
- 维护 `QUndoStack`
- 创建节点
- 创建连接线
- 复制 / 粘贴 / 剪切 / 删除
- 字体、颜色、线宽等样式修改
- 节点几何变更
- 连接线路由和弯折
- 自动布局
- 文档校验
- 网格吸附
- 场景视图对象与模型同步
- 序列化读写
- 协同快照应用

### `EditorSession` 关键成员

- `ShapeRegistry m_registry`
- `DiagramDocument m_document`
- `QGraphicsScene* m_scene`
- `QUndoStack* m_undoStack`
- `QHash<QString, QGraphicsItem*> m_viewItems`
- `std::vector<std::unique_ptr<DiagramItemModel>> m_clipboard`
- `std::optional<PendingInteraction> m_pendingInteraction`

### 节点创建

`createItem(typeId, scenePos)` 过程：

1. 从 `ShapeRegistry` 取定义
2. 创建 `NodeModel`
3. 根据默认尺寸和网格吸附计算 `rect`
4. 初始化默认属性与默认样式
5. 压入 `AddItemsCommand`

### 连接线创建

`createConnector(scenePos)` 过程：

1. 创建 `ConnectorModel`
2. `typeId = ArrowConnector`
3. 默认起点为 `scenePos`
4. 默认终点为 `scenePos + (160, 0)`
5. 默认 `props`
   - `headStyle = stealth`
   - `lineStyle = solid`
6. 压入 `AddItemsCommand`

### 连接线路由

当前实现支持三种状态：

- 自动路由
  - 删除 `bendX` / `bendY`
- 纵向折线
  - 设置 `bendX`
- 横向折线
  - 设置 `bendY`

### 自动布局

`autoLayoutSelection()` 实现思路：

1. 取当前选中节点；若未选中则取全部节点
2. 根据附着连接线建立 DAG 近似结构
3. 统计每个节点入度
4. 通过拓扑顺序估算层级
5. 对有环或剩余节点给出 fallback layer
6. 逐层按列排布，层内按行排布
7. 最终一次性提交为 `UpdateItemsCommand`

当前布局参数：

- 起点 `80, 80`
- 列间距 `120`
- 行间距 `80`

### 文档校验

`validateDocument()` 会检查：

- 是否存在节点
- 是否存在开始/结束节点
- 是否存在未完整连接的连线
- 是否存在自环
- 是否存在孤立节点
- 入口节点个数
- 出口节点个数
- 是否存在环路

### 网格吸附

当前配置：

- 开启状态默认 `true`
- 网格大小默认 `20`

吸附对象：

- 节点位置
- 节点矩形
- 自由连接线端点
- 手动弯折点的 `bendX` / `bendY`

## 2.3 命令系统

### `include/commands.h` / `src/commands.cpp`

已实现命令：

- `AddItemsCommand`
- `RemoveItemsCommand`
- `UpdateItemsCommand`

实现原则：

- 命令存储模型快照，而不是 UI 对象
- `undo` / `redo` 最终都回到 `EditorSession` 的内部插入、删除、替换接口

## 2.4 图元渲染

### `include/canvasitems.h` / `src/canvasitems.cpp`

包含两个核心对象：

- `CanvasNodeItem`
- `CanvasConnectorItem`

### `CanvasNodeItem`

负责：

- 根据 `ShapeDefinition` 绘制节点
- 显示文本
- 8 向缩放手柄
- 拖动节点
- 双击进入文本编辑
- hover 时更新鼠标形状
- 与 `EditorSession` 的交互式几何编辑联动

重要实现点：

- 文本子项是 `EditableTextItem`
- 双击进入 `TextEditorInteraction`
- 失焦后提交文本变更

### `CanvasConnectorItem`

负责：

- 根据端点和弯折属性重建路径
- 绘制连接线与箭头头部
- 提供起点、终点、弯折控制点
- 拖拽时调用 `EditorSession` 的交互式接口

### 连接线绘制细节

#### 路径生成

`connectorSegments(start, end, bendX, bendY)`：

- 有 `bendX` 时，生成一条纵向折线
- 有 `bendY` 时，生成一条横向折线
- 横平竖直时直接连线
- 默认情况下取中间列形成正交路径

#### 箭头头型

`connectorHeadPath(...)` 当前支持：

- `classic`
- `stealth`
- `diamond`
- `circle`

#### 线型

`connectorLineStyle(props)` 当前支持：

- `solid`
- `dash`
- `dot`

#### 选中高亮

连接线被选中时：

- 绘制额外 glow 描边
- 显示起点/终点控制点
- 如存在弯折控制点则额外显示

## 2.5 画布视图

### `include/canvasview.h` / `src/canvasview.cpp`

负责：

- 画布背景网格
- 拖放创建节点
- 右键菜单
- 导出 PNG / SVG
- 视图缩放与旋转
- 转发 `viewport()` 上的拖放事件

## 2.6 图元注册表

### `include/shaperegistry.h` / `src/shaperegistry.cpp`

这是“任意造型组件”设计的核心。

### 内置组件

当前内置：

- `Ellipse`
- `Rectangle`
- `Diamond`
- `RoundedRectangle`
- `Parallelogram`
- `Start_or_Terminator`
- `Subprocess`
- `Database`
- `Document`
- `DataStorage`
- `Textpointer`

### 自定义组件加载

`ShapeRegistry::reload()` 流程：

1. 清空现有定义
2. 注册内置组件
3. 读取组件目录中的 JSON
4. 转换为 `ShapeDefinition`
5. 校验并加入索引

### `drawSpec` 协议

支持命令：

- `moveTo`
- `lineTo`
- `quadTo`
- `cubicTo`
- `rect`
- `ellipse`
- `roundedRect`
- `polygon`
- `arcMoveTo`
- `arcTo`
- `close`
- `parallelogram`

坐标规则：

- 默认按归一化比例解释
- 也可通过 `normalized` 字段改成绝对值

### `portsSpec` 协议

支持：

- `fourWay`
- `twoVertical`
- `twoHorizontal`
- `none`
- `custom + points`

### 旧格式兼容

`shape.kind` 旧定义会在加载时转换为 `drawSpec`，目前兼容：

- `ellipse`
- `roundedRect`
- `diamond`
- `parallelogram`
- `hexagon`
- `terminator`
- `document`
- `polygon`

## 2.7 组件扩展管理

### `include/componentextensionmanager.h` / `src/componentextensionmanager.cpp`

负责：

- 本地组件目录路径
- 组件包导入
- 组件单文件保存
- 组件删除
- 加载已安装组件

当前支持三种文件入口：

- 根对象直接是单个组件
- `{ "component": {...} }`
- `{ "components": [ ... ] }`

### 校验规则

至少要求：

- `typeId` 非空
- `drawSpec.commands` 非空

### 存储位置

组件目录：

- `<exe-dir>/runtime-data/components/`

单组件文件名由 `typeId` 规范化得到。

## 2.8 自定义组件创建对话框

### `include/customcomponentdialog.h` / `src/customcomponentdialog.cpp`

当前对话框支持：

- 组件 ID
- 显示名称
- 模板选择
- 默认宽高
- 线宽
- 边框颜色
- 填充颜色
- `drawSpec` JSON 编辑
- `ports` JSON 编辑

内置说明文字已经明确了：

- 支持的 `drawSpec.op`
- `ports` 的写法

## 2.9 文档序列化

### `include/documentserializer.h` / `src/documentserializer.cpp`

文档根结构：

```json
{
  "version": 1,
  "items": [ ... ]
}
```

### 节点序列化字段

- `id`
- `typeId`
- `kind = node`
- `text`
- `zValue`
- `style`
- `rect`
- `props`

### 连接线序列化字段

- `id`
- `typeId`
- `kind = connector`
- `text`
- `zValue`
- `style`
- `start`
- `end`
- `props`

### `style` 结构

```json
{
  "strokeColor": { "r": 0, "g": 0, "b": 0, "a": 255 },
  "fillColor": { "r": 255, "g": 255, "b": 255, "a": 0 },
  "strokeWidth": 2,
  "font": "...",
  "bold": false,
  "italic": false,
  "underline": false,
  "rotation": 0.0,
  "scale": 1.0
}
```

### `start` / `end` 结构

```json
{
  "itemId": "node-id",
  "portIndex": 0,
  "position": { "x": 100.0, "y": 120.0 }
}
```

## 2.10 Mermaid 导出

### `include/mermaidexporter.h` / `src/mermaidexporter.cpp`

当前导出逻辑：

- 根图类型固定 `flowchart TD`
- 节点按 `itemId -> n1/n2/...` 重映射
- 文本优先取节点文本，若为空则取 `typeId`
- 已附着的连接线导出为 `-->`

当前仅对部分节点类型映射特定 Mermaid 形状：

- `Diamond`
- `Start_or_Terminator`
- `Ellipse`

其他节点统一导出为矩形。

## 2.11 本地运行数据

### `include/runtimepaths.h` / `src/runtimepaths.cpp`

统一运行目录设计：

- `executableDirectory()`
- `runtimeDataDirectory()`
- `settingsFilePath()`
- `recoveryFilePath()`
- `componentsDirectory()`

运行数据全部放在：

```text
<exe-dir>/runtime-data/
```

### `include/sessionpersistence.h` / `src/sessionpersistence.cpp`

负责：

- 最近文件
- 恢复快照

#### 最近文件

- INI key: `recentFiles`
- 默认最多保留 8 条
- 自动去重
- 自动过滤不存在文件

#### 恢复快照

结构：

```json
{
  "version": 1,
  "sourceFile": "xxx.json",
  "savedAt": "2026-03-16T12:00:00Z",
  "document": { ... }
}
```

## 2.12 主窗口

### `include/mainwindow.h` / `src/mainwindow.cpp`

主窗口负责所有桌面级交互整合：

- 欢迎页
- 菜单栏
- 工具栏
- 属性面板
- 组件管理面板
- 联机房间面板
- 最近文件菜单
- 文件导出菜单
- 自动保存定时器

### 欢迎页

欢迎页包括：

- 标题
- 新建文件
- 打开文件
- 最近文件列表
- 空状态提示

### 属性面板

当前字段：

- 类型
- ID
- 文本
- X
- Y
- 宽度
- 高度
- 旋转
- 缩放
- 填充透明度
- 线宽

### 组件管理面板

功能：

- 新建
- 导入
- 刷新
- 打开目录
- 直接编辑组件 JSON
- 保存修改
- 导出
- 删除

### 联机房间面板

字段：

- 昵称
- 房主 IP
- 房间端口
- 房间状态
- 已连接成员

按钮：

- 房主开房
- 加入房间
- 退出房间

### 自动保存与恢复

当前策略：

- 定时器间隔 `15000 ms`
- 文档 dirty 时写恢复快照
- 正常保存、正常关闭、关闭文档时清除恢复快照
- 启动时检测恢复文件并提示恢复

## 2.13 P2P 协同

### `include/peercollaborationmanager.h` / `src/peercollaborationmanager.cpp`

协同基于 `QTcpServer + QTcpSocket` 实现，当前是简单直连模式。

### 当前能力

- 房主监听端口
- 客户端连接房主
- 单对端限制
- 连接建立后互发 `hello`
- 房主发送初始文档快照
- 本地文档变更后延迟发送快照
- 对端收到快照后替换当前文档

### 关键状态

- `isHosting`
- `isConnected`
- `isConnecting`
- `remoteDisplayName`
- `statusText`

### 协议消息：`hello`

```json
{
  "type": "hello",
  "protocol": 1,
  "peerId": "uuid",
  "displayName": "Peer",
  "sentAt": "..."
}
```

### 协议消息：`snapshot`

```json
{
  "type": "snapshot",
  "protocol": 1,
  "peerId": "uuid",
  "displayName": "Peer",
  "revision": "1",
  "reason": "document_changed",
  "document": { ... }
}
```

### 协同一致性策略

- 同步粒度：整张文档
- 合并策略：后到快照覆盖先前文档
- 本地防抖：`160 ms`
- 不支持 OT
- 不支持 CRDT
- 不支持多对端

## 3. 自定义组件 JSON 细节

示例结构：

```json
{
  "typeId": "CustomHexagonNode",
  "paletteLabel": "六边形节点",
  "defaultSize": { "w": 160, "h": 100 },
  "drawSpec": {
    "commands": [
      {
        "op": "polygon",
        "points": [
          { "x": 0.25, "y": 0.0 },
          { "x": 0.75, "y": 0.0 },
          { "x": 1.0, "y": 0.5 },
          { "x": 0.75, "y": 1.0 },
          { "x": 0.25, "y": 1.0 },
          { "x": 0.0, "y": 0.5 }
        ]
      }
    ]
  },
  "ports": {
    "kind": "fourWay"
  },
  "style": {
    "strokeWidth": 2,
    "strokeColor": { "r": 29, "g": 58, "b": 84, "a": 255 },
    "fillColor": { "r": 176, "g": 220, "b": 255, "a": 180 }
  }
}
```

## 4. UI 与状态流转

## 4.1 启动流程

1. `main.cpp` 创建 `MainWindow`
2. `MainWindow` 初始化 palette、dock、欢迎页、持久化
3. 尝试恢复崩溃快照
4. 若无活动文档则停留在欢迎页

## 4.2 新建文档

1. `maybeSave()`
2. 清空当前文件路径
3. `EditorSession::resetDocument()`
4. 清理恢复快照
5. 切换到画布页

## 4.3 打开文档

1. `maybeSave()`
2. `EditorSession::loadFromFile()`
3. 记录最近文件
4. 清理恢复快照
5. 切换到画布页

## 4.4 协同快照进入

1. 收到远端 `snapshot`
2. `DocumentSerializer::fromJsonObject()`
3. `EditorSession::applyRemoteSnapshot()`
4. 当前主窗口切换到活动文档状态
5. 当前文件路径清空

## 5. 文件级职责总表

### 根目录

- `CMakeLists.txt`
  - 构建入口
- `README.md`
  - 项目说明

### `include/`

- `commands.h`
  - undo/redo 命令声明
- `canvasitems.h`
  - 节点/连接线视图声明
- `canvasview.h`
  - 画布视图声明
- `componentextensionmanager.h`
  - 组件扩展管理接口
- `customcomponentdialog.h`
  - 自定义组件对话框
- `diagramdocument.h`
  - 文档容器
- `diagramtypes.h`
  - 基础数据模型
- `documentserializer.h`
  - 文档序列化
- `editorsession.h`
  - 编辑会话核心
- `mainwindow.h`
  - 主窗口
- `mermaidexporter.h`
  - Mermaid 导出
- `palettebutton.h`
  - 图形栏按钮
- `peercollaborationmanager.h`
  - P2P 协同管理器
- `runtimepaths.h`
  - 本地运行目录
- `sessionpersistence.h`
  - 最近文件与恢复
- `shaperegistry.h`
  - 图元注册表

### `src/`

- `commands.cpp`
  - undo/redo 实现
- `canvasitems.cpp`
  - 节点与连接线绘制、交互
- `canvasview.cpp`
  - 画布背景、拖放、导出
- `componentextensionmanager.cpp`
  - 组件包读写
- `customcomponentdialog.cpp`
  - 自定义组件编辑界面
- `diagramdocument.cpp`
  - 文档容器实现
- `diagramtypes.cpp`
  - 模型辅助实现
- `documentserializer.cpp`
  - JSON 文档编解码
- `editorsession.cpp`
  - 业务核心
- `main.cpp`
  - 应用入口
- `mainwindow.cpp`
  - 桌面 UI 组装
- `mermaidexporter.cpp`
  - Mermaid 导出实现
- `palettebutton.cpp`
  - 图形栏按钮拖拽逻辑
- `peercollaborationmanager.cpp`
  - TCP P2P 协同
- `runtimepaths.cpp`
  - 本地运行路径
- `sessionpersistence.cpp`
  - 最近文件和恢复实现
- `shaperegistry.cpp`
  - 内置图元与自定义图元注册

### `ui/`

- `mainwindow.ui`
  - 主窗口基础布局和工具栏定义

### `examples/`

- `custom-components-pack.json`
  - 自定义组件扩展包示例

### `resources/`

- `img.qrc`
  - 资源清单
- `image/*`
  - 图标资源

## 6. 当前已知限制

- 无自动化测试
- 协同只支持单对端
- 协同是整文档快照同步
- 连接线没有避障路由
- Mermaid 只导出基础结构，不导出样式
- 组件扩展目前仍是 JSON 驱动，不是二进制插件 ABI

## 7. 适合二次开发的入口点

如果后续继续开发，优先从下面几个入口切：

- 新增节点协议或绘制命令
  - `src/shaperegistry.cpp`
- 增加连接线样式 / 路由策略
  - `src/canvasitems.cpp`
  - `src/editorsession.cpp`
- 增加导出格式
  - `src/documentserializer.cpp`
  - `src/mermaidexporter.cpp`
- 增强协同协议
  - `src/peercollaborationmanager.cpp`
- 增加产品化能力
  - `src/mainwindow.cpp`
