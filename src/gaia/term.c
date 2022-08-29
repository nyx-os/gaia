/*
 * Copyright (c) 2022, lg
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/font.h>
#include <gaia/term.h>

typedef struct
{
    uint32_t *addr;
    size_t width, height;
    size_t pitch;
} Framebuffer;

typedef struct
{
    Framebuffer fb;
    uint32_t fg, bg;
    size_t cursor_x, cursor_y, cursor_x_initial;
    uint32_t colors[8];
} Term;

static Term term = {0};

static inline uint8_t *get_glyph(char c)
{
    size_t char_size = ((ISO_CHAR_HEIGHT * ISO_CHAR_WIDTH) / 8);

    return &font_data[char_size * c];
}

static inline void putpixel(int x, int y, uint32_t color)
{
    size_t fb_i = x + (term.fb.pitch / sizeof(uint32_t)) * y;

    term.fb.addr[fb_i] = color;
}

static inline void draw_rect(int x, int y, int w, int h, uint32_t color)
{
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            putpixel(x + j, y + i, color);
        }
    }
}

static void plot_char(char c, int pos_x, int pos_y, uint32_t color)
{
    uint8_t *glyph = get_glyph(c);

    for (int y = 0; y < ISO_CHAR_HEIGHT; y++)
    {
        for (int x = 0; x < ISO_CHAR_WIDTH; x++)
        {
            if (glyph[y] >> x & 1)
            {
                putpixel(pos_x * ISO_CHAR_WIDTH + x, pos_y * ISO_CHAR_HEIGHT + y, color);
            }
        }
    }
}

static void write_char(char c)
{

    if (c == '\n')
    {
        term.cursor_y++;
        term.cursor_x = term.cursor_x_initial;
        return;
    }

    plot_char(c, term.cursor_x, term.cursor_y, term.fg);

    term.cursor_x++;
}

void term_init(Charon *charon)
{
    int margin = 40;
    int border_width = 4;
    int border_margin = 40 - border_width;

    term.fb.addr = (uint32_t *)charon->framebuffer.address;
    term.fb.width = charon->framebuffer.width;
    term.fb.height = charon->framebuffer.height;
    term.fb.pitch = charon->framebuffer.pitch;

    int term_width = term.fb.width - margin * 2;
    int term_height = term.fb.height - margin * 2;

    for (size_t y = 0; y < term.fb.height; y++)
    {
        for (size_t x = 0; x < term.fb.width; x++)
        {
            putpixel(x, y, 0x008080);
        }
    }

    // Top line
    draw_rect(border_margin, border_margin, term.fb.width - border_margin * 2, border_width, 0xC0C0C0);

    // Left line
    draw_rect(border_margin, margin, border_width, term_height, 0xC0C0C0);

    // Right line
    draw_rect(term.fb.width - margin, margin, border_width, term_height, 0xC0C0C0);

    // Bottom line
    draw_rect(border_margin, term.fb.height - margin, term.fb.width - border_margin * 2, border_width, 0xC0C0C0);

    draw_rect(margin, margin, term_width, term_height, 0x00000);

    // Bar
    draw_rect(margin, margin, term_width, 30, 0x010080);

    // Bar's bottom line
    draw_rect(border_margin, border_margin + 30 + 1, term.fb.width - border_margin * 2, border_width - 1, 0xC0C0C0);

    // Bar's text
    const char *s = "Gaia early console";
    int x = (margin / ISO_CHAR_WIDTH) + 1;
    while (*s)
    {
        plot_char(*s, x, (margin + 15) / ISO_CHAR_HEIGHT, 0xffffffff);
        x++;
        s++;
    }

    term.cursor_y = (margin + 30) / ISO_CHAR_HEIGHT + 1;
    term.cursor_x = margin / ISO_CHAR_WIDTH + 1;
    term.cursor_x_initial = margin / ISO_CHAR_WIDTH + 1;

    term.fg = 0xffffffff;

    term_write("Hello world!\n");
    term_write("Hello world!\n");
    term_write("Hello world!\n");
    term_write("Hello world!\n");
}

void term_write(const char *str)
{
    while (*str)
    {
        write_char(*str);
        str++;
    }
}
