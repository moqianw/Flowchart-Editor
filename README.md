# 流程图编辑器

北京理工大学小学期项目。

## 项目结构

```text
include/        头文件
src/            C++ 源码
ui/             Qt Designer 界面文件
resources/      qrc 与图片资源
samples/        示例导出文件
build/          构建输出目录
CMakeLists.txt  CMake 工程入口
```

## 使用 CMake 构建

前置条件：已安装可被 CMake 识别的 Visual Studio 2022 或 Build Tools。

仓库内的 [CMakeLists.txt](E:/Documents/qt/Flowchart-Editor-master/CMakeLists.txt) 已固定 Qt 安装路径为 `D:/Qt/6.9.3/msvc2022_64`。

1. 配置工程

```powershell
cmake -S . -B build/cmake -G "Visual Studio 17 2022" -A x64
```

2. 构建 Debug 版本

```powershell
cmake --build build/cmake --config Debug
```

3. 运行产物

```powershell
build/cmake/bin/Debug/untitled1.exe
```

根目录仅保留源码入口、文档和 CMake 工程文件，避免源码与构建产物混放。

![屏幕截图 2024-09-07 235836](https://github.com/user-attachments/assets/c1bd663f-eaeb-41f7-a08e-1c4d32913ad7)
![屏幕截图 2024-09-07 235852](https://github.com/user-attachments/assets/dc18c5f8-ce81-47af-afbb-2bfcea24db4b)
