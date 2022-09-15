/*
 * Copyright (c) 2022, lg & segfaultdev
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <gaia/font.h>
#include <gaia/term.h>

/* NOTE: this code here is ugly and is only temporary, it gets overwritten by the userspace tty */

typedef struct
{
    uint32_t *addr;
    size_t width, height;
    size_t pitch;
} Framebuffer;

typedef struct
{
    uint8_t val;
    bool empty;
} EscapeCode;
typedef struct
{
    Framebuffer fb;
    uint32_t fg, bg;
    size_t start_x, start_y;
    size_t cursor_x, cursor_y;
    uint32_t colors[8];
    EscapeCode stack[8];
    uint8_t index;
    uint8_t state;
} Term;

enum ansi_parsing_state
{
    PARSING_NONE,
    PARSING_BRACKET,
    PARSING_ATTR,
    PARSING_ENDVAL,
};

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

static inline void draw_rect_h(int x, int y, int w, int h, uint32_t color_1, uint32_t color_2)
{
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            uint32_t color_a = ((color_1 >> 0) & 0xFF) * ((w - 1) - j) + ((color_2 >> 0) & 0xFF) * j;
            uint32_t color_b = ((color_1 >> 8) & 0xFF) * ((w - 1) - j) + ((color_2 >> 8) & 0xFF) * j;
            uint32_t color_c = ((color_1 >> 16) & 0xFF) * ((w - 1) - j) + ((color_2 >> 16) & 0xFF) * j;
            uint32_t color_d = ((color_1 >> 24) & 0xFF) * ((w - 1) - j) + ((color_2 >> 24) & 0xFF) * j;

            color_a /= (w - 1);
            color_b /= (w - 1);
            color_c /= (w - 1);
            color_d /= (w - 1);

            putpixel(x + j, y + i, (color_a << 0) | (color_b << 8) | (color_c << 16) | (color_d << 24));
        }
    }
}

static inline void draw_rect_v(int x, int y, int w, int h, uint32_t color_1, uint32_t color_2)
{
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            uint32_t color_a = ((color_1 >> 0) & 0xFF) * ((h - 1) - i) + ((color_2 >> 0) & 0xFF) * i;
            uint32_t color_b = ((color_1 >> 8) & 0xFF) * ((h - 1) - i) + ((color_2 >> 8) & 0xFF) * i;
            uint32_t color_c = ((color_1 >> 16) & 0xFF) * ((h - 1) - i) + ((color_2 >> 16) & 0xFF) * i;
            uint32_t color_d = ((color_1 >> 24) & 0xFF) * ((h - 1) - i) + ((color_2 >> 24) & 0xFF) * i;

            color_a /= (h - 1);
            color_b /= (h - 1);
            color_c /= (h - 1);
            color_d /= (h - 1);

            putpixel(x + j, y + i, (color_a << 0) | (color_b << 8) | (color_c << 16) | (color_d << 24));
        }
    }
}

static inline void draw_circle(int x, int y, int radius, uint32_t color_1, uint32_t color_2)
{
    for (int i = y - radius; i <= y + radius; i++)
    {
        for (int j = x - radius; j <= x + radius; j++)
        {
            int dist = (j - x) * (j - x) + (i - y) * (i - y);
            int root = dist / 2;

            if (dist == 0)
            {
                root = 0;
            }
            else
            {
                if (root != 0)
                    root = (root + dist / root) / 2;
                if (root != 0)
                    root = (root + dist / root) / 2;
                if (root != 0)
                    root = (root + dist / root) / 2;
            }

            if (root > radius)
                continue;

            uint32_t color_a = ((color_1 >> 0) & 0xFF) * ((radius - 1) - root) + ((color_2 >> 0) & 0xFF) * root;
            uint32_t color_b = ((color_1 >> 8) & 0xFF) * ((radius - 1) - root) + ((color_2 >> 8) & 0xFF) * root;
            uint32_t color_c = ((color_1 >> 16) & 0xFF) * ((radius - 1) - root) + ((color_2 >> 16) & 0xFF) * root;
            uint32_t color_d = ((color_1 >> 24) & 0xFF) * ((radius - 1) - root) + ((color_2 >> 24) & 0xFF) * root;

            color_a /= (radius - 1);
            color_b /= (radius - 1);
            color_c /= (radius - 1);
            color_d /= (radius - 1);

            putpixel(j, i, (color_a << 0) | (color_b << 8) | (color_c << 16) | (color_d << 24));
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
            if ((glyph[y] >> x) & 1)
            {
                putpixel(pos_x + x, pos_y + y, color);
            }
        }
    }
}

static bool do_escape_code(char c)
{
    if (c == '\033')
    {
        term.index = 0;
        term.state = PARSING_BRACKET;
        term.stack[0].empty = true;
        term.stack[0].val = 0;
    }

    if (term.state == PARSING_BRACKET && c == '[')
    {
        term.state = PARSING_ATTR;
        return false;
    }

    if (term.state == PARSING_ATTR)
    {
        if (c >= '0' && c <= '9')
        {
            term.stack[term.index].val *= 10;
            term.stack[term.index].val += c - '0';
            term.stack[term.index].empty = false;
        }
        else
        {
            if (term.index < 8)
                term.index++;
            term.stack[term.index].val = 0;
            term.stack[term.index].empty = true;
            term.state = PARSING_ENDVAL;
        }
    }

    if (term.state == PARSING_ENDVAL)
    {
        if (c == ';')
        {
            term.state = PARSING_ATTR;
        }

        else if (c == 'm')
        {
            term.state = PARSING_NONE;

            for (int i = 0; i < term.index; i++)
            {

                if (!term.stack[i].empty)
                {
                    int attr = term.stack[i].val;
                    int color = 0;

                    if (attr == 1)
                    {
                        color = term.stack[++i].val;
                    }
                    else if (attr == 0)
                    {
                        term.fg = 0;
                    }

                    else if (attr >= 30 && attr <= 37)
                    {
                        color = term.stack[i].val;
                        term.fg = term.colors[color - 30];
                    }
                }
            }
            return false;
        }
    }

    return term.state == PARSING_NONE;
}

