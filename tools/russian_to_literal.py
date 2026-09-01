#!/usr/bin/env python3
import sys

def to_hex_escapes(s, encoding='cp1251'):
    encoded = s.encode(encoding)
    return ''.join(f'\\x{b:02X}' for b in encoded)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        text = sys.argv[1]
        print(to_hex_escapes(text))
    else:
        while True:
            text = input("Type russian string: ")
            print(to_hex_escapes(text))

