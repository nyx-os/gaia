#include "term.h"
#include "flanterm.h"
#include "fb.h"

struct flanterm_context *ctx;

void term_init(charon_t charon)
{
    struct {
        uint32_t default_fg;
        uint32_t default_bg;
    } colorscheme = { 0xe6e1cf, 0x0A0E14 };

    uint32_t ansi_colors[] = {
        0x01060E, // black
        0xEA6C73, // red
        0x91B362, // green
        0xF9AF4F, // yellow
        0x53BDFA, // blue
        0xFAE994, // magenta
        0x90E1C6, // cyan
        0xC7C7C7, // grey
    };

    uint32_t ansi_bright_colors[] = {
        0x686868, // black
        0xF07178, // red
        0xC2D94C, // green
        0xFFB454, // yellow
        0x59C2FF, // blue
        0xFFEE99, // magenta
        0x95E6CB, // cyan
        0xFFFFFF, // grey
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
