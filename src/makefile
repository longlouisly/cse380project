
# The following three statements determine the build configuration.
# For handling different image formats, the program can be linked with
# the libjpeg, libpng, and libtiff libraries.  For each library, set
# the flags needed for linking.  To disable use of a library, comment
# its statement.  You can disable all three (BMP is always supported).
LDLIBJPEG=-ljpeg
LDLIBPNG=-lpng -lz
LDLIBTIFF=-ltiff

## Other libraries and include directory
INCLUDE_DIR=../include
LDLIBGRVY=-lgrvy

# Uncomment this line to perform computations in single precision
# instead of double precision.
NUM_SINGLE = -DNUM_SINGLE

##
# Standard make settings
CC=g++
CFLAGS=-O0 -ansi -pedantic -Wall -Wextra -I$(TACC_GRVY_INC) -I$(INCLUDE_DIR) $(NUM_SINGLE)
LDFLAGS=-L$(TACC_GRVY_LIB)
LDLIB=-lm $(LDLIBFFTW3) $(LDLIBJPEG) $(LDLIBPNG) $(LDLIBTIFF) $(LDLIBGRVY)

CHANVESE_SOURCES=chanvesecli.c chanvese.c cliio.c \
imageio.c basic.c gifwrite.c rgb2ind.c 

ARCHIVENAME=chanvese_$(shell date -u +%Y%m%d)
SOURCES=chanvesecli.c chanvese.c chanvese.h cliio.c cliio.h \
imageio.c imageio.h gifwrite.c gifwrite.h rgb2ind.c rgb2ind.h \
basic.c basic.h num.h 

##
# These statements add compiler flags to define USE_LIBJPEG, etc.,
# depending on which libraries have been specified above.
ifneq ($(LDLIBJPEG),)
	CJPEG=-DUSE_LIBJPEG
endif
ifneq ($(LDLIBPNG),)
	CPNG=-DUSE_LIBPNG
endif
ifneq ($(LDLIBTIFF),)
	CTIFF=-DUSE_LIBTIFF
endif

ALLCFLAGS=$(CFLAGS) $(CJPEG) $(CPNG) $(CTIFF)
CHANVESE_OBJECTS=$(CHANVESE_SOURCES:.c=.o)
.SUFFIXES: .c .o
.PHONY: all clean rebuild srcdoc dist dist-zip

all: chanvese compare

chanvese: $(CHANVESE_OBJECTS)
	$(CC) $(LDFLAGS) $(CHANVESE_OBJECTS) $(LDLIB) -o ../$@

# Special syntax to take all .c and create .o files
.c.o:
	$(CC) -c $(ALLCFLAGS) $< -o $@

compare: compare.o
	$(CC) $(LDFLAGS) compare.o imageio.o basic.o $(LDLIB) -o ../$@

check: chanvese compare test/test.sh
	test/test.sh

clean:
	$(RM) $(CHANVESE_OBJECTS) compare.o 

rebuild: clean all

srcdoc:
	doxygen doxygen.conf

dist: $(SOURCES)
	-rm -rf $(ARCHIVENAME)
	mkdir $(ARCHIVENAME)
	ln $(SOURCES) $(ARCHIVENAME)
	tar vchzf $(ARCHIVENAME).tgz $(ARCHIVENAME)
	-rm -rf $(ARCHIVENAME)

dist-zip: $(SOURCES)
	-rm -rf $(ARCHIVENAME)
	mkdir $(ARCHIVENAME)
	ln $(SOURCES) $(ARCHIVENAME)
	-rm -f $(ARCHIVENAME).zip
	zip -r9 $(ARCHIVENAME).zip $(ARCHIVENAME)/*
	-rm -rf $(ARCHIVENAME)
