CC = icc
CXX = icpc
CFLAGS = -Wall -std=gnu99 -openmp -O3 -g -I.
CXXFLAGS = -Wall -openmp -O3 -g
#overkill on the flags, but that doesn't hurt anything
LDFLAGS = -lrt -lpthread
#replace this if you want to change the output name
CTARGET = myar
TEXTARGET = writeup

# LaTeX related constants
HTML=main_html
SRC=mt.h

#any headers go here
INCLUDES = 

#any .c or .cpp files go here
SOURCE = myar.c

#default is to compile and pdf
default: pdf compile
both: pdf html

#depends on all of you source and header files
compile: ${SOURCE} ${INCLUDES}
#this assumes you actually are linking all of the source files together
	${CC} ${CFLAGS} ${SOURCE} -o ${CTARGET} ${LDFLAGS}

debug: ${SOURCE} ${INCLUDES}
	${CC} ${CFLAGS} ${SOURCE} -o ${CTARGET} ${LDFLAGS} -DDEBUG

dvi: ${TEXTARGET}.tex 
#	pygmentize the input source file -- THIS NAME SHOULD BE SAFE
	pygmentize -f latex -o __${SRC}.tex ${SRC}
#	run latex twice to get references correct
	latex ${TEXTARGET}.tex
#	you can also have a bibtex line here
#	bibtex $(TEXTARGET).tex
	latex $(TEXTARGET).tex
#	remove the pygmentized output to avoid cluttering up the directory
	rm __${SRC}.tex

ps: dvi
	dvips -R -Poutline -t letter ${TEXTARGET}.dvi -o ${TEXTARGET}.ps

pdf: ps
	ps2pdf ${TEXTARGET}.ps


html:
	cp ${TEXTARGET}.tex ${HTML}.tex
	latex ${HTML}.tex
	latex2html -split 0 -noshow_section_numbers -local_icons -no_navigation -noinfo -noaddress ${HTML}

	sed 's/<BR><HR>//g' < ${HTML}/index.html > ${HTML}/index2.html
	mv ${HTML}/index2.html ${TEXTARGET}.html
	rm ${HTML}.*
	rm -rf ${HTML}

