#pragma once

#include <stdint.h>
#include "gui_state.h"

/* Layout metrics */
#define TOPBAR_HEIGHT               36
#define DOCK_WIDTH                  72
#define TITLEBAR_HEIGHT             32

/* Dynamic Palette References */
#define GUI_ACCENT_PRIMARY          g_gui_accent
#define GUI_BG_TOP                  g_gui_bg_top
#define GUI_BG_BOTTOM               g_gui_bg_bottom

/* Top Bar */
#define GUI_TOPBAR_BG               0x0C101A
#define GUI_TOPBAR_BORDER           0x1E293B

/* Dock */
#define GUI_DOCK_BG                 0x090D16
#define GUI_DOCK_BORDER             0x1E293B
#define GUI_DOCK_TILE               0x141C2E
#define GUI_DOCK_TILE_HOVER         0x1E293B
#define GUI_DOCK_TILE_BORDER        0x283548

/* Window Container & Frames */
#define GUI_WIN_BG                  0x080B11
#define GUI_WIN_SHADOW              0x030408
#define GUI_WIN_TITLEBAR_ACTIVE     0x1A2333
#define GUI_WIN_TITLEBAR_INACTIVE   0x101622
#define GUI_WIN_BORDER_ACTIVE       0x2E3D52
#define GUI_WIN_BORDER_INACTIVE     0x19212D

/* Fixed Accents */
#define GUI_ACCENT_SUCCESS          0x10B981   /* Emerald / Maximize */
#define GUI_ACCENT_WARNING          0xF59E0B   /* Amber / Minimize */
#define GUI_ACCENT_DANGER           0xEF4444   /* Crimson / Close */

/* Typography */
#define GUI_TEXT_PRIMARY            0xF8FAFC
#define GUI_TEXT_SECONDARY          0x94A3B8
#define GUI_TEXT_MUTED              0x475569

/* Backward compatibility aliases */
#define GUI_BG                      GUI_BG_TOP
#define GUI_TOPBAR                  GUI_TOPBAR_BG
#define GUI_ACCENT                  GUI_ACCENT_PRIMARY
#define GUI_TEXT                    GUI_TEXT_PRIMARY
#define GUI_SECONDARY               GUI_TEXT_SECONDARY
#define GUI_BORDER                  GUI_TOPBAR_BORDER
