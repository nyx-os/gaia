static struct {
  uint32_t default_fg;
  uint32_t default_bg;
} colorscheme = {0xC0C7C8, 0x000000};

static uint32_t ansi_colors[] = {0x000000, 0xA80000, 0x00A800, 0xA85400,
                                 0x0000A8, 0xA800A8, 0x00A8A8, 0xA8A8A8};
static uint32_t ansi_bright_colors[] = {0x545454, 0xFC5454, 0x54FC54, 0xFCFC54,
                                        0x5454FC, 0xFC54FC, 0x54FCFC, 0xFFFFFF};
