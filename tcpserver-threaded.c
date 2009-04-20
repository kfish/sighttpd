/* gcc tcpserver-threaded.c -o tcpserver-threaded -lpthread */

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <pthread.h>
#include <netdb.h>

void panic(char *msg);

#define panic(m)	{perror(m); abort();}

void *servlet(void *arg)
{
	FILE *fp = (FILE *) arg;
	char s[100];

	/* proc client's requests */
	while (fgets(s, sizeof(s), fp) != 0 && strcmp(s, "bye\n") != 0) {
		printf("msg: %s", s);	/* display message */
		fputs(s, fp);	/* echo it back */
	}

	fclose(fp);		/* close the client's channel */

	return 0;		/* terminate the thread */
}

int main(int count, char *args[])
{
	struct sockaddr_in addr;
	int sd, port;

	if (count != 2) {
		printf("usage: %s <protocol or portnum>\n", args[0]);
		exit(0);
	}

	/* Get server's IP and standard service connection* */
	if (!isdigit(args[1][0])) {
		struct servent *srv = getservbyname(args[1], "tcp");

		if (srv == NULL)
			panic(args[1]);

		printf("%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
		port = srv->s_port;
	} else {
		port = htons(atoi(args[1]));
	}

	/* Create socket * */
	sd = socket(PF_INET, SOCK_STREAM, 0);
	if (sd < 0)
		panic("socket");

	/* Bind port/address to socket * */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = INADDR_ANY;	/* any interface */

	if (bind(sd, (struct sockaddr *) &addr, sizeof(addr)) != 0)
		panic("bind");

	/* Make into listener with 10 slots * */
	if (listen(sd, 10) != 0) {
		panic("listen")
	} else {
		/* Begin waiting for connections * */
		int ad;
		socklen_t size;
		struct sockaddr gotcha;
		pthread_t child;
		FILE *fp;

		/* process all incoming clients */
		while (1) {
			size = sizeof(struct sockaddr_in);
			ad = accept(sd, &gotcha, &size);

			if (ad == -1) {
				perror("accept");
			} else {
				fp = fdopen(ad, "r+");	/* convert into FILE* */
				pthread_create(&child, 0, servlet, fp);	/* start thread */
				pthread_detach(child);	/* don't track it */
			}
		}
	}
}
