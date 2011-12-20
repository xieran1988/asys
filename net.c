#include <algo.h>

//static char testbuf[20] = {"123456"};
static struct sockaddr_in clisa;
static int sockfd;//, sockfd2;
static char bigbuf[1*1024*1024];//, mac[32];
char recvbuf[1024];
static double rxbytes, txbytes;
FILE *sendfp, *sendfp2, *recvfp;

int sockbind(int port)
{
	struct sockaddr_in si;
	int fd, bufsiz = 1*1024*1024;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	si.sin_family = AF_INET;
	si.sin_addr.s_addr = htonl(INADDR_ANY);
	si.sin_port = htons(port);
	if (bind(fd, (struct sockaddr *)&si, sizeof(si)) == -1) {
		log("bind to %d failed: %s\n", port, strerror(errno));
		panic();
	}
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char *)&bufsiz, sizeof(bufsiz));
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (const char *)&bufsiz, sizeof(bufsiz))) {
		log("setsockopt SO_SNDBUF failed\n");
		panic();
	}

	return fd;
}


void net_init()
{
	sockfd = sockbind(ARM_PORT1);
	sendfp = tmpfile();
	sendfp2 = tmpfile();
	recvfp = tmpfile();
}


