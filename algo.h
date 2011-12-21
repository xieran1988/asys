#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <dirent.h>
#include <termios.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <libxml/encoding.h>

/*
 * Basic
 */
#define ARM_PORT1 1650
#define PC_PORT1 1660
#define PC_PORT2 1661

#define IMG_W 720
#define IMG_H 576

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef struct {
	u8 data[IMG_H][IMG_W];
} img_t; 

/*
 * Utils
 */
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
float now();
#define mon(fmt, args...) {}
#define log(fmt, args...) { printf("%s: " fmt, __func__, ##args); syslog(LOG_DEBUG, "%s: " fmt, __func__, ##args); }
#define panic() { log("panic at %s() %s:%d\n", __func__, __FILE__, __LINE__); abort(); }
int selected_read(int fd);
static inline void img_copy(img_t *a, img_t *b) { memcpy(a->data, b->data, sizeof(a->data)); }
void img_bitblt(img_t *dst, int dx, int dy, img_t *src, int sx, int sy, int w, int h);
static inline void img_fill(img_t *dst, u8 val) { memset(dst->data, val, sizeof(img_t)); }
void img_fill_rect(img_t *p, int x, int y, int w, int h, int val);
void img_plot_rect(img_t *p, int x, int y, int w, int h, int val);
void img_draw_cross(img_t *p, int x, int y, int val);

/*
 * Mcu
 */
void track_send(char *ip,char *buf,int len);

/*
 * UI
 */
void decl_img(const char *name, img_t *img);
void decl_btn(const char *name, void (*cb)(const char *));
void decl_var_double(const char *name, double *p, double min, double max);
void decl_var_int(const char *name, int *p, int min, int max);
void decl();
void comm_decl();
void ui_init();

/*
 * Camera
 */
extern int T_FRM;
extern img_t Y;
void cam_loop(int argc, char *argv[]);
void cam_callback();
void cam_init();
void cam_exit();

/*
 * Comm
 */
void set_target_pan_tilt(double p, double t);
void get_cur_pan_tilt(double *p, double *t);
void set_zoom(double z);
void set_indoor();
//void PTZABS(double p, double t, double z);
void mcu_send(char *ip,char *buf,int len);
extern int bMoving;
extern double cur_pan, cur_tilt;
extern double offset_pan;


//#define PAN_RANGE 53
//#define TILT_ANGLE 0
void comm_init();
void comm_exit();

/*
 * Yours
 */
void process();
extern int bRun;
extern int bPause;
extern int DirectT;


/*
 * System
 */
void run(int argc, char *argv[]);


