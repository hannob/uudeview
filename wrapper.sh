#! /bin/sh
#
# This is a super-duper do-it-all SFX-install script for uudeview.
# $Id: wrapper.sh,v 1.21 2004/03/01 23:06:18 fp Exp $
#
VERSION=0.5
PATCH=20
#
echo
echo ----------------------------------------------------------------------
echo                    UUDeview for Unix Installation
echo ----------------------------------------------------------------------
echo
echo You are about to install UUDeview for Unix, the friendly decoding
echo software. Usually, this works absolutely painlessly, but still you
echo should be ready to answer a few questions about you and your system.
echo
/bin/echo -n "Do you want to continue [y] "
read cont
if test "x$cont" != "x" && test "x$cont" != "xy" && test "x$cont" != "xyes" ; then
	echo
	echo Very well. Installation aborted.
	echo
	exit 0
fi

rm -f uudeview.tgz
if test ! -r uudeview.tgz ; then
	echo
	/bin/echo -n extracting source ...
	dd if=$0 of=uudeview.tgz bs=1k skip=12 2> /dev/null
	if test ! -s uudeview.tgz ; then
		echo error
		echo 
		echo Oops, couldn\'t extract source. The archive is probably broken. Please
		echo download this file once more and try again. Sorry.
		echo
		exit 1
	fi
	echo " done."
fi

#
# test if we have gunzip
#
/bin/echo -n checking for gunzip ... 
if gzip --version > /dev/null 2> /dev/null ; then
	echo " ok."
else
	echo " not found."
	echo
	echo It appears that you don\'t have gunzip installed. We need gunzip
	echo to extract the UUDeview archive. I strongly suggest to install
	echo the gunzip program.
	echo Or if you are sure that gunzip is installed, make sure it can
	echo be found in the \$PATH
	echo
	echo I\'m sorry, I cannot continue installation without gunzip.
	echo
	rm -rf uudeview.tgz uudeview.tar uudeview-"$VERSION"."$PATCH"
	exit 1
fi

/bin/echo -n uncompressing source ...

rm -f uudeview.tar
gzip -dc uudeview.tgz > uudeview.tar

if test "$?" != "0" || test ! -r uudeview.tar ; then
	echo " error."
	echo
	echo Oops, uncompressing the source code failed. The archive is
	echo probably broken. Please download this file once more and
	echo try again.
	echo
	echo Sorry, I cannot continue the installation.
	echo
	rm -rf uudeview.tgz uudeview.tar uudeview-*
	exit 1
else
	echo " done."
fi

/bin/echo -n extracting files from archive ...

tar xf uudeview.tar

if test "$?" != "0" || test ! -d uudeview-"$VERSION"."$PATCH" ; then
	echo " error."
	echo
	echo Oops, couldn\'t extract the archive file. This is weird.
	echo Please download this file once more and try again.
	echo
	echo Sorry, I cannot continue the installation.
	echo
	rm -rf uudeview.tgz uudeview.tar uudeview-"$VERSION"."$PATCH"
	exit 1
else
	rm -f uudeview.tar
	echo " done."
fi

cd uudeview-"$VERSION"."$PATCH"

if test "$?" != "0" ; then
	echo
	echo OOps. Could not change directory to uudeview-"$VERSION"."$PATCH". Weird.
	echo
	echo Sorry, I cannot continue the installation.
	echo
	rm -rf uudeview.tgz uudeview.tar uudeview-"$VERSION"."$PATCH"
	exit 1
fi

if test ! -x ./configure ; then
	echo
	echo OOps. Could not find ./configure. Weird.
	echo
	echo Sorry, I cannot continue the installation.
	echo
	cd ..
	rm -rf uudeview.tgz uudeview.tar uudeview-"$VERSION"."$PATCH"
	exit 1
fi

echo
echo ----------------------------------------------------------------------
echo We will now configure UUDeview for your needs.
echo ----------------------------------------------------------------------
echo

#
# check whether we are root
#

if test "x$LOGNAME" != "x" ; then
	whoami=$LOGNAME
else
	whoami=`id | sed 's/[^(]*(\([^)]*\)).*/\1/' 2>/dev/null`
	if test "x$whoami" = "x" ; then
		whoami=unknown
	fi
fi

if test "$whoami" = "root" ; then
	echo You are logged in as root. We should thus attempt a global
	echo installation of UUDeview. The default installation prefix
	echo is /usr/local, causing the binaries to go to /usr/local/bin
	echo and the manual pages to /usr/local/man.
	echo
	/bin/echo -n "Installation prefix [/usr/local] "
	read prefix
	if test "x$prefix" = "x" ; then
		prefix=/usr/local
	fi
else
	echo
	echo You are logged in as a plain user. You will probably want
	echo to install UUDeview locally, in your home directory. The
	echo installation prefix thus defaults to \$HOME, causing the
	echo binaries to go to \$HOME/bin and the manual pages to
	echo \$HOME/man. You can also choose not to install the manual
	echo pages.
	echo
	/bin/echo -n "Installation prefix [$HOME] "
	read prefix
	if test "x$prefix" = "x" ; then
		prefix=$HOME
	fi
	/bin/echo -n "Do you want manual pages [y] "
	read manuals
	if test "x$manuals" != "x" && test "x$manuals" != "xy" && test "x$manuals" != "xyes" ; then
		manopt="--disable-manuals"
	fi
