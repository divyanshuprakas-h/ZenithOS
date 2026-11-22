#include "../graphics/graphics.h"
#include "../framebuffer/framebuffer.h"
#include "terminal.h"
#include "../font/font.h"
#include "terminal_buffer.h"
#include "../hal/io.h"
#include "../cpu/interrupts.h"

static size_t cursor_row = 0;
static size_t cursor_col = 0;

#define LINE_BUFFER_SIZE  256
#define STDIN_RING_SIZE   1024

static char line_buffer[LINE_BUFFER_SIZE];
static size_t line_len = 0;

static char stdin_ring[STDIN_RING_SIZE];
static size_t stdin_head = 0;
static size_t stdin_tail = 0;

static terminal_cell_t terminal_buffer[TERM_ROWS][TERM_COLS];

static uint32_t terminal_color = 0xFFFFFF;

static int terminal_origin_x = 0;
static int terminal_origin_y = 0;
static bool terminal_visible_flag = true;

void terminal_set_visible(bool visible)
{
    terminal_visible_flag = visible;
}

bool terminal_is_visible(void)
{
    return terminal_visible_flag;
}

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
        terminal_buffer[TERM_ROWS - 1][col].character = ' ';
        terminal_buffer[TERM_ROWS - 1][col].foreground = 0xFFFFFF;
        terminal_buffer[TERM_ROWS - 1][col].background = 0x000000;
    }

    cursor_row = TERM_ROWS - 1;
    cursor_col = 0;
}

void terminal_set_origin(int x, int y)
{
    terminal_origin_x = x;
    terminal_origin_y = y;
}

void terminal_init(void)
{
    cursor_row = 0;
    cursor_col = 0;
    terminal_color = 0xFFFFFF;
    line_len = 0;
    stdin_head = 0;
    stdin_tail = 0;

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

void terminal_clear(void)
{
    cursor_row = 0;
    cursor_col = 0;

    for (size_t r = 0; r < TERM_ROWS; r++)
    {
        for (size_t c = 0; c < TERM_COLS; c++)
        {
            terminal_buffer[r][c].character = ' ';
            terminal_buffer[r][c].foreground = 0xFFFFFF;
            terminal_buffer[r][c].background = 0x000000;
        }
    }

    terminal_render();
}

void terminal_putchar(char c)
{
    if (c == '\f')
    {
        terminal_clear();
        return;
    }

    if (c == '\r')
    {
        cursor_col = 0;
        return;
    }

    if (c == '\b')
    {
        terminal_backspace();
        return;
    }

    if (c == '\t')
    {
        cursor_col = (cursor_col + 4) & ~3;
        if (cursor_col >= TERM_COLS)
        {
            cursor_col = 0;
            cursor_row++;
            if (cursor_row >= TERM_ROWS)
            {
                terminal_scroll();
            }
        }
        return;
    }

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
    if (!terminal_visible_flag)
    {
        return;
    }

    fill_rect(
        terminal_origin_x,
        terminal_origin_y,
        TERM_COLS * 16,
        TERM_ROWS * 16,
        0x000000
    );

    for (size_t row = 0; row < TERM_ROWS; row++)
    {
        for (size_t col = 0; col < TERM_COLS; col++)
        {
            terminal_cell_t *cell = &terminal_buffer[row][col];

            if (cell->character == ' ')
                continue;

            draw_char(
                terminal_origin_x + col * 16,
                terminal_origin_y + row * 16,
                cell->character,
                cell->foreground
            );
        }
    }
}

bool terminal_stdin_has_data(void)
{
    return stdin_head != stdin_tail;
}

int terminal_stdin_read(char *buf, size_t max_len)
{
    if (buf == NULL || max_len == 0 || stdin_head == stdin_tail)
    {
        return 0;
    }

    bool irq_was_enabled = interrupt_save();

    size_t count = 0;
    while (count < max_len && stdin_head != stdin_tail)
    {
        buf[count++] = stdin_ring[stdin_head];
        stdin_head = (stdin_head + 1) % STDIN_RING_SIZE;
    }

    interrupt_restore(irq_was_enabled);
    return (int)count;
}

void terminal_handle_key(char c)
{
    if (c == '\b')
    {
        if (line_len > 0)
        {
            line_len--;
            terminal_backspace();
            outb(0xE9, '\b');
            outb(0xE9, ' ');
            outb(0xE9, '\b');
        }
        return;
    }

    if (c == '\n')
    {
        terminal_putchar('\n');
        outb(0xE9, '\n');
        terminal_render();

        bool irq_was_enabled = interrupt_save();

        for (size_t i = 0; i < line_len; i++)
        {
            size_t next = (stdin_tail + 1) % STDIN_RING_SIZE;
            if (next != stdin_head)
            {
                stdin_ring[stdin_tail] = line_buffer[i];
                stdin_tail = next;
            }
        }

        size_t next = (stdin_tail + 1) % STDIN_RING_SIZE;
        if (next != stdin_head)
        {
            stdin_ring[stdin_tail] = '\n';
            stdin_tail = next;
        }

        line_len = 0;
        interrupt_restore(irq_was_enabled);
        return;
    }

    if ((c >= ' ' && c <= '~') || c == '\t')
    {
        if (line_len + 1 < LINE_BUFFER_SIZE)
        {
            line_buffer[line_len++] = c;
            terminal_putchar(c);
            outb(0xE9, (uint8_t)c);
            terminal_render();
        }
        return;
    }
}

const char *terminal_get_input(void)
{
    return line_buffer;
}
