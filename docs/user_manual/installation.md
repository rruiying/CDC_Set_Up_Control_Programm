# CDC控制系统 - 安装指南

## 系统要求

### 最低配置
- **处理器**: 双核 2.0 GHz
- **内存**: 4 GB RAM
- **硬盘**: 500 MB可用空间
- **显卡**: 支持OpenGL 3.3
- **显示器**: 1280×720分辨率

### 推荐配置
- **处理器**: 四核 2.5 GHz或更高
- **内存**: 8 GB RAM或更多
- **硬盘**: 1 GB可用空间（用于数据存储）
- **显卡**: 独立显卡，支持OpenGL 4.0+
- **显示器**: 1920×1080分辨率或更高

### 操作系统支持
- Windows 10/11 (64位)
- Ubuntu 20.04 LTS或更高版本
- macOS 10.15 Catalina或更高版本

## Windows安装

### 方法1：使用安装程序（推荐）

1. **下载安装程序**
   - 从发布页面下载`CDC_Control_System_Setup.exe`

2. **运行安装程序**
   - 双击运行安装程序
   - 如果出现Windows防护提示，选择"更多信息"→"仍要运行"

3. **安装向导**
   - 选择安装语言
   - 接受许可协议
   - 选择安装路径（默认：`C:\Program Files\CDC Control System`）
   - 选择组件：
     - ✅ 主程序（必需）
     - ✅ 示例数据
     - ✅ 设备驱动
     - ✅ 开始菜单快捷方式
     - ✅ 桌面快捷方式

4. **安装驱动**
   - 安装程序会自动安装USB串口驱动
   - 如果提示，允许驱动安装

5. **完成安装**
   - 点击"完成"
   - 可选择立即启动程序

### 方法2：便携版

1. **下载便携版**
   - 下载`CDC_Control_System_Portable.zip`

2. **解压文件**
   - 解压到任意目录（如`D:\CDC_System`）

3. **运行程序**
   - 进入解压目录
   - 双击`CDC_Control_System.exe`

4. **安装驱动（如需要）**
   - 运行`drivers\install_driver.bat`（管理员权限）

## Linux安装

### Ubuntu/Debian

#### 使用DEB包
```bash
# 下载deb包
wget https://github.com/yourusername/CDC_Control_System/releases/download/v1.0.0/cdc-control-system_1.0.0_amd64.deb

# 安装
sudo dpkg -i cdc-control-system_1.0.0_amd64.deb

# 修复依赖（如果需要）
sudo apt-get install -f
```

#### 使用AppImage
```bash
# 下载AppImage
wget https://github.com/yourusername/CDC_Control_System/releases/download/v1.0.0/CDC_Control_System-x86_64.AppImage

# 添加执行权限
chmod +x CDC_Control_System-x86_64.AppImage

# 运行
./CDC_Control_System-x86_64.AppImage
```

#### 从源码安装
```bash
# 安装依赖
sudo apt-get update
sudo apt-get install qt5-default qt3d5-dev qtserialport5-dev build-essential cmake git

# 克隆仓库
git clone https://github.com/yourusername/CDC_Control_System.git
cd CDC_Control_System

# 下载QCustomPlot
cd third_party/qcustomplot
wget https://www.qcustomplot.com/release/2.1.1/QCustomPlot.tar.gz
tar -xzf QCustomPlot.tar.gz
cp QCustomPlot/qcustomplot.* .
cd ../..

# 构建
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4

# 安装
sudo make install
```

### 串口权限设置
```bash
# 添加用户到dialout组
sudo usermod -a -G dialout $USER

# 创建udev规则
echo 'SUBSYSTEM=="tty", GROUP="dialout", MODE="0666"' | sudo tee /etc/udev/rules.d/99-serial.rules

# 重新加载规则
sudo udevadm control --reload-rules
sudo udevadm trigger

# 注销并重新登录使更改生效
```

## macOS安装

### 使用DMG安装包

1. **下载DMG文件**
   - 下载`CDC_Control_System.dmg`

