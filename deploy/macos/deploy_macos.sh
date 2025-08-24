#!/bin/bash

# CDC Control System - macOS Deployment Script
# Author: RuiYing, TUM LSE Laboratory

set -e  # Exit on any error

# Color output functions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Set project paths (relative to this script location)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
APP_PATH="$BUILD_DIR/bin/CDC_Control_System.app"
DEPLOY_DIR="$PROJECT_ROOT/deploy/package/macos"

# Qt paths - try to detect automatically
QT_PATHS=(
    "$(brew --prefix qt@6 2>/dev/null || echo '')/bin"
    "$HOME/Qt/6.9.1/macos/bin"
    "/usr/local/opt/qt@6/bin"
    "/opt/homebrew/opt/qt@6/bin"
)

QT_BIN=""
for path in "${QT_PATHS[@]}"; do
    if [[ -n "$path" ]] && [[ -x "$path/macdeployqt" ]]; then
        QT_BIN="$path"
        break
    fi
done

echo "================================================"
echo "       CDC Control System Deployment Tool"
echo "================================================"

print_info "Project Root: $PROJECT_ROOT"
print_info "Build Directory: $BUILD_DIR"
print_info "App Bundle: $APP_PATH"
print_info "Deployment Directory: $DEPLOY_DIR"

# Check Qt installation
if [[ -z "$QT_BIN" ]]; then
    print_error "Qt not found! Please install Qt6 using:"
    echo "  brew install qt@6"
    echo "  or download from https://www.qt.io/download"
    exit 1
fi

print_success "Qt found at: $QT_BIN"

# Check if build completed
if [[ ! -d "$APP_PATH" ]]; then
    print_error "App bundle not found at: $APP_PATH"
    print_error "Please build first:"
    echo "  mkdir build && cd build"
    echo "  cmake -DCMAKE_PREFIX_PATH=\"\$(brew --prefix qt@6)\" -DCMAKE_BUILD_TYPE=Release .."
    echo "  cmake --build . --parallel \$(sysctl -n hw.ncpu)"
    exit 1
fi

print_success "App bundle found"

# Create deployment directory
print_info "Creating deployment directory..."
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR"

# Copy app bundle
print_info "Copying app bundle..."
cp -R "$APP_PATH" "$DEPLOY_DIR/"

if [[ ! -d "$DEPLOY_DIR/CDC_Control_System.app" ]]; then
    print_error "Failed to copy app bundle"
    exit 1
fi

# Copy resources if they exist
if [[ -d "$PROJECT_ROOT/resources" ]]; then
    print_info "Copying resources..."
    cp -R "$PROJECT_ROOT/resources" "$DEPLOY_DIR/CDC_Control_System.app/Contents/"
fi

# Copy configuration files
if [[ -f "$PROJECT_ROOT/deploy/package/qt.conf" ]]; then
    print_info "Copying Qt configuration..."
    cp "$PROJECT_ROOT/deploy/package/qt.conf" "$DEPLOY_DIR/CDC_Control_System.app/Contents/Resources/"
fi

# Run macdeployqt
print_info "Running macdeployqt..."
cd "$DEPLOY_DIR"

"$QT_BIN/macdeployqt" CDC_Control_System.app

if [[ $? -ne 0 ]]; then
    print_error "macdeployqt failed"
    exit 1
fi

print_success "macdeployqt completed successfully"

# Create launcher script
print_info "Creating launcher script..."
cat > "$DEPLOY_DIR/run_CDC_Control_System.sh" << 'EOF'
#!/bin/bash
# CDC Control System Launcher

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Starting CDC Control System..."
open CDC_Control_System.app

# Wait a moment and check if app started
sleep 2
if pgrep -f "CDC_Control_System" > /dev/null; then
    echo "CDC Control System started successfully"
else
    echo "Warning: CDC Control System may not have started properly"
    echo "You can also double-click CDC_Control_System.app directly"
fi
EOF

chmod +x "$DEPLOY_DIR/run_CDC_Control_System.sh"

# Create DMG creation script (optional)
print_info "Creating DMG creation script..."
cat > "$DEPLOY_DIR/create_dmg.sh" << 'EOF'
#!/bin/bash
# Create DMG for distribution

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DMG_NAME="CDC_Control_System_macOS"
VOLUME_NAME="CDC Control System"

cd "$SCRIPT_DIR"

# Create temporary DMG
hdiutil create -size 100m -fs HFS+ -volname "$VOLUME_NAME" "$DMG_NAME.temp.dmg"

# Mount the DMG
hdiutil attach "$DMG_NAME.temp.dmg"

# Copy app to DMG
cp -R CDC_Control_System.app "/Volumes/$VOLUME_NAME/"

# Create Applications symlink
ln -s /Applications "/Volumes/$VOLUME_NAME/Applications"

# Unmount
hdiutil detach "/Volumes/$VOLUME_NAME"

# Convert to compressed read-only DMG
hdiutil convert "$DMG_NAME.temp.dmg" -format UDZO -o "$DMG_NAME.dmg"
rm "$DMG_NAME.temp.dmg"

echo "DMG created: $DMG_NAME.dmg"
EOF

chmod +x "$DEPLOY_DIR/create_dmg.sh"

# Create deployment README
print_info "Creating deployment README..."
cat > "$DEPLOY_DIR/README.txt" << EOF
CDC Control System - macOS Deployment Package
============================================

This package contains the CDC Control System application
and all required Qt dependencies for macOS.

To run the application:
1. Double-click CDC_Control_System.app
2. Or run: ./run_CDC_Control_System.sh
3. Or from Terminal: open CDC_Control_System.app

System Requirements:
- macOS 10.15 Catalina or later
- Intel or Apple Silicon Mac

Optional - Create DMG for distribution:
- Run: ./create_dmg.sh

Installation:
- Drag CDC_Control_System.app to your Applications folder

Developer: RuiYing, TUM LSE Laboratory
Build Date: $(date)
EOF

# Calculate package size
PACKAGE_SIZE=$(du -sh "$DEPLOY_DIR" | cut -f1)

# Display deployment information
echo ""
echo "================================================"
echo "             Deployment Complete!"
echo "================================================"
print_success "Deployment directory: $DEPLOY_DIR"
print_success "Package size: $PACKAGE_SIZE"
echo ""
print_info "Package contents:"
ls -la "$DEPLOY_DIR"

echo ""
print_info "You can now:"
echo "1. Run: open $DEPLOY_DIR/CDC_Control_System.app"
echo "2. Create DMG: cd $DEPLOY_DIR && ./create_dmg.sh"
echo "3. Copy the entire 'macos' folder to other Macs"
echo "4. Upload to distribution platform"
echo ""

# Test deployment option
read -p "Would you like to test run the deployed application? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_info "Testing deployed application..."
    open "$DEPLOY_DIR/CDC_Control_System.app"
    sleep 3
    
    if pgrep -f "CDC_Control_System" > /dev/null; then
        print_success "Application started successfully! Deployment is complete!"
    else
        print_warning "Application may not have started. Check the app manually."
    fi
fi

print_success "Deployment script finished successfully!"