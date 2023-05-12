#include "term.h"
#include "flanterm.h"
#include "fb.h"

struct flanterm_context *ctx;

void term_init(charon_t charon)
{
    struct {
        uint32_t default_fg;
        uint32_t default_bg;
    } colorscheme = { 0xe6e1cf, 0x0f1419 };

    uint32_t ansi_colors[] = {
        0x00000000, // black
        0xf34a4a, // red
        0xbae67e, // green
        0xffee99, // yellow
        0x73d0ff, // blue
        0xd4bfff, // magenta
        0x83CEC6, // cyan
        0xf2f2f2, // grey
    };

    uint32_t ansi_bright_colors[] = {
        0x737d87, // black
        0xff3333, // red
        0xc2d94c, // green
        0xe7c547, // yellow
        0x59c2ff, // blue
        0xb77ee0, // magenta
        0x5ccfe6, // cyan
        0xffffff, // grey
    };

    ctx = flanterm_fb_init(kmem_alloc, (uint32_t *)charon.framebuffer.address,
                           charon.framebuffer.width, charon.framebuffer.height,
                           charon.framebuffer.pitch, NULL, ansi_colors,
                           ansi_bright_colors, &colorscheme.default_bg,
                           &colorscheme.default_fg, NULL, NULL, NULL, 16, 8, 10,
                           1, 1, 0);
}

void term_write(const char *str)
{
    flanterm_write(ctx, str, strlen(str));
}
