all: 
	latex bdz.tex
	bibtex bdz
	latex bdz.tex
	latex bdz.tex
	dvips bdz.dvi -o bdz.ps
run: clean all 
	gv bdz.ps &
html: clean all
	latex2html bdz.tex
clean:
	rm bdz.dvi bdz.ps *.lot *.lof *.aux *.bbl *.blg *.log *.toc
