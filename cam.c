#include <algo.h>

img_t Y;

typedef struct {
	int index;
	unsigned int length;
	char *start;
} capbuf_t;
#define CAP_BUFCNT 3
#define DEF_PIX_FMT		V4L2_PIX_FMT_UYVY
static capbuf_t capbuf[CAP_BUFCNT];
static int capfd;

void cam_exit()
{
	int i;
	log("closing\n");
	for (i = 0; i < CAP_BUFCNT; i++) {
		if ((void *)capbuf[i].start > (void *)0) {
			munmap(capbuf[i].start,
			       capbuf[i].length);
		}
	}
	if (capfd)
		close(capfd);
}

static int shouldshrink;

void cam_init()
{
	int i, rt = 1;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer buf;
	struct v4l2_capability capability;
	struct v4l2_input input;
	struct v4l2_format fmt;

retry:
	capfd = open("/dev/video0", O_RDWR);
	if (capfd <= 0) {
		log("open dev failed\n");
		panic();
	}

	while (1) {
		i = 0;
		if (ioctl(capfd, VIDIOC_G_INPUT, &i) < 0) {
			log("error VIDIOC_G_INPUT, retry\n");
			sleep(1);
			continue;
		}
		log("G_INPUT idx: %d\n", i);
		break;
	}

	memset(&input, 0, sizeof(input));
	input.index = i;
	if (ioctl(capfd, VIDIOC_ENUMINPUT, &input) < 0) {
		log("error VIDIOC_ENUMINPUT");
		panic();
	}
	log("input: %s\n", input.name);

	if (ioctl(capfd, VIDIOC_QUERYCAP, &capability) < 0) {
		log("error VIDIOC_QUERYCAP");
		panic();
	}
	if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
		log("Not capable of streaming\n");
		panic();
	}

	for (i = 0; ; i++) {
		struct v4l2_fmtdesc fmtdesc = {};
		fmtdesc.index = i;
		fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (ioctl(capfd, VIDIOC_ENUM_FMT, &fmtdesc))
			break;
		log("fmt: %.32s\n", fmtdesc.description);
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(capfd, VIDIOC_G_FMT, &fmt)) {
		log("error VIDIOC_G_FMT\n");
		panic();
	}

	fmt.fmt.pix.pixelformat = DEF_PIX_FMT;
	if (ioctl(capfd, VIDIOC_S_FMT, &fmt)) {
		log("error VIDIOC_S_FMT\n");
		panic();
	}

	if (ioctl(capfd, VIDIOC_G_FMT, &fmt)) {
		log("error VIDIOC_G_FMT\n");
		panic();
	}

	log("getfmt.size %dx%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
	log("want.size %dx%d\n", IMG_W, IMG_H);

	if (!
			((fmt.fmt.pix.width == IMG_W*2 && fmt.fmt.pix.height == IMG_H*2) || 
			(fmt.fmt.pix.width == IMG_W && fmt.fmt.pix.height == IMG_H) )
		 )	
	{
		log("err getfmt.size %d,%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);
		log("retry %d ...\n", rt);
		rt++;
		close(capfd);
		goto retry;
	}

	if (fmt.fmt.pix.width == IMG_W*2 && fmt.fmt.pix.height == IMG_H*2) {
		shouldshrink = 1;
	}

	if (fmt.fmt.pix.pixelformat != DEF_PIX_FMT) {
		log("Requested pixel format not supported\n");
		panic();
	}

	memset(&reqbuf, 0, sizeof(reqbuf));
	reqbuf.count = CAP_BUFCNT;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(capfd, VIDIOC_REQBUFS, &reqbuf) < 0) {
		log("error Cannot allocate memory");
		panic();
	}
	log("reqbuf.count: %d\n", reqbuf.count);

	memset(&buf, 0, sizeof(buf));
	for (i = 0; i < reqbuf.count; i++) {
		buf.type = reqbuf.type;
		buf.index = i;
		buf.memory = reqbuf.memory;
		if (ioctl(capfd, VIDIOC_QUERYBUF, &buf)) {
			log("error VIDIOC_QUERYCAP");
			panic();
		}

		capbuf[i].length = buf.length;
		capbuf[i].index = i;
		capbuf[i].start = mmap(NULL, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, capfd, buf.m.offset);
		if (capbuf[i].start == MAP_FAILED) {
			log("error Cannot mmap = %d buffer\n", i);
			panic();
		}
		memset((void *) capbuf[i].start, 0x80,
			capbuf[i].length);
		if (ioctl(capfd, VIDIOC_QBUF, &buf)) {
			log("error VIDIOC_QBUF");
			panic();
		}
	}
	log("Init done successfully\n");
}

static void wait_(float t)
{
	static float t_last;

	t = 1000./16;
	t -= now() - t_last;

	printf("%s %.2f\n", t>0?"":"!!!!", t);
	if (t > 0) {
		if (usleep(t * 1000))
			printf("usleep failed\n");
	}
	t_last = now();
}

static void process_img(void *data, void *_)
{
	int x, y;
	u8 *b, *p = (u8 *)data;

	if (shouldshrink) {
		for (y = 0; y < IMG_H*2; y += 2) {
			for (x = 0; x < IMG_W*2*2; x += 4) {
				b = p + y*IMG_W*2*2 + x;
				Y.data[y/2][x/4] = b[1];
			}
		}
	} else {
		for (y = 0; y < IMG_H; y += 1) {
			for (x = 0; x < IMG_W*2; x += 2) {
				b = p + y*IMG_W*2 + x;
				Y.data[y][x/2] = b[1];
			}
		}
	}
	cam_callback();
}

static void cam_streamon()
{
	int a;

	a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(capfd, VIDIOC_STREAMON, &a)) {
		log("error VIDIOC_STREAMON\n");
		panic();
	}
	log("Stream on...\n");
}

void cam_poll_wait(void (*func)(void *, void *), void *p, float t)
{
	struct v4l2_buffer capture_buf;
	
	capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	capture_buf.index = 0;
	capture_buf.memory = V4L2_MEMORY_MMAP;

	if (ioctl(capfd, VIDIOC_DQBUF, &capture_buf)) {
		log("error VIDIOC_DQBUF\n");
		panic();
	}
	func(capbuf[capture_buf.index].start, p);
	wait_(t);
	if (ioctl(capfd, VIDIOC_QBUF, &capture_buf)) {
		log("error VIDIOC_QBUF\n");
		panic();
	}
}

#ifndef MYSRC
void cam_loop(int argc, char *argv[])
{
	cam_streamon();
	while (1) 
		cam_poll_wait(process_img, NULL, T_FRM);
}
#endif

void cam_start()
{
	utils_init();
	cam_init();
	cam_streamon();
}