static void write_char(char c)
{

    if (c == '\n')
    {
        term.cursor_y++;
        term.cursor_x = 0;
        return;
    }

    if (do_escape_code(c))
    {
        plot_char(c, term.cursor_x * ISO_CHAR_WIDTH + term.start_x, term.cursor_y * ISO_CHAR_HEIGHT + term.start_y, term.fg);
        term.cursor_x++;
    }
}

#define SHADOW_COLOR 0x008C8C

void term_init(Charon *charon)
{
    const int margin = 40;

    term.fb.addr = (uint32_t *)charon->framebuffer.address;
    term.fb.width = charon->framebuffer.width;
    term.fb.height = charon->framebuffer.height;
    term.fb.pitch = charon->framebuffer.pitch;

    int start_x = margin;
    int start_y = margin;

    int width = term.fb.width - 2 * margin;
    int height = term.fb.height - 2 * margin;

    // Clear the screen
    draw_rect(0, 0, term.fb.width, term.fb.height, 0x00AAAA);

    // Shadow circles
    draw_circle(start_x - 1, start_y - 1, 16, SHADOW_COLOR, 0x00AAAA);
    draw_circle(start_x + width, start_y - 1, 16, SHADOW_COLOR, 0x00AAAA);
    draw_circle(start_x - 1, start_y + height, 16, SHADOW_COLOR, 0x00AAAA);
    draw_circle(start_x + width, start_y + height, 16, SHADOW_COLOR, 0x00AAAA);

    // Base window rectangle
    draw_rect(start_x, start_y, width, height, 0xAAAAAA);

    // Base window highlight
    draw_rect(start_x + 1, start_y + 1, width - 3, 1, 0xFFFFFF);
    draw_rect(start_x + 1, start_y + 2, 1, height - 4, 0xFFFFFF);

    // Base window dark part
    draw_rect(start_x + 1, start_y + (height - 2), width - 2, 1, 0x555555);
    draw_rect(start_x + (width - 2), start_y + 1, 1, height - 2, 0x555555);

    // Base window darker part
    draw_rect(start_x, start_y + (height - 1), width, 1, 0x000000);
    draw_rect(start_x + (width - 1), start_y, 1, height, 0x000000);

    // Title bar
    draw_rect(start_x + 4, start_y + 4, width - 8, 2 + ISO_CHAR_HEIGHT, 0x0000AA);

    // Title bar's text
    const char *s = "Gaia early console";
    int x = 0;

    while (*s)
    {
        plot_char(*s, x * ISO_CHAR_WIDTH + start_x + 6, start_y + 5, 0xFFFFFF);
        x++, s++;
    }

    int textbox_x = start_x + 4;
    int textbox_y = start_y + 8 + ISO_CHAR_HEIGHT;

    int textbox_width = width - 8;
    int textbox_height = height - (12 + ISO_CHAR_HEIGHT);

    // Text box rectangle
    draw_rect(textbox_x, textbox_y, textbox_width, textbox_height, 0xFFFFFF);

    // Text box dark part
    draw_rect(textbox_x, textbox_y, textbox_width - 1, 1, 0x555555);
    draw_rect(textbox_x, textbox_y + 1, 1, textbox_height - 2, 0x555555);

    // Text box darker part
    draw_rect(textbox_x + 1, textbox_y + 1, textbox_width - 3, 1, 0x000000);
    draw_rect(textbox_x + 1, textbox_y + 2, 1, textbox_height - 4, 0x000000);

    // Text box normal part
    draw_rect(textbox_x + 1, textbox_y + (textbox_height - 2), textbox_width - 2, 1, 0xAAAAAA);
    draw_rect(textbox_x + (textbox_width - 2), textbox_y + 1, 1, textbox_height - 2, 0xAAAAAA);

    // Super cute shadow
    draw_rect_h(start_x + width, start_y, 16, height, SHADOW_COLOR, 0x00AAAA);
    draw_rect_h(start_x - 16, start_y, 16, height, 0x00AAAA, SHADOW_COLOR);
    draw_rect_v(start_x, start_y + height, width, 16, SHADOW_COLOR, 0x00AAAA);
    draw_rect_v(start_x, start_y - 16, width, 16, 0x00AAAA, SHADOW_COLOR);

    term.cursor_x = 0;
    term.cursor_y = 0;

    term.start_x = textbox_x + 8;
    term.start_y = textbox_y + 4;

    term.fg = 0x000000;
    term.colors[0] = 0;
    term.colors[1] = 0x800000;
    term.colors[2] = 0x008000;
    term.colors[3] = 0x808000;
    term.colors[4] = 0x010081;
    term.colors[5] = 0x800080;
    term.colors[6] = 0x008080;
    term.colors[7] = 0xc0c0c0;
}

void term_write(const char *str)
{
    while (*str)
    {
        write_char(*str);
        str++;
    }
}
