# CDC Setup Control Program

A Qt-based control program for CDC (Capacitance Distance Control) measurement system, featuring real-time sensor monitoring, motor control, and data recording capabilities.

## Features

- **Real-time Sensor Monitoring**: Monitor multiple distance sensors, temperature, angle, and capacitance measurements
- **Dual Motor Control**: Control stepper motor (height) and servo motor (angle) with safety limits
- **Data Recording & Export**: Record measurement data and export to CSV format
- **Calibration Support**: Manual and automatic calibration modes
- **Safety Features**: Emergency stop, range limits, and speed control
- **Multi-device Support**: Connect and manage multiple serial devices

## System Requirements

- **OS**: Windows 10/11, Linux (Ubuntu 20.04+), macOS 11+
- **Qt**: 6.5 or higher
- **Compiler**: C++17 compatible (GCC 9+, MSVC 2019+, Clang 10+)
- **CMake**: 3.16 or higher
- **Hardware**: USB serial port for MCU communication

## Building from Source

### Prerequisites

1. Install Qt 6.5+ with the following components:
   - Qt Core
   - Qt Widgets
   - Qt Serial Port

2. Install CMake 3.16+

3. Install Google Test (optional, for running tests)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/yourusername/CDC_Set_Up_Control_Program.git
cd CDC_Set_Up_Control_Program

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Run tests (optional)
ctest