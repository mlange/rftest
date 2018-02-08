# Generated automatically from Makefile.in by configure.
#
# SOURCE FILE: Makefile.in
#
# DESCRIPTION: Makefile for the RF test utility to be used with configure.
#

SHELL = /bin/ksh

MSGFMT   = /usr/bin/msgfmt
XGETTEXT = /usr/bin/xgettext

CC       = cl
CPPFLAGS = 
CFLAGS   = -c
LDFLAGS  = -O
LIBS     =

# 
# Targets.
#

all: rftest termid rftest.mo
	@echo
	@echo "The RF test utility has been built."
	@echo "Now you can do a 'make install'."
	@echo

install: all
	install-sh rftest    ../bin
	install-sh termid    ../bin
	install-sh rftest.mo ../C/LC_MESSAGES

clean:
	rm -f rftest.o termid.o termid_main.o rftest rftest.mo termid timetest.o timetest

realclean: clean
	rm -f config.cache config.log

timetest: timetest.o
	$(CC) $(LDFLAGS) -o $@ timetest.o $(LIBS)

rftest:	rftest.o termid.o
	$(CC) $(LDFLAGS) -o $@ rftest.o termid.o $(LIBS)

termid: termid_main.o termid.o
	$(CC) $(LDFLAGS) -o $@ termid_main.o termid.o $(LIBS)

rftest.mo:
	$(XGETTEXT) -drftest -m"" rftest.c 
	$(MSGFMT) -orftest.mo rftest.po

rftest.o: config.h termid.h proto.h vendors.h

termid_main.o: config.h termid.h proto.h vendors.h

termid.o: config.h termid.h proto.h vendors.h

timetest.o:   

#
# Suffix rules.
#

.SUFFIXES: .c.o

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<
