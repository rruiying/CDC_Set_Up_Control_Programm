# CDC Control System - Capacitive Displacement Control System

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Qt](https://img.shields.io/badge/Qt-6.9.1-green)
![C++](https://img.shields.io/badge/C++-17-orange)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

## 📋 Project Overview

CDC Control System is a precision capacitive measurement and control software designed for controlling the height and angle of parallel plate capacitors, real-time monitoring of capacitance changes, and data acquisition.

### Key Features

- 🔌 **Serial Communication Management** - Multi-device connection and management
- 🎯 **Precision Motor Control** - Z-axis height and angle control
- 📊 **Real-time Sensor Monitoring** - Multi-sensor data acquisition
- 💾 **Data Recording & Export** - CSV format data export
- ⚙️ **Hardware Integration** - PSoC and sensor integration

## 🛠️ Technology Stack

- **Programming Language**: C++17
- **UI Framework**: Qt 6.9.1
- **Build System**: CMake 3.16+
- **Compiler**: Visual Studio 2022 (MSVC)
- **Serial Communication**: Qt SerialPort
- **Testing Framework**: Google Test (optional)

## 📁 Project Structure

```
CDC_Control_System/
├── src/                    # Source code
│   ├── app/               # Application modules
│   │   ├── include/       # Application headers
│   │   └── src/           # Application implementation
│   ├── core/              # Core business logic
│   │   ├── include/       # Core headers (motor, sensor, safety)
│   │   └── src/           # Core implementation
│   ├── data/              # Data processing modules
│   │   ├── include/       # Data processing headers
│   │   └── src/           # Data implementation (CSV, export)
│   ├── hardware/          # Hardware interface layer
│   │   ├── include/       # Hardware interface headers
│   │   └── src/           # Serial/sensor communication
│   ├── models/            # Data models
│   │   ├── include/       # Model headers (physics, config)
│   │   └── src/           # Model implementation
│   └── utils/             # Utility classes
│       ├── include/       # Utility headers (logger, math)
│       └── src/           # Utility implementation
├── ui/                     # User interface
│   ├── forms/             # Qt UI files (.ui)
│   ├── include/           # UI header files
│   └── src/               # UI implementation files
├── tests/                 # Unit tests
│   ├── core_tests/        # Core module tests
│   ├── data_tests/        # Data processing tests
│   ├── hardware_tests/    # Hardware interface tests
│   ├── models_tests/      # Model tests
│   └── utils_tests/       # Utility tests
├── docs/                  # Documentation
│   ├── developer/         # Developer documentation
│   └── user_manual/       # User manual
├── resources/             # Application resources
│   ├── config/            # Configuration files
│   ├── icons/             # Application icons
│   └── styles/            # UI stylesheets
└── deploy/                # Deployment configuration
    ├── package/           # Deployment packages
    │   └── qt.conf        # Qt configuration file
    ├── windows/           # Windows-specific deployment
    │   ├── app.ico        # Application icon
    │   └── deploy_windows.bat # Deployment script
    └── macos/             # macOS-specific deployment
        └── deploy_macos.sh # Deployment script
```

## 🚀 Getting Started

### System Requirements

- **Primary Platform**: Windows 10/11 
- **Secondary Platform**: macOS 10.15+ (experimental)
- **Qt Version**: 6.9.1 or higher
- **CMake**: 3.16 or higher
- **Compilers**: 
  - Windows: Visual Studio 2022 (MSVC)
  - macOS: Xcode 12+ (Clang)

### Installation Steps

1. **Clone Repository**
```bash
git clone https://github.com/yourusername/CDC_Control_System.git
cd CDC_Control_System
```

2. **Install Qt 6.9.1**

**Windows:**
- Download and install [Qt 6.9.1 for MSVC 2022 64-bit](https://www.qt.io/download)
- Ensure the following components are selected:
  - Qt 6.9.1 MSVC 2022 64-bit
  - Qt SerialPort
  - CMake
  - Ninja

**macOS:**

*Option 1: Using Homebrew (Recommended)*
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Qt6
brew install qt@6
```

*Option 2: Using Qt Installer*
- Download Qt installer from [qt.io](https://www.qt.io/download)
- Install Qt 6.9.1 for macOS
- Ensure the following components are selected:
  - Qt 6.9.1 macOS
  - Qt SerialPort
  - CMake (if not installed via Xcode)

3. **Install Build Tools**

**Windows:**
- Install Visual Studio 2022 Community or higher
- Include "Desktop development with C++" workload

**macOS:**
- Install Xcode from App Store
- Install Xcode Command Line Tools:
```bash
xcode-select --install
```
- Install Homebrew (if not already installed):
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

4. **Build Project**

### Windows Build

**Method 1: One-Click Build & Deploy** (Recommended)

*Windows:*
```batch
# Modify paths in build_and_deploy.bat then run
build_and_deploy.bat
```

*macOS:*
```bash
# Make executable and run
chmod +x build_and_deploy_macos.sh
./build_and_deploy_macos.sh
```

**Method 2: Manual Build**
```batch
# 1. Navigate to project directory
cd C:\path\to\your\CDC_Set_Up_Control_Programm

# 2. Clean and create build directory
rmdir /s /q build
mkdir build && cd build

# 3. Configure CMake (modify Qt path to your actual installation)
cmake -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64" ..

# 4. Build project
cmake --build . --config Release

# 5. Navigate to executable directory
cd build\bin\Release

# 6. Deploy Qt dependencies
C:\Qt\6.9.1\msvc2022_64\bin\windeployqt.exe CDC_Control_System.exe

# 7. Run application
CDC_Control_System.exe
```

### macOS Build

**Method 1: Using Homebrew Qt**
```bash
# Install Qt using Homebrew
brew install qt@6

# Navigate to project directory
cd /path/to/your/CDC_Set_Up_Control_Programm

# Create build directory
rm -rf build
mkdir build && cd build

# Configure CMake
cmake -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)" \
      -DCMAKE_BUILD_TYPE=Release ..

# Build project
cmake --build . --parallel $(sysctl -n hw.ncpu)

# Run application
open bin/CDC_Control_System.app
# Or directly: ./bin/CDC_Control_System
```

**Method 2: Using Qt Installer**
```bash
# After installing Qt from qt.io to ~/Qt/6.9.1/macos
cd /path/to/your/CDC_Set_Up_Control_Programm

rm -rf build
mkdir build && cd build

# Configure CMake
cmake -DCMAKE_PREFIX_PATH="$HOME/Qt/6.9.1/macos" \
      -DCMAKE_BUILD_TYPE=Release ..

# Build project
cmake --build . --parallel $(sysctl -n hw.ncpu)

# Deploy Qt frameworks (if needed for distribution)
$HOME/Qt/6.9.1/macos/bin/macdeployqt bin/CDC_Control_System.app

# Run application
open bin/CDC_Control_System.app
```

5. **Create Distribution Package** (Optional)

**Windows:**
```batch
# Run deployment script to create complete distribution package
deploy\windows\deploy_windows.bat
```

**macOS:**
```bash
# Run deployment script to create complete distribution package
chmod +x deploy/macos/deploy_macos.sh
./deploy/macos/deploy_macos.sh

# Optional: Create DMG for distribution
cd deploy/package/macos
./create_dmg.sh
```

## 💻 Usage Instructions

### 1. Device Connection
- Open software, navigate to "Serial Communication" page
- Click "Add Device" to add serial port device
- Select correct COM port and baud rate (default 115200)
- Click "Connect" to establish connection

### 2. Motor Control
- Switch to "Motor Control" page
- Set target height (0-100mm) and angle (-90° to 90°)
- Click "Move to Position" to execute movement
- Use "Emergency Stop" for immediate halt

### 3. Data Recording
- View real-time data in "Sensor Monitor" page
- Click "Record Data" to log current data point
- Use "Export" to save data as CSV file

### 4. System Monitoring
- Monitor sensor readings in real-time
- Track system status and error conditions
- Configure alarm thresholds and notifications

## 🔧 Hardware Configuration

### Supported Hardware
- **Main Controller**: PSoC 4500S
- **Motor Driver**: TMC2208-LA
- **Distance Sensors**: DFRobot SEN0427 (×4)
- **Angle Sensor**: Murata SCA830-D07-1
- **Temperature Sensor**: DS18B20+
- **Servo Motor**: SER0019

### Communication Protocol
- Baud Rate: 9600/19200/38400/57600/115200 (selectable)
- Data Bits: 8
- Stop Bits: 1
- Parity: None

## 📊 Data Format

### CSV Export Format
```csv
Timestamp,Set_Height(mm),Set_Angle(deg),Theoretical_Capacitance(pF),
Upper_Sensor_1(mm),Upper_Sensor_2(mm),Lower_Sensor_1(mm),Lower_Sensor_2(mm),
Temperature(C),Measured_Angle(deg),Measured_Capacitance(pF),...
```

## 📦 Project Deployment

### Automated Deployment

**Windows:**
The project includes complete automated deployment solution:

- `build_and_deploy.bat` - One-click build and deployment
- `deploy/windows/deploy_windows.bat` - Windows-specific deployment script
- `deploy/package/qt.conf` - Qt runtime configuration
- `deploy/windows/app.ico` - Application icon

Deployed application package is located in `deploy/package/windows/` directory and can be directly copied to other Windows computers.

**macOS:**
Complete automated deployment with app bundle creation:

- `deploy/macos/deploy_macos.sh` - macOS deployment script with auto Qt detection
- Automatic `macdeployqt` execution
- DMG creation script for distribution
- App bundle with all dependencies included

```bash
# One-command deployment
chmod +x deploy/macos/deploy_macos.sh
./deploy/macos/deploy_macos.sh

# The deployed .app bundle is located in deploy/package/macos/
```

### Manual Distribution

**Windows:**
For manual distribution package creation:
1. Copy all files from `build/bin/Release/`
2. Run `windeployqt.exe` to deploy Qt dependencies
3. Include necessary runtime configuration files

**macOS:**
For manual distribution:
1. Build the application as shown above
2. Use `macdeployqt` to create a self-contained app bundle
3. Optionally create a DMG file for easier distribution
4. For App Store distribution, code signing is required

## 🐛 Troubleshooting

### Common Issues

1. **Cannot Connect to Device**
   - Check if serial port is in use
   - Verify correct baud rate setting
   - Check USB driver installation

2. **Build Errors**
   - Confirm Qt version >= 6.9.1
   - Check CMake version >= 3.16
   - Ensure Visual Studio 2022 is properly installed
   - Verify Qt path configuration

3. **windeployqt Cannot Find Dependencies**
   - Confirm correct Qt installation path
   - Check system PATH environment variable
   - Try running with administrator privileges

4. **Application Won't Run on Other Computers**
   - Ensure target computer has Visual C++ Redistributable 2022 installed
   - Verify deployment package includes