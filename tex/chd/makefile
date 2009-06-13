all: 
	latex chd.tex
	bibtex chd
	latex chd.tex
	latex chd.tex
	dvips chd.dvi -o chd.ps
run: clean all 
	gv chd.ps &
html: clean all
	latex2html chd.tex
clean:
	rm chd.dvi chd.ps *.lot *.lof *.aux *.bbl *.blg *.log *.toc
