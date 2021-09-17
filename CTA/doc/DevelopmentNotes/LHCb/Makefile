TARGETS   = atlas20180703.pdf cta20180820.pdf gfal.pdf lhcb20180704.pdf lhcb20180808.pdf lhcb20180817.pdf
LATEX_TMP = *.aux *.bbl *.blg *.log *.dvi *.bak *.lof *.log *.lol *.lot *.out *.toc

all: $(TARGETS)

%.pdf: %.tex
	pdflatex $<
	pdflatex $<

# Phony targets

clean:
	rm -f $(LATEX_TMP)

clobber: clean
	rm -f $(TARGETS)

.INTERMEDIATE: $(LATEX_TMP)

.PHONY: all clean clobber

