/* 
 *  A threaded server based on the work of
 *  Martin Broadhurst (www.martinbroadhurst.com)
 *  Compile with -pthread
 *
 * From Martin Broadhurst's web site:
 *
 * "This is a collection of example network server implementations for
 *	Linux and Windows. You can use these as a starting point for your own
 *	server implementation."
 *
 * This is the main entry point for the hub-server.  It accomplishes the following:
 * -- Initialize the listening socket.
 * -- Connect to all external servers specified on the command line,
 *	  calling hs_sock_new_connect(int socket) for each connection.
 * -- Call the hs_sock_init() function in file hub-server-sock.c.
 * -- Loop forever accepting new connections and calling 
 *    hs_sock_new_connect(int socket) in file hub-server-sock.c. 
 *
 * Note that this server hasn't been "serverized" so it's not a proper daemon.
 * For instance, it logs informational & error messages on (unbuffered) stdout.
 */
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

#include "hub-server-util.h"
#include "hub-server-sock.h"

#define REVISION "rev_10 PRE-RELEASE" 	/* Update this on each revision */

FILE* fpS;

void usage(void)
{
	fprintf(stderr, "# hub-server %s\n", REVISION);
#if HUB_SERVER_FILTER    
    fprintf(stderr, "# Usage: $ hub-server [--csmode] [--nodelay] [--eol=<n>] [--dle=<n>] [--file=<path/name>] <listen_ipad> <listen_port> \n");   
#else    
	fprintf(stderr, "# Usage: $ hub-server [--csmode] [--nodelay] [--eol=<n>] [--dle=<n>] <listen_ipad> <listen_port> {<connect_host> <connect_port>}*\n");   
#endif    
	hs_exit(1);
}

char *host_name = NULL;				/* Host (ipad) to bind listening socket */
char *svcs_name = NULL;	 			/* Service (port) to bind listening socket */
char eol = 012;						/* End-of-line (initially: ASCII LF) */
char dle = 0;						/* Data-link-escape (initially: don't care) */
int dle_flag = 0;					/* DLE set flag (initially: false) */
int cs_mode = 0;					/* Client/server mode flag */
int nodelay_flag = 0;				/* Get rid of Nagle delay */

int main(int argc, char *argv[])
{
	char *me = "main";
    int sock;
    struct addrinfo hints, *res;
    int reuseaddr = 1;
    void do_cmd_line_processing(int argc, char *argv[]);

    setbuf(stdout, NULL);				/* Make stdout unbuffered */
    openlog("hub-server", LOG_PID, LOG_DAEMON);

	syslog(LOG_INFO, "#### Starting hub-server %s\n", REVISION);

    hs_sock_init();						/* Init connect input service thread */

printf("BEGIN: argc %i argv %s\n",argc, *argv);

	do_cmd_line_processing(argc, argv);

    /* Get the address info */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host_name, svcs_name, &hints, &res) != 0)
        hs_fatal(me, "getaddrinfo");

    /* Create the socket */
    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1)
        hs_fatal(me, "socket");

    /* Enable the socket to reuse the address */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1)
        hs_fatal(me, "setsockopt");

    /* Bind to the address */
    if (bind(sock, res->ai_addr, res->ai_addrlen) == -1)
        hs_fatal(me, "bind");

    freeaddrinfo(res);

    /* Set listen mode */
    if (listen(sock, 5) == -1)
        hs_fatal(me, "listen");

    /* Main loop.  We monitor the listen socket for connections and 
     * create new connects as connections come in.
     */
    syslog(LOG_INFO, "Loop forever and wait on accept(%d, ...);\n", sock);

    while (1) 
    {
        size_t size = sizeof(struct sockaddr_in);
        struct sockaddr_in their_addr;
        int newsock;
        
        newsock = accept(sock, (struct sockaddr*)&their_addr, (socklen_t *)&size);
        if (newsock < 0)
            perror("main -- accept returns < 0");
        else
        	hs_sock_new_connect(newsock, 0, 0);
    }

    close(sock);
    return 0;
}

void connect_to_server(char *hostname, char *serviceport, int connectnum)
{
	char *me = "connect_to_server";
    struct sockaddr_in serveraddr;
    struct hostent *server;
    int sockfd;
    
    /* Connecting to external server */
    syslog(LOG_INFO, "Connecting to external server \"%s:%s\"\n", hostname, serviceport);
    
    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        hs_fatal(me, "ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        hs_fatal(me, "ERROR, no such host\n");
        hs_exit(2);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(atoi(serviceport));
    /* connect: create a connection with the server */
    if (connect(sockfd,
    		(const struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) 
      hs_fatal(me, "ERROR connecting");
      
// Non-zero clientflag conveys client connection
    hs_sock_new_connect(sockfd, 1, connectnum);
}

void do_cmd_line_processing(int argc, char *argv[])
{
    int clientctr = 0;

	#define	ARG_IS(string)		(strncmp(argv[0], string, strlen(string)) == 0)
	
	if(argc < 3) usage();
int ct=0;
printf("A %i: argc %i argv %s\n",ct++, argc, *argv);

	for(argc -= 1, argv += 1; argc > 0 && *argv[0] == '-'; argc -= 1, argv += 1)
	{
printf("B %i: argc %i argv %s\n",ct++, argc, *argv);

		if(ARG_IS("-?")) usage();
		if(ARG_IS("--help")) usage();
		
		if(ARG_IS("--csmode")) { cs_mode = 1; continue; }
		if(ARG_IS("--nodelay")) { nodelay_flag = 1; continue; }
		if(ARG_IS("--eol=")) { eol = (char)strtol(argv[0]+6, NULL, 0); continue; }		
		if(ARG_IS("--dle=")) { dle = (char)strtol(argv[0]+6, NULL, 0); dle_flag = 1; continue; }
        if(ARG_IS("--file=")) 
        {
            if ((fpS=fopen((argv[0]+7),"r")) == NULL)
            {
                printf("# --file%s failed to open\n",(argv[0]+6));
                usage();
            }
printf("C 3: --file %s opened OK!\n",(argv[0]+6));
            continue;
        }        
	}
printf("D %i: argc %i argv %s\n",ct++, argc, *argv);

	if(argc <= 0  ||  (argc & 1) != 0)
	{
printf("E %i: argc %i argv %s\n",ct++, argc, *argv);
    	printf("# Error: <ipad>/<port> come in pairs!\n");
		hs_exit(3);
	}

	/* Setup listening socket pair */
	host_name = argv[0];
	svcs_name = argv[1];

    for(argc -= 2, argv += 2 ; argc >= 2; argc -= 2, argv += 2)
    { // Client connection counter will be 1 for first client
printf("connecting to server: %s %s command line client pair: %i \n",argv[0], argv[1],clientctr);        
        clientctr += 1;
        connect_to_server(argv[0], argv[1], clientctr);
    }
    hsd_sandbox_init(fpS);

	/* Log setup stuff */
	syslog(LOG_INFO, "Listening socket is \"%s:%s\"\n", host_name, svcs_name);
	syslog(LOG_INFO, "End-of-line is 0x%X\n", (unsigned int)eol & 0xff);
	if(dle_flag)
		syslog(LOG_INFO, "Data-link-escape is 0x%X\n", (unsigned int)dle & 0xff);
	if(cs_mode)
		syslog(LOG_INFO, "Mode is client/server mode\n");
	if(nodelay_flag)
		syslog(LOG_INFO, "TCP_NODELAY set\n");
}
