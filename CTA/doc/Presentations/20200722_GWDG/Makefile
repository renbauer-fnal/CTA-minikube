TARGET=CTA_GWDG.pdf

IMAGES=$(wildcard images/*)

all: $(TARGET)

show: $(TARGET)
	evince $(TARGET)

# Need to compile with xelatex to get Avenir font

%.pdf: %.tex $(IMAGES) Makefile
	xelatex $<
	xelatex $<

clean:
	rm -f $(TARGET) *.aux *.bbl *.blg *.log *.dvi *.bak *.lof *.log *.lol *.lot *.out *.toc *.eps *.nav *.snm
