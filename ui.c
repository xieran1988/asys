#include <algo.h>

static enum {
	RD, WR
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

void decl_var_double(const char *name, double *p, double min, double max)
{
	if (mode == WR) {
		startnode("double");
		attr("name", "%s", name);
		attr("max", "%lf", max);
		attr("min", "%lf", min);
		attr("val", "%lf", *p);
		endnode();
	}
	if (mode == RD && nodeis("double") && attris("name", name)) {
		getattr("val", "%lf", p);
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

void ui_init()
{
	writexml("test.xml");
	readxml("test.xml");
}

