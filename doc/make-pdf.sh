#!/bin/sh

groff -Tpdf -P-pa4 -t -mpdfmark -mandoc ../man/asm6809.1 > asm6809.pdf
