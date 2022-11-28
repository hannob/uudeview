
/*
 * needed for auto configuration
 * $Id: acconfig.h,v 1.1.1.1 1996/06/06 19:41:05 fp Exp $
 */

/*
 * If your system is kinda special
 */
#undef SYSTEM_DOS
#undef SYSTEM_QUICKWIN
#undef SYSTEM_WINDLL
#undef SYSTEM_OS2

/*
 * If your system has stdin/stdout/stderr
 */
#undef HAVE_STDIO

/*
 * how to declare functions that are exported from the UU library
 */
#undef UUEXPORT

/*
 * how to declare functions that are exported from the UUTCL library
 */
#undef UUTCLEXPORT

/*
 * how to declare functions that are exported from the fptools library
 */
#undef TOOLEXPORT

/*
 * how to declare functions that are interfaced with TCL
 */
#undef UUTCLFUNC

/*
 * define if your compiler supports function prototypes
 */
#undef PROTOTYPES

/*
 * define if you have TCL version 7.5 or later
 */
#undef HAVE_TCL

/* 
 * define if you HAVE_TCL and TK version 4.1 or later 
 */
#undef HAVE_TK

/*
 * define if your Tk can handle the Tk_CreatePhotoImageFormat hook 
 */
#undef TK_PHOTO

/*
 * define if Tcl_Main or Tk_Main needs Tcl_AppInit as third parameter 
 */
#undef TMAIN_THREE

/*
 * Replacement functions.
 * #define strerror _FP_strerror
 * #define tempnam  _FP_tempnam
 * if you don't have these functions
 */
#undef strerror
#undef tempnam

/*
 * your system's directory separator (usually "/")
 */
#undef DIRSEPARATOR

/* 
 * your mailing program. full path and the necessary parameters.
 * the recepient address is added to the command line (with a leading
 * space) without any further options
 */
#undef PROG_MAILER

/* 
 * define if the mailer needs to have the subject set on the command
 * line with -s "Subject". Preferredly, we send the subject as a header.
 */
#undef MAILER_NEEDS_SUBJECT

/* 
 * define if posting is enabled. Do not edit.
 */
#undef HAVE_NEWS

/*
 * your local news posting program. full path and parameters, so that
 * the article and all its headers are read from stdin
 */
#undef PROG_INEWS

/*
 * the name of your local domain. only needed when using minews
 */
#undef DOMAINNAME

/* 
 * your local NNTP news server. only needed when using minews
 * can be overridden by $NNTPSERVER at runtime
 */
#undef NNTPSERVER

/*
 * defined when we use minews, so that we know that we must define
 * the NNTPSERVER environment variable to be able to post
 */
#undef NEED_NNTPSERVER

/*
 * whether you want to hide your hostname behind the domain. usually
 * undefined
 */
#undef HIDDENNET

