
/*
 * needed for auto configuration
 * $Id: acconfig.h,v 1.1.1.1 1996/06/06 19:41:15 fp Exp $
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
 * how to declare functions that are exported from the fptools library
 */
#undef TOOLEXPORT

/*
 * define if your compiler supports function prototypes
 */
#undef PROTOTYPES

/*
 * Replacement functions.
 * #define strerror _FP_strerror
 * #define tempnam  _FP_tempnam
 * if you don't have these functions
 */
#undef strerror
#undef tempnam

/* 
 * your mailing program. full path and the necessary parameters.
 * the recipient address is added to the command line (with a leading
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

