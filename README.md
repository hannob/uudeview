UUDEVIEW
========

Welcome to UUDeview, a package of a very smart decoder and an encoder for Base64 (MIME),
uuencoded, xxencoded and Binhex files.  
Its ultimate goal is to fully replace the "standard", but dumb uudecode and uuencode
utilities.

Introduction
------------

If you're into emailing and reading news, you have already learned how difficult it is
to distribute or receive binary files by this means. You will have noticed that you must
frequently edit files before decoding them, and make sure they're in the right order.
Here's a tool to free you of these permanent bothers. The package features a smart
decoder, UUDeview, which allows for comfortable decoding of about anything you feed into
it. It reads in a bunch of files, sorts the individual postings or emails and lets you
choose which files to decode. You will also find the equally helpful UUEnview, which not
only encodes binaries for sending, but automatically sends them, too.

The decoder supports the four encoding methods uuencoding, xxencoding, Base64 (from the
MIME standard) and BinHex, the encoder can encode to all of them except to BinHex. When
encoding, you can choose the method with a command-line option, and the decoder
auto-detects all of them.

The frontend programs are built around a powerful decoding library, which can be easily
integrated into other applications as well. You can find LaTeX documentation in the doc/
subdirectory.

Credits
-------

UUDeview is currently maintained by [Hanno Böck](https://hboeck.de/). It has originally
been developed and maintained by [Frank Pilhofer](http://www.fpx.de/).

This piece of software wouldn't be what it is today without the help, encouragement,
suggestions and bugreports from a number of people.

Previous contributors include Michael Newcomb, Nico Mak, Stephane Barizien, Kris Karas,
Joachim Meyn, Josip Rodin, Peter Muir, Chris Hanson, Andrew Shadura, Adrian Bunk,
Andreas Metzler, Roland Rosenfeld, Martin Schulze, Nico Golde, tastytea, Pascal Jäger,
Urs Janßen, Brady Fomegne, J. Wilkinson, Kevin B. York, Steven Grady, Matthew Mueller,
Yuri D'Elia.

More Info
---------

UUDeview's old web page can be found here: http://www.fpx.de/fp/Software/UUDeview/

UUDeview is currently maintained on GitHub, and can be found here:
https://github.com/hannob/uudeview/
