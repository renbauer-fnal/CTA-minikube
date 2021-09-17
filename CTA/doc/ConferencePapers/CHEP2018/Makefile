TARGET = CHEP2018_CTA.pdf
BIBLIO = CHEP2018_CTA.bib
IMAGES = $(wildcard images/*)

TEXINPUTS = .:./webofc:
BSTINPUTS = .:./webofc:
LATEX_TMP = *.aux *.bbl *.blg *.log *.dvi *.bak *.lof *.log *.lol *.lot *.out *.toc savedrecs.bib

all: $(TARGET)

%.pdf: %.tex %.bbl $(IMAGES)
	pdflatex $<

refs:
	@touch $(BIBLIO)

%.bbl: %.bib
	pdflatex $(subst .bib$,.tex,$<) 2> $<.log
	bibtex $(subst .bib$,,$<)
	pdflatex $(subst .bib$,.tex,$<)

# Phony targets

show: $(TARGET)
	evince $(TARGET)

clean:
	rm -f $(LATEX_TMP)

clobber: clean
	rm -f $(TARGET)

.EXPORT_ALL_VARIABLES:

.INTERMEDIATE: $(LATEX_TMP)

.PHONY: all show clean clobber
