#include "desktop.h"
#include "theme.h"
#include "cursor.h"

#include "windows.h"
#include "window_manager.h"
#include "../font/font.h"
#include "../graphics/graphics.h"
#include "../framebuffer/framebuffer.h"
#include "../drivers/mouse.h"

static void desktop_draw_background(void)
{
    for (size_t y = TOPBAR_HEIGHT; y < framebuffer_height(); y++)
    {
        uint32_t color = 0x2D2D2D + (y / 20);

        fill_rect(
            DOCK_WIDTH,
            y,
            framebuffer_width() - DOCK_WIDTH,
            1,
            color
        );
    }

    fill_rect(
        0,
        0,
        framebuffer_width(),
        TOPBAR_HEIGHT,
        GUI_TOPBAR
    );

    draw_text_scaled(
        20,
        14,
        "Zenith OS",
        GUI_TEXT,
        2,
        0
    );

    size_t text_width = font_text_width_scaled(
        "Development Build",
        2,
        0
    );

    draw_text_scaled(
        framebuffer_width() - text_width - 20,
        14,
        "Development Build",
        GUI_SECONDARY,
        2,
        0
    );

    fill_rect(
        0,
        TOPBAR_HEIGHT - 1,
        framebuffer_width(),
        1,
        GUI_BORDER
    );

    fill_rect(
        0,
        TOPBAR_HEIGHT,
        DOCK_WIDTH,
        framebuffer_height() - TOPBAR_HEIGHT,
        0x1E293B
    );

    fill_rect(17,69,38,38,0x64748B);
    fill_rect(18,70,36,36,0x475569);

    fill_rect(17,119,38,38,0x64748B);
    fill_rect(18,120,36,36,0x475569);

    fill_rect(17,169,38,38,0x64748B);
    fill_rect(18,170,36,36,0x475569);

    fill_rect(17,219,38,38,0x64748B);
    fill_rect(18,220,36,36,0x475569);
}

void desktop_init(void)
{
    desktop_draw_background();
}

void desktop_draw(void)
{
    desktop_draw_background();

    window_manager_draw();

    cursor_draw(mouse_state.x, mouse_state.y);
}
