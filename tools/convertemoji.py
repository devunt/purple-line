#!/usr/bin/env python2.7
# coding:utf-8

# Converts the LINE desktop client emoji file into a C++ structure

import sys, csv

def hex_encode(data):
    return "".join(r"\x{0:02x}".format(ord(b)) for b in data)

print "#include \"emoji.hpp\""
print
print "// This file was generated with convertemoji.py"
print
print "const EmojiData emoji_data[] = {"

with open(sys.argv[1], "rb") as emoji_file:
    emoji_file.readline()

    reader = csv.reader(emoji_file, delimiter="\t", quoting=csv.QUOTE_NONE)

    for row in reader:
        if len(row) < 8:
            continue

        print "    {{ \"{0}\", \"{1}\", {2} }},".format(
            hex_encode(unichr(int(row[2], 16)).encode("utf-8")),
            hex_encode(row[5]),
            row[7])

print "};"
