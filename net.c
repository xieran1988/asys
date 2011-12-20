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

static void net_recv(struct sockaddr_in sa)
{
	if (!strcmp(recvbuf, "use")) {
		log("%s want use me\n", inet_ntoa(sa.sin_addr));
		clisa = sa;
	}
}

static void test_and_recv(FILE *fp, int fd)
{
	int len, r;
	struct sockaddr_in sa;

	if (selected_read(fd)) {
		len = sizeof(sa);
		r = recvfrom(fd, bigbuf, sizeof(bigbuf), 0, (struct sockaddr *)&sa, (socklen_t *)&len);
		if (r != -1) {
			fwrite(bigbuf, 1, r, fp);
			rewind(fp);
			while (fgets(recvbuf, sizeof(recvbuf), fp)) {
				log("got line: %s\n", recvbuf);
				net_recv(sa);
				ui_recv();
			}
			rxbytes += r;
		} else {
			printf("test_and_recv recvfrom failed\n");
		}
	}

	rewind(fp);
	if (ftruncate(fileno(fp), 0))
		panic();
}

static void test_and_send(FILE *fp, int fd, int port)
{
	int len, r;
	struct sockaddr_in sa = clisa;

	if (clisa.sin_family && ftell(fp)) {
		rewind(fp);
		len = fread(bigbuf, 1, sizeof(bigbuf), fp);
		if (len != -1) {
			sa.sin_port = htons(port);
			r = sendto(fd, bigbuf, len, 0, (struct sockaddr *)&sa, sizeof(sa));
			if (r == -1) {
				printf("test_and_send sendto %d failed\n", port);
			} else
				txbytes += r;
		}
	}

	rewind(fp);
	if (ftruncate(fileno(fp), 0))
		panic();
}

void sendimg(img_t *img, int seq, int start, int cnt)
{
	img_part_t h;
	int i;

	h.seq = seq;
	//printf("send img seq %d start %d cnt %d\n", seq, start, cnt);
//	printf("no img\n");
//	return ;
	for (i = start; i < start + cnt; i++) {
//		log("send img part %d img %p\n", i, img);
		h.idx = i;
		fwrite(&h, 1, sizeof(h), sendfp2);
		fwrite(img->data[i*IMG_PART_H], 1, IMG_PART_H*IMG_W, sendfp2);
		test_and_send(sendfp2, sockfd, PC_PORT2);
	}
//	log("send img done\n");
}

void net_send_and_recv()
{
	static double t;

	mon("send %.2lfK/s recv %.2lfK/s\n", txbytes/(now()-t), rxbytes/(now()-t));
	//printf("clisa %s. testbuf %p clisa %p buf=%s\n", inet_ntoa(clisa.sin_addr), testbuf, &clisa, testbuf);
	t = now();
	txbytes = 0;
	rxbytes = 0;

	test_and_send(sendfp, sockfd, PC_PORT1);
	test_and_recv(recvfp, sockfd);
}

void net_init()
{
	sockfd = sockbind(ARM_PORT1);
	sendfp = tmpfile();
	sendfp2 = tmpfile();
	recvfp = tmpfile();
}


