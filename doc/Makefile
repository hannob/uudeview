#
# $Id: Makefile,v 1.2 1996/06/15 11:49:05 fp Exp $
#

DVI	=	library.dvi
PS	=	${DVI:.dvi=.ps}


LATEX	=	latex
DVIPS	=	dvips
FIG2DEV	=	fig2dev


.SUFFIXES:
.SUFFIXES: .ltx .dvi .ps .fig .tex .eps


all:	$(DVI)
dvi:	$(DVI)
ps:	$(PS)


library.ps:		library.dvi
library.dvi:		library.ltx structure.tex binhex.tex

clean:
	rm -f *.aux *.log *.toc *~
	rm -f *.o td-v1 td-v2 td-v3

distclean: clean
	for fig in *.fig ; do \
		rm -f `basename $$fig .fig`.eps ; \
		rm -f `basename $$fig .fig`.tex ; \
	done
	rm -f *.dvi *.ps

.ltx.dvi:
	$(LATEX) $<
	$(LATEX) $<

.dvi.ps:
	$(DVIPS) -f < $< > $@

.fig.tex:
	$(FIG2DEV) -L latex < $< > $@

.fig.eps:
	$(FIG2DEV) -L ps < $< > $@

