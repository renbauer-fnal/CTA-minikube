TARGET=OutlookForArchivalStorageAtCERN_MichaelDavis.pdf

IMAGES=$(wildcard images/*)

all: $(TARGET)

show: $(TARGET)
	evince $(TARGET)

# Need to compile with xelatex to get Avenir font

%.bbl: %.bib
	xelatex $(subst .bib$,.tex,$<) 2> $<.log
	bibtex $(subst .bib$,,$<)
	xelatex $(subst .bib$,.tex,$<)

%.pdf: %.tex %.bbl $(IMAGES) Makefile
	xelatex $<

clean:
	rm -f $(TARGET) *.aux *.bbl *.blg *.log *.dvi *.bak *.lof *.log *.lol *.lot *.out *.toc *.eps *.nav *.snm
