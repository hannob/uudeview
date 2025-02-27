/*
 * Definitions for NNTP client routines.
 *
 * @(#)clientlib.h	1.1	(Berkeley) 1/9/88
 */

extern char *getserverbyfile(char *file);
extern int server_init(char *machine);
extern void put_server(char *string);
extern void close_server();

extern int get_tcp_socket(char *machine);
extern int get_server(char *string, int size);
extern int handle_server_response(int response, char *server);
