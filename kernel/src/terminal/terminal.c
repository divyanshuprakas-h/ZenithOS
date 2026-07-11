#include "../framebuffer/framebuffer.h"
#include "terminal.h"
#include "../font/font.h"
#include "terminal_buffer.h"

static size_t cursor_row;
static size_t cursor_col;
static terminal_cell_t terminal_buffer[TERM_ROWS][TERM_COLS];

static uint32_t terminal_color = 0xFFFFFF;
static void terminal_render(void);

static void terminal_scroll(void)
{
    for (size_t row = 1; row < TERM_ROWS; row++)
    {
        for (size_t col = 0; col < TERM_COLS; col++)
        {
            terminal_buffer[row - 1][col] = terminal_buffer[row][col];
        }
    }

    for (size_t col = 0; col < TERM_COLS; col++)
    {
        terminal_buffer[TERM_ROWS - 1][col].character = '#';
        terminal_buffer[TERM_ROWS - 1][col].foreground = 0x00FF00;
        terminal_buffer[TERM_ROWS - 1][col].background = 0x000000;
    }

    cursor_row = TERM_ROWS - 1;
    cursor_col = 0;

//     framebuffer_clear(0xFF0000);
//     for (;;)
//     {
//         __asm__ volatile("hlt");
//     }
}


void terminal_init(void)
{
    cursor_row = 0;
    cursor_col = 0;
    terminal_color = 0xFFFFFF;

    for (size_t r = 0; r < TERM_ROWS; r++)
    {
        for (size_t c = 0; c < TERM_COLS; c++)
        {
            terminal_buffer[r][c].character = ' ';
            terminal_buffer[r][c].foreground = 0xFFFFFF;
            terminal_buffer[r][c].background = 0x000000;
        }
    }
}

void terminal_putchar(char c)
{
    if (c == '\n')
    {
        cursor_col = 0;
        cursor_row++;

        if (cursor_row >= TERM_ROWS)
        {
            terminal_scroll();
        }

        return;
    }

    if (cursor_row >= TERM_ROWS)
    {
        terminal_scroll();
        return;
    }

    terminal_buffer[cursor_row][cursor_col].character = c;
    terminal_buffer[cursor_row][cursor_col].foreground = terminal_color;
    terminal_buffer[cursor_row][cursor_col].background = 0x000000;

    cursor_col++;

    if (cursor_col >= TERM_COLS)
    {
        cursor_col = 0;
        cursor_row++;

        if (cursor_row >= TERM_ROWS)
        {
            terminal_scroll();
        }
    }
}

void terminal_backspace(void)
{
    if (cursor_row == 0 && cursor_col == 0)
    {
        return;
    }

    if (cursor_col == 0)
    {
        cursor_row--;
        cursor_col = TERM_COLS;
    }

    cursor_col--;
    terminal_buffer[cursor_row][cursor_col].character = ' ';
    terminal_buffer[cursor_row][cursor_col].foreground = 0xFFFFFF;
    terminal_buffer[cursor_row][cursor_col].background = 0x000000;
    terminal_render();
}

void terminal_write(const char *str)
{
    while (*str)
    {
        terminal_putchar(*str);
        str++;
    }
    terminal_render();
}


void terminal_set_color(uint32_t color)
{
    terminal_color = color;
}

void terminal_render(void)
{
    framebuffer_clear(0x000000);

    for (size_t row = 0 ; row < TERM_ROWS; row ++)
    {
        for (size_t col = 0 ; col < TERM_COLS; col ++)
        {
            terminal_cell_t *cell = &terminal_buffer[row][col];

            if (cell->character == ' ')
                continue;

            draw_char(
                col * 16,
                row * 16,
                cell->character,
                cell->foreground
            );
        }
    }
}
