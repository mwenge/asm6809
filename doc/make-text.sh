#!/bin/sh

groff -Tascii -P-bcou -t -mpdfmark -mandoc ../man/asm6809.1 > asm6809.txt
