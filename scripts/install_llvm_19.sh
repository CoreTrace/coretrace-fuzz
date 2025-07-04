#!/bin/bash

# Script to install and switch to LLVM/Clang 19.1 on Arch Linux
# This script will install LLVM 19 alongside the current version

set -e

echo "=== LLVM/Clang 19.1 Installation Script for Arch Linux ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   print_error "This script should not be run as root"
   exit 1
fi

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check current LLVM version
print_status "Checking current LLVM/Clang version..."
if command_exists clang; then
    current_version=$(clang --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+')
    echo "Current Clang version: $current_version"
else
    echo "Clang not found"
fi

# Check if LLVM 19 is already installed
print_status "Checking if LLVM 19 is already available..."
if command_exists llvm-config-19 || command_exists clang-19; then
    print_success "LLVM 19 tools already found on system!"
else
    print_status "LLVM 19 not found, proceeding with installation..."
fi

# Method 1: Try to install LLVM 19 from official repositories
print_status "Attempting to install LLVM 19 from official repositories..."

# Update package database
print_status "Updating package database..."
sudo pacman -Sy

# Try to install LLVM 19 packages
print_status "Installing LLVM 19 packages..."
if sudo pacman -S --needed --noconfirm llvm19 clang19 llvm19-libs 2>/dev/null; then
    print_success "LLVM 19 installed from official repositories"
else
    print_warning "LLVM 19 not available in official repositories, trying AUR..."
    
    # Method 2: Install from AUR using yay or paru
    if command_exists yay; then
        print_status "Installing LLVM 19 from AUR using yay..."
        yay -S --needed --noconfirm llvm19 clang19
    elif command_exists paru; then
        print_status "Installing LLVM 19 from AUR using paru..."
        paru -S --needed --noconfirm llvm19 clang19
    else
        print_warning "No AUR helper found. You'll need to install LLVM 19 manually."
        print_status "Please install yay or paru first, or install LLVM 19 manually from AUR"
        
        # Method 3: Manual installation from official LLVM
        print_status "Attempting manual installation from LLVM official releases..."
        
        # Create temporary directory
        TEMP_DIR=$(mktemp -d)
        cd "$TEMP_DIR"
        
        # Download LLVM 19.1.6 (latest 19.x release)
        LLVM_VERSION="19.1.6"
        ARCH="x86_64"
        LLVM_URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/clang+llvm-${LLVM_VERSION}-${ARCH}-linux-gnu-ubuntu-22.04.tar.xz"
        
        print_status "Downloading LLVM ${LLVM_VERSION}..."
        if wget "$LLVM_URL" -O llvm-${LLVM_VERSION}.tar.xz; then
            print_status "Extracting LLVM ${LLVM_VERSION}..."
            tar -xf llvm-${LLVM_VERSION}.tar.xz
            
            # Install to /opt/llvm-19
            INSTALL_DIR="/opt/llvm-19"
            print_status "Installing LLVM ${LLVM_VERSION} to ${INSTALL_DIR}..."
            sudo mkdir -p "$INSTALL_DIR"
            sudo cp -r clang+llvm-${LLVM_VERSION}-${ARCH}-linux-gnu-ubuntu-22.04/* "$INSTALL_DIR/"
            
            # Create symlinks
            print_status "Creating symlinks..."
            sudo ln -sf "$INSTALL_DIR/bin/clang" /usr/local/bin/clang-19
            sudo ln -sf "$INSTALL_DIR/bin/clang++" /usr/local/bin/clang++-19
            sudo ln -sf "$INSTALL_DIR/bin/llvm-config" /usr/local/bin/llvm-config-19
            sudo ln -sf "$INSTALL_DIR/bin/llc" /usr/local/bin/llc-19
            sudo ln -sf "$INSTALL_DIR/bin/opt" /usr/local/bin/opt-19
            
            # Add to LD_LIBRARY_PATH
            echo "export LD_LIBRARY_PATH=\"$INSTALL_DIR/lib:\$LD_LIBRARY_PATH\"" | sudo tee /etc/profile.d/llvm-19.sh
            source /etc/profile.d/llvm-19.sh
            
            print_success "LLVM ${LLVM_VERSION} installed manually to ${INSTALL_DIR}"
        else
            print_error "Failed to download LLVM ${LLVM_VERSION}"
            exit 1
        fi
        
        # Cleanup
        cd - > /dev/null
        rm -rf "$TEMP_DIR"
    fi
fi

# Verify installation
print_status "Verifying LLVM 19 installation..."

# Check for LLVM 19 binaries
LLVM19_FOUND=false
for cmd in clang-19 clang++-19 llvm-config-19; do
    if command_exists "$cmd"; then
        version=$($cmd --version 2>/dev/null | head -n1 || echo "Unknown")
        print_success "$cmd found: $version"
        LLVM19_FOUND=true
    else
        print_warning "$cmd not found"
    fi
done

if [ "$LLVM19_FOUND" = false ]; then
    print_error "LLVM 19 installation failed or not properly configured"
    exit 1
fi

# Create environment setup script
print_status "Creating environment setup script..."
cat > /tmp/setup_llvm19_env.sh << 'EOF'
#!/bin/bash

# Environment setup for LLVM 19
export LLVM_CONFIG=llvm-config-19
export CC=clang-19
export CXX=clang++-19

# Add LLVM 19 paths
if [ -d "/opt/llvm-19" ]; then
    export PATH="/opt/llvm-19/bin:$PATH"
    export LD_LIBRARY_PATH="/opt/llvm-19/lib:$LD_LIBRARY_PATH"
    export LLVM_DIR="/opt/llvm-19"
fi

# For CMake
export LLVM_ROOT=/opt/llvm-19
export CMAKE_PREFIX_PATH="/opt/llvm-19:$CMAKE_PREFIX_PATH"

echo "Environment configured for LLVM 19"
echo "CC=$CC"
echo "CXX=$CXX"
echo "LLVM_CONFIG=$LLVM_CONFIG"
EOF

# Copy the environment script to the project
cp /tmp/setup_llvm19_env.sh /home/yoolooops/eip/coretrace-fuzz/scripts/setup_llvm19_env.sh
chmod +x /home/yoolooops/eip/coretrace-fuzz/scripts/setup_llvm19_env.sh

print_success "Environment setup script created at scripts/setup_llvm19_env.sh"

# Test the dynamic library compatibility
print_status "Testing dynamic library compatibility..."
source /home/yoolooops/eip/coretrace-fuzz/scripts/setup_llvm19_env.sh

cd /home/yoolooops/eip/coretrace-fuzz

# Try to load the library with LLVM 19
print_status "Testing libcompilerlib.so with LLVM 19..."
if [ -f "include/compilerlib/libcompilerlib.so" ]; then
    # Simple test to see if the library loads
    if ldd include/compilerlib/libcompilerlib.so | grep -q "not found"; then
        print_warning "Some dependencies not found:"
        ldd include/compilerlib/libcompilerlib.so | grep "not found" || true
    else
        print_success "Library dependencies look good"
    fi
    
    # Check library architecture and symbols
    print_status "Library info:"
    file include/compilerlib/libcompilerlib.so
    nm -D include/compilerlib/libcompilerlib.so 2>/dev/null | grep -E "(compile_to_ir|get_functions_from)" | head -5 || echo "No matching symbols found"
else
    print_error "libcompilerlib.so not found at include/compilerlib/libcompilerlib.so"
fi

echo
print_success "LLVM 19 installation complete!"
echo
echo "To use LLVM 19 in your current session, run:"
echo "  source scripts/setup_llvm19_env.sh"
echo
echo "To make this permanent, add the following to your ~/.bashrc or ~/.zshrc:"
echo "  source /home/yoolooops/eip/coretrace-fuzz/scripts/setup_llvm19_env.sh"
echo
echo "Then rebuild your project with:"
echo "  cd /home/yoolooops/eip/coretrace-fuzz"
echo "  source scripts/setup_llvm19_env.sh"
echo "  make clean"
echo "  cmake -B build -DCMAKE_BUILD_TYPE=Debug"
echo "  make -C build"
echo
