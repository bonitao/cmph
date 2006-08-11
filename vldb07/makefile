all: 
	latex vldb.tex 
	bibtex vldb
	latex vldb.tex 
	latex vldb.tex
	dvips vldb.dvi -o vldb.ps
	ps2pdf vldb.ps
	chmod -R g+rwx *

perm:
	chmod -R g+rwx *

run: clean all 
	gv vldb.ps &
clean:
	rm *.aux *.bbl *.blg *.log *.ps *.pdf *.dvi