2. **安装**
   - 双击DMG文件
   - 将CDC Control System拖到Applications文件夹
   - 弹出DMG

3. **首次运行**
   - 打开Finder → Applications
   - 右键点击CDC Control System
   - 选择"打开"（绕过Gatekeeper）

### 使用Homebrew
```bash
# 添加tap
brew tap yourusername/cdc-system

# 安装
brew install cdc-control-system

# 运行
cdc-control-system
```

### 串口权限
macOS通常不需要额外配置串口权限，但可能需要安装驱动：
- CH340/CH341: https://www.wch.cn/download/CH341SER_MAC_ZIP.html
- CP210x: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- FTDI: https://ftdichip.com/drivers/vcp-drivers/

## 验证安装

### 1. 启动程序
- Windows: 开始菜单 → CDC Control System
- Linux: 终端输入`cdc-control-system`或点击应用图标
- macOS: Launchpad或Applications文件夹

### 2. 检查版本
- 帮助 → 关于
- 应显示版本号：v1.0.0

### 3. 测试串口
- 打开"Serial Communication"页面
- 点击"Add Device"
- 应能看到可用的COM端口

### 4. 测试3D显示
- 打开"Data Visualization"页面
- 选择"3D Distance-Angle-Capacitance"
- 应能看到3D坐标轴

## 故障排除

### Windows问题

#### 程序无法启动
- 错误："缺少MSVCP140.dll"
  - 解决：安装Visual C++ Redistributable
  ```
  https://aka.ms/vs/17/release/vc_redist.x64.exe
  ```

#### 找不到串口
- 检查设备管理器中的COM端口
- 重新安装USB驱动
- 尝试不同的USB端口

#### 3D显示问题
- 更新显卡驱动
- 检查OpenGL支持：
  ```powershell
  # 下载GPU-Z查看OpenGL版本
  https://www.techpowerup.com/gpuz/
  ```

### Linux问题

#### 缺少依赖
```bash
# 查看缺少的库
ldd CDC_Control_System | grep "not found"

# 安装缺少的Qt库
sudo apt-get install libqt5core5a libqt5gui5 libqt5widgets5
sudo apt-get install libqt53dcore5 libqt53drender5
```

#### 串口权限被拒绝
```bash
# 检查用户组
groups

# 如果没有dialout，添加并重新登录
sudo usermod -a -G dialout $USER
```

### macOS问题

#### "无法打开，因为它来自身份不明的开发者"
- 系统偏好设置 → 安全性与隐私
- 点击"仍要打开"

#### 串口未显示
- 检查系统信息 → USB
- 安装相应的驱动程序

## 卸载

### Windows
- 控制面板 → 程序和功能
- 选择"CDC Control System"
- 点击"卸载"

### Linux
```bash
# 如果使用dpkg安装
sudo dpkg -r cdc-control-system

# 如果使用make install
sudo make uninstall

# 删除配置文件
rm -rf ~/.config/CDC_Control_System
```

### macOS
- 将CDC Control System从Applications拖到垃圾桶
- 删除配置文件：
  ```bash
  rm -rf ~/Library/Preferences/com.tumse.cdc-control-system.plist
  rm -rf ~/Library/Application\ Support/CDC\ Control\ System
  ```

## 配置文件位置

- **Windows**: `%APPDATA%\CDC Control System\`
- **Linux**: `~/.config/CDC_Control_System/`
- **macOS**: `~/Library/Application Support/CDC Control System/`

## 日志文件位置

- **Windows**: `%LOCALAPPDATA%\CDC Control System\logs\`
- **Linux**: `~/.local/share/CDC_Control_System/logs/`
- **macOS**: `~/Library/Logs/CDC Control System/`

## 获取帮助

如果遇到问题：
1. 查看[用户指南](user_guide.md)
2. 查看[FAQ](../faq.md)
3. 提交Issue: https://github.com/yourusername/CDC_Control_System/issues
4. 联系支持: support@example.com