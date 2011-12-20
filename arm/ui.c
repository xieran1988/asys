#include <algo.h>

enum {
	IMG, IMGSEL, BTN, TEXT, VARDOUBLE, VARINT,
};
typedef struct {
	int type;
	const char *name;
	void (*btncb)(const char *);
	imgsel_t *imgsel;
	img_t *img;
	void *switchimg;
	double dmax, dmin, *dp;
	int imax, imin, *ip;
} ui_t;
static ui_t ui[256];
static int uicnt, uiseq;

static void *curimg;
static int imgsegsize = IMG_H / IMG_PART_H / 4;
static int imgseq, imgstart;

static void ui_changed()
{
	uiseq = rand(); 
}

static void img_reset() 
{
	imgseq = rand();
	imgstart = 0;
}

void ui_send()
{
	int i;

#define range(min, max, val) (int)((val - min) * 10000 / (max - min))
	sendstr("seq %d\n", uiseq);
	for (i = 0; i < uicnt; i++) {
		ui_t *p = &ui[i];
		switch (p->type) {
		case IMG:
			if (curimg == (void *)p) {
				int cnt = imgstart + imgsegsize > IMG_H/IMG_PART_H ? 
					IMG_H/IMG_PART_H - imgstart :
					imgsegsize;
				sendimg(p->img, imgseq, imgstart, cnt);
				imgstart += cnt;
				if (imgstart == IMG_H/IMG_PART_H) {
					img_reset();
				}
			}
			break;
		case BTN: 
			sendstr("btn %s\n", p->name); 
			break;
		case VARDOUBLE: 
			sendstr("slider %s %d\n", p->name, range(p->dmin, p->dmax, *p->dp)); 
			break;
		case VARINT:
			sendstr("slider %s %d\n", p->name, range(p->imin, p->imax, *p->ip)); 
			break;
		}
	}
#undef range
}

void ui_mon()
{
	int i;
	for (i = 0; i < uicnt; i++) {
		if (ui[i].type == VARINT) 
			mon("%s: %d\n", ui[i].name, *ui[i].ip);
		if (ui[i].type == VARDOUBLE) 
			mon("%s: %lf\n", ui[i].name, *ui[i].dp);
	}
}

void ui_recv()
{
	int i, x, y, vi;
	char s[512], n[512];

	strncpy(s, recvbuf, sizeof(s)-1);
	for (i = 0; i < uicnt; i++) {
		ui_t *u = &ui[i];
		switch (u->type) {
		case IMGSEL:
			if (sscanf(s, "%s%d%d", n, &x, &y) == 3 && !strcmp(n, "imgclick")) {
				log("img clicked %d %d\n", x, y);
				imgsel_t *sel = u->imgsel;
				if (sel->stat == IMGSEL_SEL1) {
					sel->x1 = x;
					sel->y1 = y;
					sel->stat = IMGSEL_SEL2;
				} else if (sel->stat == IMGSEL_SEL2) {
					sel->x2 = max(sel->x1, x);
					sel->x1 = min(sel->x1, x);
					sel->y2 = max(sel->y1, y);
					sel->y1 = min(sel->y1, y);
					sel->stat = IMGSEL_OK;
				}
				ui_changed();
			}
			break;
		case BTN: 
			if (sscanf(s, "%s", n) == 1 && !strcmp(n, u->name)) {
				log("btn %s clicked\n", n);
				if (u->btncb)
					u->btncb(u->name); 
				if (u->imgsel)
					u->imgsel->stat = IMGSEL_SEL1;
				if (u->switchimg) {
					curimg = u->switchimg;
					img_reset();
				}
				ui_changed();
			}
			break;
		case VARINT: 
			if (sscanf(s, "%s%d", n, &vi) == 2 && !strcmp(n, u->name)) {
				log("slider %s %d\n", n, vi);
				*u->ip = u->imin + vi * (u->imax - u->imin) / 10000;
				ui_changed();
			}
			break;
		case VARDOUBLE:
			if (sscanf(s, "%s%d", n, &vi) == 2 && !strcmp(n, u->name)) {
				log("slider %s %d\n", n, vi);
				*u->dp = u->dmin + vi * (u->dmax - u->dmin) / 10000;
				ui_changed();
			}
			break;
		}
	}
}

static ui_t *_decl_btn(const char *name, void (*cb)(const char *)) 
{
	ui_t *u = &ui[uicnt++];
	u->name = name; u->type = BTN; u->btncb = cb;
	return u;
}

void decl_img(const char *name, img_t *img)
{
	ui_t *u = &ui[uicnt++], *u2;
	u->name = name; u->type = IMG; u->img = img;
	char *s = (char *)malloc(128);
	sprintf(s, "switch_to_%s", name);
	u2 = _decl_btn((const char *)s, NULL);
	u2->switchimg = u;
	if (!curimg)
		curimg = u;
}