fi

echo
echo If you have Tcl/Tk version 8.0 or greater installed, we can try to
echo build the graphical frontend for UUDeview, xdeview.
echo
echo Usually, the configuration script finds out by itself where and if you
echo have these two packages installed. On the following prompts, you can
echo either accept the default [y], which means to add support if available,
echo \'no\' or \'none\' to disable support, or you can give a base directory
echo where the package can be found \(we will need DIR/include for the
echo include files and DIR/lib for the libraries\).
echo
/bin/echo -n "Add Tcl support if possible [y] "
read tcl
if test "x$tcl" = "x" || test "x$tcl" = "xy" || test "x$tcl" = "xyes" ; then
	tclopt=""
	tkopt="y"
else
if test "x$tcl" = "xn" || test "x$tcl" = "xno" || test "x$tcl" = "xnone" ; then
	tclopt="--disable-tcl"
	tkopt="--disable-tk"
else
	tclopt="--enable-tcl=$tcl"
	tkopt="$tcl"
fi
fi
if test "x$tkopt" != "x--disable-tk" ; then
	/bin/echo -n "Add Tk  support if possible [$tkopt] "
	read tk
	if test "x$tk" = "x" ; then
		tk=$tkopt
	fi
	if test "x$tk" = "xy" || test "x$tk" = "xyes" ; then
		tkopt=""
	else
	if test "x$tk" = "xn" || test "x$tk" = "xno" || test "x$tk" = "xnone" ; then
		tkopt="--disable-tk"
	else
		tkopt="$tk"
	fi
	fi
fi

#
# get domain name
#

domainname=`grep '^domain' /etc/resolv.conf | tr -d ' 	' | sed 's/domain//'`

echo
echo If you want to post directly from uuenview, we might need to know
echo the internet domain you are in. The \"domain\" is the fully qualified
echo name of your host minus the hostname. For example, if your host is
echo bogus.cs.cms.edu, then the domain is cs.cms.edu. If you do not con-
echo figure a domain name, a bogus default will be used. In that case,
echo users will not be able to reply to your postings.
echo
/bin/echo -n "Your domain [$domainname] "
read domain
if test "x$domain" != "x" ; then
	domopt="--enable-domain=$domain"
fi

echo
echo We will now complete the configuration. This might take a while.
echo

./configure --prefix=$prefix $manopt $tclopt $tkopt

echo
echo ----------------------------------------------------------------------
echo Configuration finished. Building UUDeview.
echo ----------------------------------------------------------------------
echo
echo The package is now configured and ready to build. Do you want to
/bin/echo -n "build UUDeview now [y] "
read build
if test "x$build" != "x" && test "x$build" != "xy" && test "x$build" != "xyes" ; then
	echo
	echo Very well. Installation aborted.
	echo
	echo You can compile the package later by changing to `pwd`
	echo and running first \"make\", then \"make install\".
	echo
	exit 0
fi

echo
echo building UUDeview. This might take a while.
echo

make

if test ! -x ./unix/uudeview || test ! -x ./unix/uuenview ; then
	echo
	echo Compilation failed. Please examine the error messages above
	echo and try to figure out what\'s wrong. Sorry, can\'t help you
	echo here.
	echo
	exit 1
fi

if test ! -w $prefix && test "$whoami" != "root" ; then
	echo
	echo You want to install UUDeview globally but aren\'t logged in as
	echo root. I cannot install the package now. Please log in as root,
	echo change to `pwd` and enter the command \"make install\" to
	echo complete the installation.
	echo
	exit 0
fi

echo
echo ----------------------------------------------------------------------
echo Building finished. Installing UUDeview.
echo ----------------------------------------------------------------------
echo
echo I am now ready to install UUdeview. Do you want to complete
/bin/echo -n "the Installation now [y] "
read install
if test "x$install" != "x" && test "x$install" != "xy" && test "x$install" != "xyes" ; then
	echo
	echo Very well. Installation aborted. You can later install the package
	echo by running \"make install\" from `pwd`.
	echo
	exit 0
fi

echo
echo Installing Package.
echo

make install
cd ..

echo
echo ----------------------------------------------------------------------
echo Installation finished. Removing source files.
echo ----------------------------------------------------------------------
echo
/bin/echo -n "Do you want the source to be removed [y] "
read rem
if test "x$rem" = "x" || test "x$rem" = "xy" || test "x$rem" = "xyes" ; then
	rm -rf uudeview.tgz uudeview.tar uudeview-"$VERSION"."$PATCH"
fi
echo
echo Congratulations, you should now have a complete installation of
echo UUDeview. Now make sure that the binary directory $prefix/bin
echo is in your \$PATH. Then I suggest reading the manual pages.
echo Have fun!
echo
exit 0

################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################
################# Padding characters to fill 12 Kilobytes #################

