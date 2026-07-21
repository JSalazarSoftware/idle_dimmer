// Copyright (C) 2026 JSalazarSoftware
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://gnu.org>.

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <cstdio>
#include <memory>
#include <array>
#include <cstdlib> 
#include <algorithm> // For string trim helpers
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/scrnsaver.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrender.h>

// --- Configuration Variables (Loaded from config.txt) ---
long IDLE_WARNING_SEC   = 10;   // Default: 10 seconds
long IDLE_BLACKOUT_SEC  = 20;   // Default: 20 seconds
double MAX_DIM_OPACITY  = 0.65; // Default: 65% maximum darkness

// --- Hardcoded System Constants ---
const int  CHECK_INTERVAL_MS = 100;   // Poll rate for state tracking
const int  FADE_DURATION_MS  = 1500;  // Asymmetric fade-in length in milliseconds

// --- Application State ---
enum ScreenState { STATE_NORMAL, STATE_DIMMING, STATE_BLACKOUT };
ScreenState currentState = STATE_NORMAL;

// Helper function to strip trailing/leading spaces or tabs from parsed strings
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string getConfigPath() {
    const char* homeDir = std::getenv("HOME");
    if (!homeDir) {
        return "config.txt"; 
    }
    return std::string(homeDir) + "/.config/idle_dimmer/config.txt";
}

void loadConfiguration() {
    std::string path = getConfigPath();
    std::ifstream configFile(path);
    if (!configFile.is_open()) {
        std::cerr << "[Config] Warning: Could not open config file at " << path << ". Using defaults.\n";
        return;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        line = trim(line);
        if (line.empty()) continue;
        
        // Fix: Correctly checks if the line STARTs with a comment token
        if (line[0] == '#' || line[0] == ';') continue;

        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                key = trim(key);
                value = trim(value);

                if (key == "IDLE_WARNING_SEC") {
                    IDLE_WARNING_SEC = std::stol(value);
                } else if (key == "IDLE_BLACKOUT_SEC") {
                    IDLE_BLACKOUT_SEC = std::stol(value);
                } else if (key == "MAX_DIM_OPACITY") {
                    MAX_DIM_OPACITY = std::stod(value);
                    if (MAX_DIM_OPACITY < 0.0) MAX_DIM_OPACITY = 0.0;
                    if (MAX_DIM_OPACITY > 1.0) MAX_DIM_OPACITY = 1.0;
                }
            }
        }
    }
    std::cout << "[Config] Loaded Settings -> Warning: " << IDLE_WARNING_SEC 
              << "s | Blackout: " << IDLE_BLACKOUT_SEC << "s | Max Dimness: " 
              << (MAX_DIM_OPACITY * 100.0) << "%\n";
}

struct PipeDeleter {
    void operator()(FILE* p) const {
        if (p) pclose(p);
    }
};

bool isAudioPlaying() {
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, PipeDeleter> pipe(popen("pactl list short sink-inputs 2>/dev/null", "r"));
    if (!pipe) return false;
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return !result.empty();
}

long getIdleTimeMS(Display* display) {
    XScreenSaverInfo* info = XScreenSaverAllocInfo();
    XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
    long idle = info->idle;
    XFree(info);
    return idle;
}

void makeWindowClickThrough(Display* display, Window window) {
    XShapeCombineRectangles(display, window, ShapeInput, 0, 0, nullptr, 0, ShapeSet, YXBanded);
}

Window createOverlayWindow(Display* display, int screen, int width, int height, XVisualInfo vinfo) {
    XSetWindowAttributes attrs;
    attrs.colormap = XCreateColormap(display, RootWindow(display, screen), vinfo.visual, AllocNone);
    attrs.border_pixel = 0;
    attrs.background_pixel = 0;
    attrs.override_redirect = True; 

    Window win = XCreateWindow(
        display, RootWindow(display, screen),
        0, 0, width, height, 0,
        vinfo.depth, InputOutput, vinfo.visual,
        CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attrs
    );

    makeWindowClickThrough(display, win);
    
    Atom windowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom typeNotification = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False);
    XChangeProperty(display, win, windowType, XA_ATOM, 32, PropModeReplace, (unsigned char*)&typeNotification, 1);

    return win;
}

void paintOverlay(Display* display, Window window, XRenderPictFormat* format, int width, int height, double opacity) {
    XRenderPictureAttributes pa;
    Picture picture = XRenderCreatePicture(display, window, format, 0, &pa);
    
    XRenderColor color;
    color.red = 0;
    color.green = 0;
    color.blue = 0;
    color.alpha = static_cast<unsigned short>(opacity * 65535);

    XRenderFillRectangle(display, PictOpSrc, picture, &color, 0, 0, width, height);
    XRenderFreePicture(display, picture);
    XFlush(display);
}

void forceScreenStandby(Display* display) {
    std::cout << "[Action] Targets hit. Forcing DPMS Standby.\n";
    int dummy = std::system("xset dpms force standby");
    (void)dummy;
}

void forceScreenWake() {
    std::cout << "[Action] Activity detected. Waking screen.\n";
    int dummy = std::system("xset dpms force on");
    (void)dummy;
}

int main() {
    loadConfiguration();

    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X display\n";
        return 1;
    }

    int screen = DefaultScreen(display);
    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);

    XVisualInfo vinfo;
    if (!XMatchVisualInfo(display, screen, 32, TrueColor, &vinfo)) {
        std::cerr << "No 32-bit ARGB visual found!\n";
        XCloseDisplay(display);
        return 1;
    }

    XRenderPictFormat* format = XRenderFindVisualFormat(display, vinfo.visual);
    if (!format) {
        std::cerr << "XRender format not found.\n";
        XCloseDisplay(display);
        return 1;
    }

    Window overlay = createOverlayWindow(display, screen, width, height, vinfo);
    double currentOpacity = 0.0;

    std::cout << "Custom Desktop Idle Tool Started.\n";

    while (true) {
        long idleTime = getIdleTimeMS(display);
        bool mediaActive = isAudioPlaying();

        if (mediaActive) {
            idleTime = 0; 
        }

        long idleWarningMs  = IDLE_WARNING_SEC * 1000;
        long idleBlackoutMs = IDLE_BLACKOUT_SEC * 1000;

        if (idleTime >= idleBlackoutMs) {
            if (currentState != STATE_BLACKOUT) {
                currentState = STATE_BLACKOUT;
                forceScreenStandby(display);
            }
        } 
        else if (idleTime >= idleWarningMs) {
            if (currentState == STATE_BLACKOUT) {
                forceScreenWake();
            }
            if (currentState != STATE_DIMMING) {
                XMapRaised(display, overlay);
                currentState = STATE_DIMMING;
            }

            long timeInWarning = idleTime - idleWarningMs;
            double fadeRatio = static_cast<double>(timeInWarning) / FADE_DURATION_MS;
            if (fadeRatio > 1.0) fadeRatio = 1.0;

            currentOpacity = fadeRatio * MAX_DIM_OPACITY;
            paintOverlay(display, overlay, format, width, height, currentOpacity);
        } 
        else {
            if (currentState == STATE_BLACKOUT) {
                forceScreenWake();
            }
            if (currentState != STATE_NORMAL) {
                XUnmapWindow(display, overlay);
                XFlush(display);
                currentOpacity = 0.0;
                currentState = STATE_NORMAL;
                std::cout << "[Action] Screen restored to normal.\n";
                
                loadConfiguration();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));
    }

    XDestroyWindow(display, overlay);
    XCloseDisplay(display);
    return 0;
}
