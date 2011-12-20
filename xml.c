#include <algo.h>

void decl_img(const char *name, img_t *img)
{
}

void decl_btn(const char *name, void (*cb)(const char *))
{

void decl_var_int(const char *name, int *p, int min, int max)
{
}

void decl_var_double(const char *name, double *p, double min, double max)
{
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
	decl_btn("RUN", track_run);
	decl_btn("STOP", track_stop);
	decl_btn("Continue", track_continue);
	decl_btn("Pause", track_pause);
}

