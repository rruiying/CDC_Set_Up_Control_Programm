# CDC控制系统 - 构建指南

## 前置要求

### 必需软件
- **CMake**: 3.16 或更高版本
- **Qt**: 5.15.2 或更高版本（包含Qt3D模块）
- **编译器**:
  - Windows: MSVC 2019/2022
  - Linux: GCC 9.0+
  - macOS: Clang 12+
- **Git**: 用于版本控制

### 可选软件
- **Qt Creator**: IDE（推荐）
- **Visual Studio**: Windows开发
- **CLion**: 跨平台IDE

## 环境配置

### Windows

#### 1. 安装Qt
1. 下载Qt在线安装器：https://www.qt.io/download
2. 运行安装器，选择Qt 5.15.2
3. 确保选择以下组件：
   - Qt 5.15.2 > MSVC 2019 64-bit
   - Qt 5.15.2 > Qt 3D
   - Qt 5.15.2 > Qt Charts
   - Tools > CMake
   - Tools > Qt Creator

#### 2. 安装Visual Studio
1. 下载Visual Studio 2019/2022 Community
2. 安装时选择"使用C++的桌面开发"工作负载

#### 3. 设置环境变量
```powershell
# 添加到系统PATH
C:\Qt\5.15.2\msvc2019_64\bin
C:\Qt\Tools\CMake_64\bin
```

### Linux (Ubuntu/Debian)

#### 1. 安装依赖
```bash
# 更新包管理器
sudo apt-get update

# 安装构建工具
sudo apt-get install build-essential cmake git

# 安装Qt5和Qt3D
sudo apt-get install qt5-default qtbase5-dev
sudo apt-get install qt3d5-dev qt3d-assimpsceneimport-plugin
sudo apt-get install qt3d-defaultgeometryloader-plugin
sudo apt-get install qtserialport5-dev

# 安装额外工具（可选）
sudo apt-get install qtcreator
```

#### 2. 验证安装
```bash
qmake --version
cmake --version
g++ --version
```

### macOS

#### 1. 安装Homebrew
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 2. 安装依赖
```bash
# 安装Qt5
brew install qt@5
brew link qt@5 --force

# 安装CMake
brew install cmake

# 安装编译器（如果需要）
xcode-select --install
```

#### 3. 设置环境变量
```bash
echo 'export PATH="/usr/local/opt/qt@5/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

## 获取源代码

```bash
# 克隆仓库
git clone https://github.com/yourusername/CDC_Control_System.git
cd CDC_Control_System

# 初始化子模块（如果有）
git submodule update --init --recursive
```

## 下载第三方库

### QCustomPlot
```bash
cd third_party
mkdir qcustomplot
cd qcustomplot

# 下载QCustomPlot
wget https://www.qcustomplot.com/release/2.1.1/QCustomPlot.tar.gz
tar -xzf QCustomPlot.tar.gz
cp QCustomPlot/qcustomplot.h .
cp QCustomPlot/qcustomplot.cpp .

cd ../..
```

## 构建步骤

### 使用命令行

#### Debug构建
```bash
# 创建构建目录
mkdir build-debug
cd build-debug

# 配置
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 构建
cmake --build . --parallel 4

# 或使用make（Linux/macOS）
make -j4
```

#### Release构建
```bash
mkdir build-release
cd build-release

# 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build . --config Release --parallel 4
```

### 使用Qt Creator

1. 打开Qt Creator
2. 选择"文件" > "打开文件或项目"
3. 选择项目根目录的`CMakeLists.txt`
4. 配置构建套件（选择Qt 5.15.2）
5. 点击"构建"按钮或按`Ctrl+B`

### 使用Visual Studio (Windows)

```powershell
# 生成VS解决方案
mkdir build-vs
cd build-vs
cmake .. -G "Visual Studio 16 2019" -A x64

# 打开生成的.sln文件
start CDC_Control_System.sln
```

## 构建选项

### CMake选项
```bash
# 禁用测试
cmake .. -DBUILD_TESTS=OFF

# 指定Qt路径
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64"

# 设置安装路径
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/cdc_system

# 启用更多警告
cmake .. -DCMAKE_CXX_FLAGS="-Wall -Wextra"
```

## 运行测试

```bash
# 构建测试
cmake .. -DBUILD_TESTS=ON
cmake --build .

# 运行所有测试
ctest

# 或直接运行测试可执行文件
./bin/CDC_Tests

# 运行特定测试
./bin/CDC_Tests --gtest_filter=DataVisualizationTest.*
```

## 部署

### Windows部署
```powershell
# 使用windeployqt
cd build-release/bin
C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe CDC_Control_System.exe

# 复制额外的DLL（如果需要）
copy C:\Qt\5.15.2\msvc2019_64\bin\Qt53DCore.dll .
copy C:\Qt\5.15.2\msvc2019_64\bin\Qt53DRender.dll .
```

### Linux部署
```bash
# 创建AppImage（推荐）
# 安装linuxdeployqt
wget https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod +x linuxdeployqt-continuous-x86_64.AppImage

# 部署
./linuxdeployqt-continuous-x86_64.AppImage CDC_Control_System -appimage
```

### macOS部署
```bash
# 使用macdeployqt
/usr/local/opt/qt@5/bin/macdeployqt CDC_Control_System.app -dmg
```

## 故障排除

### 常见问题

#### 1. CMake找不到Qt
```bash
# 指定Qt路径
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt5
```

#### 2. Qt3D模块未找到
```bash
# 确保安装了Qt3D
# Ubuntu/Debian:
sudo apt-get install qt3d5-dev

# 或重新安装Qt，确保选择Qt3D组件
```

#### 3. 链接错误
```bash
# 清理构建目录
rm -rf build/*
# 重新配置和构建
cmake ..
make clean
make
```

#### 4. QCustomPlot编译错误
确保：
- QCustomPlot文件在正确位置
- CMakeLists.txt中启用了AUTOMOC
- Qt PrintSupport模块已安装

## 开发环境设置

### VS Code配置
创建`.vscode/settings.json`:
```json
{
    "cmake.sourceDirectory": "${workspaceFolder}",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.configureSettings": {
        "CMAKE_PREFIX_PATH": "/path/to/qt5"
    }
}
```

### CLion配置
1. 打开项目
2. 设置 > Build, Execution, Deployment > CMake
3. 添加CMake配置文件
4. 设置Qt路径

## 持续集成

### GitHub Actions示例
```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install Qt
      uses: jurplel/install-qt-action@v2
      with:
        version: '5.15.2'
        modules: 'qt3d qtserialport'
    
    - name: Configure
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release
    
    - name: Build
      run: cmake --build build --parallel
    
    - name: Test
      run: cd build && ctest
```

## 性能优化

### 编译优化
```bash
# Release模式优化
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"

# Link Time Optimization
cmake .. -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### 调试构建
```bash
# 启用调试符号和地址消毒器
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-g -fsanitize=address"
```

## 版本信息

查看构建版本：
```bash
./CDC_Control_System --version
```