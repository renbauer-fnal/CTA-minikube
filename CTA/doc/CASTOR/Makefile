# Prerequisites:
# texlive 2016
# tikz-uml

TARGET       = TapeServer

PDF_TARGET   = $(patsubst %,%.pdf,$(TARGET))

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
