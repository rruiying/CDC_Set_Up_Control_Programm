# CDC Control System - 电容位移控制系统

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Qt](https://img.shields.io/badge/Qt-5.15.2-green)
![C++](https://img.shields.io/badge/C++-17-orange)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

## 📋 项目简介

CDC控制系统是一个用于精密电容测量与控制的上位机软件，主要用于控制平行板电容器的高度和角度，实时监测电容值变化，并进行数据分析和可视化。

### 主要功能

- 🔌 **串口通信管理** - 支持多设备连接与管理
- 🎯 **精密电机控制** - Z轴高度和角度精确控制
- 📊 **实时传感器监控** - 多传感器数据实时采集
- 💾 **数据记录与导出** - CSV格式数据导出
- 📈 **3D数据可视化** - 支持2D/3D图表分析

## 🛠️ 技术栈

- **开发语言**: C++17
- **UI框架**: Qt 5.15.2
- **图表库**: QCustomPlot 2.1.1 + Qt3D
- **构建系统**: CMake 3.16+
- **串口通信**: Qt SerialPort
- **测试框架**: Google Test (可选)

## 📁 项目结构

```
CDC_Control_System/
├── src/                    # 源代码
│   ├── app/               # 应用程序模块
│   ├── core/              # 核心业务逻辑
│   ├── data/              # 数据处理模块
│   ├── hardware/          # 硬件接口层
│   ├── models/            # 数据模型
│   └── utils/             # 工具类
├── ui/                     # 用户界面
│   ├── forms/             # UI文件
│   ├── include/           # 头文件
│   └── src/               # 实现文件
├── third_party/           # 第三方库
│   └── qcustomplot/       # 图表库
├── tests/                 # 单元测试
├── docs/                  # 文档
└── runtime/               # 运行时配置
```

## 🚀 快速开始

### 系统要求

- **操作系统**: Windows 10/11, Ubuntu 20.04+, macOS 10.15+
- **Qt版本**: 5.15.2 或更高
- **CMake**: 3.16 或更高
- **编译器**: 
  - Windows: MSVC 2019/2022
  - Linux: GCC 9.0+
  - macOS: Clang 12+

### 安装步骤

1. **克隆仓库**
```bash
git clone https://github.com/yourusername/CDC_Control_System.git
cd CDC_Control_System
```

2. **安装依赖**

**Windows:**
```bash
# 使用Qt在线安装器安装Qt 5.15.2
# 确保选择Qt3D组件
```

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install qt5-default qtbase5-dev qt3d5-dev
sudo apt-get install build-essential cmake
```

**macOS:**
```bash
brew install qt@5 cmake
brew link qt@5 --force
```

3. **下载QCustomPlot**
```bash
cd third_party/qcustomplot
wget https://www.qcustomplot.com/release/2.1.1/QCustomPlot.tar.gz
tar -xzf QCustomPlot.tar.gz
cp QCustomPlot/qcustomplot.* .
```

4. **构建项目**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel 4
```

5. **运行程序**
```bash
./bin/CDC_Control_System
```

## 💻 使用说明

### 1. 设备连接
- 打开软件，进入"Serial Communication"页面
- 点击"Add Device"添加串口设备
- 选择正确的COM口和波特率（默认115200）
- 点击"Connect"连接设备

### 2. 电机控制
- 切换到"Motor Control"页面
- 设置目标高度（0-100mm）和角度（-90°到90°）
- 点击"Move to Position"执行移动
- 使用"Emergency Stop"紧急停止

### 3. 数据记录
- 在"Sensor Monitor"页面查看实时数据
- 点击"Record Data"记录当前数据点
- 使用"Export"导出为CSV文件

### 4. 数据可视化
- 在"Data Visualization"页面加载CSV文件
- 选择图表类型：
  - 距离-电容分析
  - 角度-电容分析
  - 3D距离-角度-电容图
  - 误差分析
- 支持缩放、平移等交互操作

## 🔧 硬件配置

### 支持的硬件
- **主控芯片**: PSoC 4500S
- **电机驱动**: TMC2208-LA
- **距离传感器**: DFRobot SEN0427 (×4)
- **角度传感器**: Murata SCA830-D07-1
- **温度传感器**: DS18B20+
- **伺服电机**: SER0019

### 通信协议
- 波特率: 9600/19200/38400/57600/115200 (可选)
- 数据位: 8
- 停止位: 1
- 校验: 无

## 📊 数据格式

### CSV导出格式
```csv
Timestamp,Set_Height(mm),Set_Angle(deg),Theoretical_Capacitance(pF),
Upper_Sensor_1(mm),Upper_Sensor_2(mm),Lower_Sensor_1(mm),Lower_Sensor_2(mm),
Temperature(C),Measured_Angle(deg),Measured_Capacitance(pF),...
```

## 🐛 故障排除

### 常见问题

1. **无法连接设备**
   - 检查串口是否被占用
   - 确认波特率设置正确
   - 检查USB驱动是否安装

2. **Qt3D无法显示**
   - 确保安装了Qt3D模块
   - 检查显卡驱动是否支持OpenGL 3.3+

3. **编译错误**
   - 确认Qt版本 >= 5.15.2
   - 检查CMake版本 >= 3.16
   - 确保所有依赖库已安装

## 📝 开发文档

- [架构设计](docs/developer/architecture.md)
- [构建指南](docs/developer/build_guide.md)
- [API文档](docs/api/index.html)

## 🤝 贡献指南

欢迎提交Issue和Pull Request！

1. Fork本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

## 👥 团队

- **开发团队**: TUM LSE实验室
- **项目负责人**: RuiYing
- **联系方式**: [your.email@tum.de]

## 🙏 致谢

- Qt Framework
- QCustomPlot
- Google Test
- 所有贡献者

---

*最后更新: 2025年1月*