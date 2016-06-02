.PHONY: all
all: asm6809.pdf asm6809.txt

asm6809.1: manpage-header asm6809.html
	(cat manpage-header; hxnormalize -xe -l 99999 asm6809.html | hxpipe | ./html2man.pl) > $@

asm6809.pdf: asm6809.1
	./man2pdf.sh $< $@

asm6809.txt: asm6809.1
	./man2txt.sh $< $@

.PHONY: all
clean:
	rm -f asm6809.1 asm6809.pdf asm6809.txt
