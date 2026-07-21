# Idle Dimmer Daemon

A custom, lightweight desktop utility for Linux Mint (Cinnamon/X11) that provides smart, hardware-independent screen dimming and power saving. 

This tool is designed specifically for desktop PCs where kernel-level backlight controllers or standard utilities (like `brightnessctl`) fail. It uses an input-transparent software overlay via X11 XRender for gradual dimming, and invokes DPMS directly to safely enter standby mode without enforcing an automated lock screen.

## Key Features

- **Hardware-Independent Software Dimming:** Fades your monitor down smoothly using an X11 ARGB alpha transparency channel overlay.
- **Input-Transparent Overlay:** Uses the XShape extension to ensure the dark overlay is completely click-through. Keyboard inputs, clicks, and scrolls pass directly to your active apps underneath.
- **DPMS Power Management:** Forces your display into complete hardware standby (`xset dpms force standby`) without interacting with Cinnamon's lock modules or password screens.
- **Asymmetric Snap Back:** Fades out smoothly over 1.5 seconds when entering an idle warning state, but snaps back to full transparency in **0 milliseconds** the instant user activity is detected.
- **Media & Call Detection:** Continuously checks system audio streams via `PulseAudio`/`PipeWire` to automatically pause the dimming cycle if movies, music, or video calls are active.
- **Dynamic Hot-Reloading:** Reads configurations from your global home profile folder and automatically refreshes your settings every time you wake the monitor back up.

## Installation & Setup

If you are setting this up on a fresh Linux installation, follow these quick commands to clone, compile, and configure the background daemon.

### 1. Install Dependencies
Ensure your Linux Mint machine has the required C++ and X11 development headers installed:
```bash
sudo apt update
sudo apt install build-essential libx11-dev libxext-dev libxrender-dev libxss-dev
```

### 2. Compile the Binary
Build the optimized production binary using `g++`:
```bash
g++ -O3 main.cpp -o idle_dimmer -lX11 -lXext -lXrender -lXss
```

### 3. Deploy Directories and Config File
Create the persistent user configurations infrastructure and place your binary in your local user path:
```bash
# Create local bin and config directories
mkdir -p ~/.local/bin
mkdir -p ~/.config/idle_dimmer

# Copy executable and baseline configuration
cp idle_dimmer ~/.local/bin/idle_dimmer
cp config.txt ~/.config/idle_dimmer/config.txt
```

### 4. Enable Startup Autostart
To ensure the application launches silently in the background whenever you sign into your Cinnamon desktop session, create an autostart desktop profile shortcut:
```bash
cat << 'EOF' > ~/.config/autostart/idle_dimmer.desktop
[Desktop Entry]
Type=Application
Name=Idle Dimmer Daemon
Comment=Custom Input-Transparent Hardware Idle Software Dimming Layer
Exec=/home/joe/.local/bin/idle_dimmer
Icon=display
Terminal=false
Categories=Utility;
X-GNOME-Autostart-enabled=true
EOF
```
*(Note: If your system profile name is not `joe`, replace the username in the `Exec=` line above with your actual user path).*

## Configuration

You can change all timing metrics or maximum overlay darkness constraints by modifying your system configuration file at **`~/.config/idle_dimmer/config.txt`**:

```text
# Configuration for Idle Dimmer (Times in seconds)
IDLE_WARNING_SEC=10
IDLE_BLACKOUT_SEC=20

# Max software dimming opacity value between 0.0 (clear) and 1.0 (blackout)
MAX_DIM_OPACITY=0.65
```
Settings are hot-reloaded automatically every time you return to your desk and wake the machine!

## Process Management

- **Manually Run Process in Background:**
  ```bash
  ~/.local/bin/idle_dimmer > /dev/null 2>&1 &
  ```
- **Terminate Active Background Daemon:**
  ```bash
  pkill idle_dimmer
  ```

## License

Distributed under the GNU General Public License v3.0 or later. See `LICENSE` for more information.