void decl_imgsel(const char *name, imgsel_t *sel)
{
	ui_t *u = &ui[uicnt++];
	u->name = name; u->type = IMGSEL; u->imgsel = sel;
	char *s = (char *)malloc(128);
	sprintf(s, "click_to_select_%s", name);
	u = _decl_btn((const char *)s, NULL);
	u->imgsel = sel;
}

void decl_btn(const char *name, void (*cb)(const char *))
{
	_decl_btn(name, cb);
}

void decl_var_int(const char *name, int *p, int min, int max)
{
	ui_t *u = &ui[uicnt++];
	u->name = name; u->type = VARINT; u->imin = min; u->imax = max; u->ip = p;
}

void decl_var_double(const char *name, double *p, double min, double max)
{
	ui_t *u = &ui[uicnt++];
	u->name = name; u->type = VARDOUBLE; u->dmin = min; u->dmax = max; u->dp = p;
}

void ui_save_cfg(char *file)
{
	FILE *fp;
	int i;

	fp = fopen2(file, "w+");

	log("to %s:\n", file);
	for (i = 0; i < uicnt; i++) {
		ui_t *u = &ui[i];
		imgsel_t *s = u->imgsel;
		switch (u->type) {
		case VARINT: 
			fprintf(fp, "%s %d\n", u->name, *u->ip); 
			log("int %s %d\n", u->name, *u->ip);
			break;
		case VARDOUBLE: 
			fprintf(fp, "%s %lf\n", u->name, *u->dp); 
			log("double %s %lf\n", u->name, *u->dp);
			break;
		case IMGSEL:
			fprintf(fp, "%s %d %d %d %d %d\n", 
					u->name, s->stat, s->x1, s->y1, s->x2, s->y2
					);
			log("imgsel %s %d (%d,%d) (%d,%d)\n", 
				u->name, s->stat, s->x1, s->y1, s->x2, s->y2
				);
			break;
		}
	}

	log("done\n");
	fclose(fp);
}

void ui_load_cfg(char *file)
{
	FILE *fp;
	int i, vi;
	double vd;
	char s[512], n[512];
	imgsel_t is;

	fp = fopen(file, "r");
	if (!fp) 
		return ;

	log("from %s:\n", file);
	while (fgets(s, sizeof(s), fp)) {
		for (i = 0; i < uicnt; i++) {
			ui_t *u = &ui[i];
			switch (u->type) {
			case VARINT: 
				if (sscanf(s, "%s%d", n, &vi) == 2 && !strcmp(n, u->name)) {
					*u->ip = vi;
					log("int %s %d\n", n, vi);
				}
				break;
			case VARDOUBLE:
				if (sscanf(s, "%s%lf", n, &vd) == 2 && !strcmp(n, u->name)) {
					*u->dp = vd;
					log("double %s %lf\n", n, vd);
				}
				break;
			case IMGSEL:
				if (sscanf(s, "%s%d%d%d%d%d", n, 
							&is.stat, &is.x1, &is.y1, &is.x2, &is.y2) == 6 && 
					!strcmp(n, u->name)
					) 
				{ 
					*u->imgsel = is;
					log("imgsel %s %d (%d,%d) (%d,%d)\n", n, is.stat, is.x1, is.y1, is.x2, is.y2);
				}
				break;
			}
		}
	}

	fclose(fp);
	log("done\n");
}

static char *cfg = AROOT "/cfg";
static char *defcfg = AROOT "/defcfg";

static void save_cfg(const char *_) 
{
   	ui_save_cfg(cfg); 
}

static void reset_cfg(const char *_) 
{
   	ui_load_cfg(defcfg); 
	ui_save_cfg(cfg);
}

static void track_run(const char *_) 
{
   	bRun=1;
}

static void track_stop(const char *_) 
{
   	bRun=0;
}

static void track_continue(const char *_) 
{
   	bPause=0;
}

static void track_pause(const char *_) 
{
	if(DirectT==0)
	{
		DirectT=1;
		mcu_send("192.168.1.3","PDI PDMINPUT=12576;#",21);
	}
   	bPause=1;
}

void ui_init()
{
	ui_changed();
	decl_btn("save_cfg", save_cfg);
	decl_btn("reset_cfg", reset_cfg);
	decl_btn("RUN", track_run);
	decl_btn("STOP", track_stop);
	decl_btn("Continue", track_continue);
	decl_btn("Pause", track_pause);
	decl_var_int("img_send_seg_h", &imgsegsize, 2, IMG_H/IMG_PART_H);
	img_reset();
	ui_save_cfg(defcfg);
	ui_load_cfg(cfg);
}
