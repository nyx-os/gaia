import toml 
import sys
import argparse
import os

parser = argparse.ArgumentParser(
                    prog='alacritty2fb',
                    description='Converts alacritty YAML colorschemes to C framebuffer values')

parser.add_argument('filename')
parser.add_argument('-d', '--directory', help="Directory of installed https://github.com/alacritty/alacritty-theme")

args = parser.parse_args()


name2index = {
    'black': 0,
    'red': 1,
    'green': 2,
    'yellow': 3,
    'blue': 4,
    'magenta': 5,
    'cyan': 6,
    'white': 7,
}

filename = args.filename

if args.directory:
    filename = os.path.join(os.path.join(args.directory, 'themes'), filename)

with open(filename, 'r') as file:
    data = toml.load(file)
    
    print(Rf"""static struct {{
  uint32_t default_fg;
  uint32_t default_bg;
}} colorscheme = {{{data['colors']['primary']['foreground'].replace('#', '0x')}, {data['colors']['primary']['background'].replace('#', '0x')}}};
    """)

    normal_arr = []
    bright_arr = []
    
    for name, color in data['colors']['normal'].items():
        normal_arr.insert(name2index[name], color.replace('#', '0x'))

    if 'bright' in data['colors']:
        for name, color in data['colors']['bright'].items():
            bright_arr.insert(name2index[name], color.replace('#', '0x'))
    else:
        bright_arr = normal_arr


    print(f"static uint32_t ansi_colors[] = {{{', '.join(normal_arr)}}};");
    print(f"static uint32_t ansi_bright_colors[] = {{{', '.join(bright_arr)}}};");
