# Idle Dimmer Daemon

A custom, lightweight desktop utility for Linux Mint (Cinnamon/X11) that provides smart, hardware-independent screen dimming and KVM-friendly power management. This tool is designed specifically for setups where kernel-level backlight controllers or standard utilities (like `brightnessctl`) fail, or where physical KVM switches break display connections when monitors are powered off. 

It uses an input-transparent software overlay via X11 XRender for gradual dimming, and offers a choice between an overlay-driven software blackout (preserving KVM handshakes) or direct DPMS hardware standby.

## Key Features

- **Hardware-Independent Software Dimming:** Fades your monitor down smoothly using an X11 ARGB alpha transparency channel overlay.
- **Input-Transparent Overlay:** Uses the XShape extension to ensure the dark overlay is completely click-through. Keyboard inputs, clicks, and scrolls pass directly to your active apps underneath.
- **KVM & Hardware-Friendly Modes:** 
  - *Software Blackout:* Paints a 100% black software layer while keeping the video signal active. This stops KVM switches and TV displays from dropping their hardware connection (EDID handshake) when you switch computers.
  - *DPMS Power Management:* Directly forces your display into complete hardware standby (`xset dpms force standby`) if you are not using a KVM and want to save physical power.
- **Asymmetric Snap Back:** Fades out smoothly over 1.5 seconds when entering an idle warning state, but snaps back to full transparency in **0 milliseconds** the instant user activity is detected.
- **Media & Call Detection:** Continuously checks system audio streams via `PulseAudio`/`PipeWire` to automatically pause the dimming cycle if movies, music, or video calls are active.
- **Dynamic Hot-Reloading:** Reads structured TOML configurations from your global home profile folder and automatically refreshes your settings every time you wake the monitor back up.

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
cp config.toml ~/.config/idle_dimmer/config.toml
```

### 4. Enable Startup Autostart
To ensure the application launches silently in the background whenever you sign into your Cinnamon desktop session, run this command to create a desktop autostart profile shortcut:
```bash
cat << EOF > ~/.config/autostart/idle_dimmer.desktop
[Desktop Entry]
Type=Application
Name=Idle Dimmer Daemon
Comment=Custom Input-Transparent Hardware Idle Software Dimming Layer
Exec=\$HOME/.local/bin/idle_dimmer
Icon=display
Terminal=false
Categories=Utility;
X-GNOME-Autostart-enabled=true
EOF
```

---

## Configuration

You can customize your timing metrics, maximum overlay darkness constraints, and KVM behavior by modifying your system configuration file at **`~/.config/idle_dimmer/config.toml`**. 

```toml
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
```
Settings are hot-reloaded automatically every time you return to your desk and wake the machine!

> **💡 Linux Mint Cinnamon KVM Tip:** If you are using a KVM switch (`use_dpms = 0`), you should also open Mint's **System Settings -> Power Management**, and set **"Turn off the screen when inactive for"** to **Never**. This completely stops Cinnamon from overriding your daemon and turning off the video link.

---

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
