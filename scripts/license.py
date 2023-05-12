#!/usr/bin/python3

import sys

string = "/* SPDX-License-Identifier: {} */\n"

license_names = {
    "bsd2": "BSD-2-Clause",
    "mit": "MIT",
    "gpl2": "GPL-2.0",
    "gpl3": "GPL-3.0",
    "unlicense": "Unlicense",
}

if len(sys.argv) > 1:
    for i in sys.argv[1:]:
        with open(i, 'r+') as f:
            line = f.readline()
            if line[:2] == "/*" and line.find("*/") != -1:
                comment_str = line[2:line.find("*/")].strip().split(':')

                if comment_str[0][0] != '@':
                    continue

                if comment_str[0][1:] == "license":
                    if comment_str[1] == "none" or comment_str[1] == "ignore":
                        continue

                    s = f.read()
                    f.seek(0, 0)
                    f.write(string.format(license_names[comment_str[1]]) + s)
