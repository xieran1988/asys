#include <algo.h>

int bPause, DirectT;
char buf[1*1024*1024];
int bRun=1;
void process() {}

static void doexit(int i)
{
	exit(0);
}

int main()
{
	struct termios tio;
	int sockfd, ttyfd;
	struct sockaddr_in si;
	int bufsiz = 1*1024*1024;
	struct sockaddr_in sa;

	signal(2, doexit);
	signal(6, doexit);
	signal(9, doexit);
	signal(15, doexit);
	signal(13, doexit);
	setbuf(stdout, NULL);

	log ("try open socket\n");

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	si.sin_family = AF_INET;
	si.sin_addr.s_addr = htonl(INADDR_ANY);
	si.sin_port = htons(2010);
	if (bind(sockfd, (struct sockaddr *)&si, sizeof(si)) == -1) {
		log("bind to %d failed: %s\n", 2010, strerror(errno));
		panic();
	}
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char *)&bufsiz, sizeof(bufsiz));
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char *)&bufsiz, sizeof(bufsiz))) {
		log("setsockopt SO_SNDBUF failed\n");
		panic();
	}

	memset(&tio,0,sizeof(tio));
	tio.c_iflag=0;
	tio.c_oflag=0;
	tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
	tio.c_lflag=0;
	tio.c_cc[VMIN]=1;
	tio.c_cc[VTIME]=5;

	log ("try open ttyS0\n");

	ttyfd = open("/dev/ttyS0", O_RDWR | O_NONBLOCK);
	if (ttyfd == -1) {
		log("open serial failed: %s\n", strerror(errno));
		panic();
	}
	cfsetospeed(&tio,B9600);            // 115200 baud
	cfsetispeed(&tio,B9600);            // 115200 baud

	tcsetattr(ttyfd, TCSANOW, &tio);

	while (1) {
		int salen;
		int r;
		int maxfd = sockfd > ttyfd ? sockfd : ttyfd;
		fd_set fdsr;

		FD_ZERO(&fdsr);
		FD_SET(sockfd, &fdsr);
		FD_SET(ttyfd, &fdsr);

		if (select(maxfd+1,&fdsr,NULL,NULL,NULL) > 0) {
			if (FD_ISSET(sockfd, &fdsr)) {
				r = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&sa, (socklen_t *)&salen);
				if(r!=5)
				{

					printf("---- ");
					int k;			
					for(k=0;k<r;k++)
					{
						printf("%02X ", buf[k]);
					}
					printf("\n");
				}

				printf("got %d bytes from socket: %d\n", r, sa.sin_port);
				if (r != -1) 
					write(ttyfd, buf, r);
			}
			if (FD_ISSET(ttyfd, &fdsr)) {
				usleep(15000);
				r = read(ttyfd, buf, 1024*1024);
				printf("got %d bytes from tty\n", r);
				/*			int k;			
								for(k=0;k<r;k++)
								{
								printf("%02X ", buf[k]);
								}
								printf("\n");
								*/
				if (r != -1)
					sendto(sockfd, buf, r, 0, (struct sockaddr *)&sa, sizeof(sa));
			}
		}
		//usleep(1000);
	}

	return 0;
}



