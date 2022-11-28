# Makefile for NN version of mini inews.
#
# $RCSfile: Makefile,v $	$Revision: 1.1.1.1 $
#
# $Author: fp $	$Date: 1996/06/06 19:41:07 $
#
# $State: Exp $	$Locker:  $
#
# $Log: Makefile,v $
# Revision 1.1.1.1  1996/06/06 19:41:07  fp
# UUDeview put under revision control
#
# Revision 1.3  89/12/21  17:59:52  news
# Added kit processing, cleanups.
#
# This makefile needs the definitions of NNTPSERVER, DOMAIN and HIDDENNET
# on the make command line. __FP__
#

TARGET	= minews

BINDIR	= .
SHELL	= /bin/sh

SRCS	= inews.c clientlib.c version.c
OBJS	= inews.o clientlib.o version.o
CFLAGS	= -DNNTPSERVER=\"$(NNTPSERVER)\" -DDOMAIN=\"$(DOMAIN)\" \
	  -DHIDDENNET=$(HIDDENNET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) core lint tags *~

clobber:	clean
	rm -f $(TARGET) Make.Log Manifest $(KIT)

install:	$(TARGET)
	../inst inews $(TARGET)
#	install -o news -g news -m 511 -s -c $(TARGET) $(BINDIR)

