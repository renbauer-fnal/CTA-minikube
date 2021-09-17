# Prerequisites:
# texlive 2016
# tikz-uml

TARGET       = cta queueRequestSeqDiags The_Tape_Storage_Element

PDF_TARGET   = $(patsubst %,%.pdf,$(TARGET))

HTML_TARGET  = html/cta.html #$(patsubst %,html/%.html,$(TARGET))

all: $(PDF_TARGET)
	cp *.pdf ..

LATEX_TMP    = $(patsubst %,%.4ct,$(TARGET)) \
               $(patsubst %,%.4tc,$(TARGET)) \
               $(patsubst %,%.aux,$(TARGET)) \
               $(patsubst %,%.css,$(TARGET)) \
               $(patsubst %,%.dvi,$(TARGET)) \
               $(patsubst %,%.glg,$(TARGET)) \
               $(patsubst %,%.glo,$(TARGET)) \
               $(patsubst %,%.gls,$(TARGET)) \
               $(patsubst %,%.glsdefs,$(TARGET)) \
               $(patsubst %,%.html,$(TARGET)) \
               $(patsubst %,%.idv,$(TARGET)) \
               $(patsubst %,%.ist,$(TARGET)) \
               $(patsubst %,%.lg,$(TARGET)) \
               $(patsubst %,%.log,$(TARGET)) \
               $(patsubst %,%.out,$(TARGET)) \
               $(patsubst %,%.png,$(TARGET)) \
               $(patsubst %,%.svg,$(TARGET)) \
               $(patsubst %,%.tmp,$(TARGET)) \
               $(patsubst %,%.toc,$(TARGET)) \
               $(patsubst %,%.xref,$(TARGET)) \
               texput.log

# Generic targets

%.pdf: %.tex
	pdflatex $<
	pdflatex $<

%.html: $(wildcard *.tex)
	htlatex $(patsubst %.html,%.tex,$@)
	mv *.html *.css *.svg *.png ../html

# Specific targets and rules for cta.pdf

cta.pdf: cta.glg
	pdflatex cta.tex

cta.aux: $(wildcard cta*.tex) cta.cls
	pdflatex cta.tex

cta.glg: $(wildcard cta*.tex)
	pdflatex cta.tex
	makeglossaries cta.glo

CTA_Admin.pdf: $(wildcard CTA_Admin*.tex) cta.cls
	pdflatex CTA_Admin.tex
	pdflatex CTA_Admin.tex

EOSCTA.pdf: $(wildcard EOSCTA*.tex) cta.cls
	pdflatex EOSCTA.tex
	pdflatex EOSCTA.tex

# Phony targets

clean:
	rm -f $(LATEX_TMP)
	rm -f $(patsubst %,%.pdf,$(TARGET))

clobber: clean
	cd ..; \
	rm -f $(patsubst %,%.pdf,$(TARGET)); \
	rm -f html/*

.INTERMEDIATE: $(LATEX_TMP)

.PHONY: all clean clobber
