#include <algo.h>

double now()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec + tv.tv_usec/1e6)*1000;
}

int selected_read(int fd)
{
	fd_set fs;
	int r;
	struct timeval tv = {};

	FD_ZERO(&fs);
	FD_SET(fd, &fs);

	r = select(fd + 1, &fs, NULL, NULL, &tv);
	if (r < 0) {
		log("select failed: %s\n", strerror(errno));
		abort();
	}

	return r > 0;
}

void img_bitblt(img_t *dst, int dx, int dy, img_t *src, int sx, int sy, int w, int h)
{
	while (h--) {
		memcpy(dst->data[dy] + dx, src->data[sy] + sx, w);
		dy++; sy++;
	}
}

void img_fill_rect(img_t *p, int x, int y, int w, int h, int val)
{
	if (x < 0) {
		w += x;
		x = 0;
	}
	if (x + w >= IMG_W)
		w = IMG_W - x;
	while (h--) {
		if (y >= 0 && y < IMG_H)
			memset(p->data[y] + x, val, w);
		y++;
	}
}

void img_plot_rect(img_t *p, int x, int y, int w, int h, int val)
{
	int rw = 2;
	img_fill_rect(p, x - rw, y - rw, w + rw*2, rw*2, val);
	img_fill_rect(p, x - rw, y - rw, rw*2, h + rw*2, val);
	img_fill_rect(p, x - rw, y + h - rw, w + rw*2, rw*2, val);
	img_fill_rect(p, x + w - rw, y - rw, rw*2, h + rw*2, val);
}

void img_draw_cross(img_t *p, int x, int y, int val)
{
	int w = 2, r = 6;
	img_fill_rect(p, x - r, y - w, r*2, w*2, val);
	img_fill_rect(p, x - w, y - r, w*2, r*2, val);
}




