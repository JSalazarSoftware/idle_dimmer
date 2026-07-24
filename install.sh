#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "===================================================="
echo "      Idle Dimmer Daemon Installer Script"
echo "===================================================="

# 1. Check for required build and library dependencies
echo "=> Checking system dependencies..."
REQUIRED_PKGS=(build-essential libx11-dev libxext-dev libxrender-dev libxss-dev)
MISSING_PKGS=()

for pkg in "${REQUIRED_PKGS[@]}"; do
    if ! dpkg -l "$pkg" &> /dev/null; then
        MISSING_PKGS+=("$pkg")
    fi
done

if [ ${#MISSING_PKGS[@]} -ne 0 ]; then
    echo " -> Missing dependencies: ${MISSING_PKGS[*]}"
    echo " -> Requesting password to install missing packages..."
    sudo apt update
    sudo apt install -y "${MISSING_PKGS[@]}"
else
    echo " -> All required X11 development packages are installed."
fi

# 2. Compile the production binary
echo "=> Compiling optimized production binary with g++..."
if [ ! -f "main.cpp" ]; then
    echo "❌ Error: main.cpp not found in the current directory!"
    exit 1
fi
g++ -O3 main.cpp -o idle_dimmer -lX11 -lXext -lXrender -lXss
echo " -> Compilation successful!"

# 3. Create persistent directories
echo "=> Creating local directory infrastructure..."
mkdir -p "$HOME/.local/bin"
mkdir -p "$HOME/.config/idle_dimmer"

# 4. Create baseline standard TOML configuration if it doesn't exist
CONFIG_FILE="$HOME/.config/idle_dimmer/config.toml"
if [ ! -f "$CONFIG_FILE" ]; then
    echo "=> Creating baseline KVM-friendly config.toml at $CONFIG_FILE..."
    cat << 'EOF' > "$CONFIG_FILE"
# Configuration for the screen dimmer tool

[timers]
# Inactivity time before the screen starts dimming (in seconds)
idle_warning_sec = 10

# Total inactivity time before full screen blackout (in seconds)
idle_blackout_sec = 20

[display]
# Maximum dimness level during warning phase (0.0 = clear, 1.0 = pitch black)
max_dim_opacity = 0.65

# 0 = Safe Software Blackout (KVM-friendly, leaves screen on but 100% black)
# 1 = Hardware Power Down (Puts monitor panel to sleep via DPMS)
use_dpms = 0
EOF
    echo " -> Baseline config.toml generated successfully."
else
    echo " -> Existing config.toml found. Leaving your settings intact."
fi

# 5. Process management (Kills active binaries before overwriting)
echo "=> Managing active processes..."
if pgrep -x "idle_dimmer" > /dev/null; then
    echo " -> Found running background process. Terminating legacy instance..."
    pkill -9 -x "idle_dimmer" || true
    sleep 0.5
fi

# 6. Deploy binary execution path
echo "=> Deploying executable binary to local user path..."
cp idle_dimmer "$HOME/.local/bin/idle_dimmer"

# 7. Enable Desktop Session Autostart (FIXED: Added single quotes to prevent evaluation errors)
echo "=> Setting up Cinnamon desktop session auto-start shortcut..."
mkdir -p "$HOME/.config/autostart"
cat << 'EOF' > "$HOME/.config/autostart/idle_dimmer.desktop"
[Desktop Entry]
Type=Application
Name=Idle Dimmer Daemon
Comment=Custom Input-Transparent Hardware Idle Software Dimming Layer
Exec=sh -c "$HOME/.local/bin/idle_dimmer"
Icon=display
Terminal=false
Categories=Utility;
X-GNOME-Autostart-enabled=true
EOF
echo " -> Desktop entries populated cleanly."

echo "=> Launching the newly updated daemon background service..."
"$HOME/.local/bin/idle_dimmer" > /dev/null 2>&1 &

echo "===================================================="
echo "🎉 Process Finished Successfully!"
echo "===================================================="
echo " -> Daemon running live in the background (PID: $!)."
echo " -> KVM switch protection is ACTIVE by default (use_dpms = 0)."
echo " -> Edit settings at: ~/.config/idle_dimmer/config.toml"
echo "===================================================="
