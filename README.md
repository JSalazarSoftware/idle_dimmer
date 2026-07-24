## Installation & Setup

Choose your preferred method below to install, compile, and configure the background daemon.

### Method 1: The Automated Way (Recommended)
We provide an automated installer script that handles downloading/verifying dependencies, compiling the binary with performance optimizations (`-O3`), structuring your local environment, setting up Cinnamon autostart, and launching the daemon right away.

```bash
chmod +x install.sh
./install.sh
```

---

### Method 2: The Manual Way (For the Hardcore Linux Nerds 🤓)
If you prefer to audit and run the deployment steps yourself, follow these commands:

#### 1. Install System Development Headers
Ensure your Linux Mint machine has the required C++ and X11 development headers installed:
```bash
sudo apt update
sudo apt install build-essential libx11-dev libxext-dev libxrender-dev libxss-dev
```

#### 2. Compile the Optimized Production Binary
Build the binary with aggressive compiler optimizations (`-O3`) using `g++`:
```bash
g++ -O3 main.cpp -o idle_dimmer -lX11 -lXext -lXrender -lXss
```

#### 3. Deploy Local Workspace Architecture
Create the persistent user configuration folders and place your executable and a baseline configuration in your local home space:
```bash
# Create local bin and config directories
mkdir -p ~/.local/bin
mkdir -p ~/.config/idle_dimmer

# Copy executable and create the baseline TOML configuration
cp idle_dimmer ~/.local/bin/idle_dimmer

cat << 'EOF' > ~/.config/idle_dimmer/config.toml
# Configuration for the screen dimmer tool

[timers]
idle_warning_sec = 10
idle_blackout_sec = 20

[display]
max_dim_opacity = 0.65
use_dpms = 0
EOF
```

#### 4. Configure Background Autostart
To ensure the daemon launches silently whenever you sign into your desktop session, generate a custom system autostart profile shortcut:
```bash
cat << 'EOF' > ~/.config/autostart/idle_dimmer.desktop
[Desktop Entry]
Type=Application
Name=Idle Dimmer Daemon
Comment=Custom Input-Transparent Hardware Idle Software Dimming Layer
Exec=sh -c "\$HOME/.local/bin/idle_dimmer"
Icon=display
Terminal=false
Categories=Utility;
X-GNOME-Autostart-enabled=true
EOF
```

#### 5. Launch Live Service
Start up your newly deployed daemon into a detached background environment:
```bash
~/.local/bin/idle_dimmer > /dev/null 2>&1 &
```
