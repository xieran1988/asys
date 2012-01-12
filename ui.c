#include <algo.h>

static enum {
	RD, WR, IMG
} mode;
static xmlTextReaderPtr reader;
static xmlTextWriterPtr writer;

#define startnode(s) xmlTextWriterStartElement(writer, (xmlChar *)s);
#define endnode() xmlTextWriterEndElement(writer);
#define attr(s, ...) xmlTextWriterWriteFormatAttribute(writer, (xmlChar *)s, __VA_ARGS__)
#define nodeis(s) !strcmp((char *)xmlTextReaderConstName(reader), s)
#define nodeattr(s) (char *)xmlTextReaderGetAttribute(reader, (xmlChar *)s)
#define attris(s, v) !strcmp(nodeattr(s), v)
#define getattr(s, ...) sscanf(nodeattr(s), __VA_ARGS__)

void decl_img(const char *name, img_t *img)
{
	if (mode == WR) {
		startnode("img");
		endnode();
	}
}

void decl_btn(const char *name, void (*cb)(const char *))
{
	if (mode == WR) {
		startnode("btn");
		attr("name", "%s", name);
		endnode();
	}
}

void decl_var_int(const char *name, int *p, int min, int max)
{
	if (mode == WR) {
		startnode("int");
		attr("name", "%s", name);
		attr("max", "%d", max);
		attr("min", "%d", min);
		attr("val", "%d", *p);
		endnode();
	}
	if (mode == RD && nodeis("int") && attris("name", name)) {
		getattr("val", "%d", p);
	}
}

void decl_var_float(const char *name, float *p, float min, float max)
{
	if (mode == WR) {
		startnode("float");
		attr("name", "%s", name);
		attr("max", "%f", max);
		attr("min", "%f", min);
		attr("val", "%f", *p);
		endnode();
	}
	if (mode == RD && nodeis("float") && attris("name", name)) {
		getattr("val", "%f", p);
	}
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

static void all_decl()
{
	decl();
	comm_decl();
	decl_var_int("T_FRM", &T_FRM, 50, 500);
	decl_btn("RUN", track_run);
	decl_btn("STOP", track_stop);
	decl_btn("Continue", track_continue);
	decl_btn("Pause", track_pause);
}

static FILE *jpgfp;
static img_t jpgimg;
static struct jpeg_compress_struct jcs;
static struct jpeg_error_mgr jem;

static void writejpgstart(img_t *img)
{
	jpgfp = fopen(JPGPATH ".tmp", "w+");
	memcpy(&jpgimg, img, sizeof(jpgimg));
	jpeg_create_compress(&jcs);
	jpeg_stdio_dest(&jcs, jpgfp);
	jcs.err = jpeg_std_error(&jem);
	jcs.image_width = IMG_W;
	jcs.image_height = IMG_H;
	jcs.input_components = 1;
	jcs.in_color_space = JCS_GRAYSCALE;
	jpeg_set_defaults(&jcs);
	jpeg_set_quality(&jcs, 10, TRUE);
	jpeg_start_compress(&jcs, TRUE);
}

static void writejpglines(int len)
{
//	while (jcs.next_scanline < jcs.image_height) {
	while (len--) {
		JSAMPROW r[] = { (JSAMPROW)jpgimg.data[jcs.next_scanline] };
		jpeg_write_scanlines(&jcs, r, 1);
	}
}

static void writejpgend()
{
	jpeg_finish_compress(&jcs);
	jpeg_destroy_compress(&jcs);
	fclose(jpgfp);
	rename(JPGPATH ".tmp", JPGPATH);
}

static FILE *bmpfp;
static img_t bmpimg;

static void writebmpstart()
{
	static unsigned char hdr[0x436] = {
		0x42,0x4D,0x36,0x30,0x01,0x00,0x00,0x00,0x00,0x00,0x36,0x04,0x00,0x00,
		0x28,0x00,0x00,0x00,0x40,0x01,0x00,0x00,0xF0,0x00,0x00,0x00,0x01,0x00,
		0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x2C,0x01,0x00,0x12,0x0B,0x00,0x00,
		0x12,0x0B,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,
	};
	int i;
	*(int *)(hdr + 0x12) = IMG_W;
	*(int *)(hdr + 0x16) = IMG_H;
	for (i = 0; i < 256; i++) {
		*(unsigned int *)(hdr + 0x36 + i*4) =
				i | (i << 8) | (i << 16);
	}
	bmpfp = fopen(BMPPATH ".tmp", "wb+");
	fwrite(hdr, 1, sizeof(hdr), bmpfp);
	ftruncate(fileno(bmpfp), sizeof(hdr) + IMG_W*IMG_H);
	log("write %s\n", BMPPATH);
}

static void writebmplines(int len)
{
	int start = ((int)ftell(bmpfp)-0x436)/IMG_W;
	int i;
	for (i = 0; i < len; i++)
		fwrite(bmpimg.data[start+len-1-i], 1, IMG_W, bmpfp);
}

static void writebmpend()
{
	fclose(bmpfp);
	rename(BMPPATH ".tmp", BMPPATH);
}

static void writeyuv(img_t *img, const char *name) 
{
	FILE *fp = fopen(name, "w+");
	fwrite(img->data, 1, IMG_W*IMG_H, fp);
	fclose(fp);
}

static void readxml(const char *name)
{
	mode = RD;
	reader = xmlReaderForFile(name, NULL, 0);
	xmlTextReaderRead(reader); // skip root
	while (xmlTextReaderRead(reader) == 1) {
		all_decl();
	}
	xmlFreeTextReader(reader);
}

static void writexml(const char *name)
{
	mode = WR;
	writer = xmlNewTextWriterFilename(name, 0);
	xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL);
	startnode("root");
	all_decl();
	endnode();
	xmlFreeTextWriter(writer);
}

void ui_int()
{
	static int start;

	if (!start) {
	}
}

void ui_init()
{
	/*
	int i, j;
	for (i = 0; i < IMG_H; i++)
		for (j = 0; j < IMG_W; j++)
			Y.data[i][j] = rand()*9;
	*/
//	writexml("/tmp/test.xml"); // 5ms
//	readxml("/tmp/test.xml"); // 7ms
//	writeyuv(&Y, "/tmp/a.jpg"); // 5ms
//	writeimg(&Y, "/tmp/a.jpg"); // 50ms
//	writeimg(&Y, "/tmp/a.jpg"); // half lines=25ms
	log("start jpg start %lf\n", now());
	writejpgstart(&Y);
	log("start jpg lines %lf\n", now());
	writejpglines(IMG_H);
	log("end jpg lines %lf\n", now());
	writejpgend();
//	writebmpstart(); // 2ms
//	writebmplines(&Y, 0, IMG_H); // 7ms
//	writebmplines(&Y, 0, IMG_H/2); // 4ms
//	writebmpend(); // 1ms
//	memcpy(&bmpimg, &Y, sizeof(Y)); 3ms
}


