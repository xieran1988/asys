#include <algo.h>
int bMoving;
double cur_pan, cur_tilt;
static double pan;
static double tilt;
static double zoom;
static double targetPan;
static double targetTilt;
static double targetZoom;
static double PTTemp,ZTemp;    //用于控制变速，暂时采用简单的线性变化(每增加一度，速度加1)
static double PTAccuracy, ZAccuracy;
static int ID;
static int protocol;
static char rbuf[100];
static int buf[50];
static int tty_fd;


static int MyRead(char *bt,int maxlen);
static void MyWrite(char *bt,int len);
static void PTZMove(int h, int v);
static void PTZZoom(int speed);

double offset_pan = 0;
double offset_tilt = 0;
int	call_delay=30;
int	call_timeout=20;

struct sockaddr_in sersa;
void set_indoor()
{
	char bytes[]={0x80,0x01,0x04,0x35,0x01,0xFF};
	MyWrite(bytes,6);
}

void comm_decl()
{
	
	decl_var_double("offset_pan", &offset_pan, -50, 50);
	decl_var_double("offset_tilt", &offset_tilt, -50, 50);
	decl_var_double("pan_tilt_speed", &PTTemp, 0, 4);
	decl_var_double("zoom_speed", &ZTemp, 0, 5);
	decl_var_double("pan_tilt_accuracy", &PTAccuracy, 0.1, 5);
	decl_var_double("zoom_accuracy", &ZAccuracy, 0.1, 5);
	decl_var_int("call_delay", &call_delay, 10, 500);
	decl_var_int("call_timeout", &call_timeout, 10, 500);

}

static void MyPTZ()
{
	bMoving=1;
	ID=1;
	protocol=0;
	pan=0;
	tilt=0;
	zoom=1;
	PTTemp=0.2;
	ZTemp=0.5;
	PTAccuracy = 0.5;
	ZAccuracy = 0.1;
	targetZoom = 1;
	targetPan = 0;
	targetTilt = 0;
}

static void Init()
{
	//这里要读那个全局的配置参数
	ID=1;//一般是这样的，除非用了菊花链。
	protocol=0;//先只做Visca的Sony D系列，后面我来扩展
	set_zoom(1);
	
}

static void CallZoom()
{
	if(bRun==0)
		return;
	char getZoom[]={0x80,0x09,0x04,0x47,0xFF};
	MyWrite(getZoom,5);
}
static void CallPanTilt()
{
	if(bRun==0)
		return;
	char getPT[]={0x80,0x09,0x06,0x12,0xFF};
	MyWrite(getPT,5);
}
/*
void PTZABS(double p, double t, double z)
{

	unsigned long dp,dt;
	if(p>=0)
		dp=(unsigned long)(p/180.0*2400);
	else
		dp=(unsigned long)(16*16*16*16+p/180.0*2400);	
	if(t>=0)
		dt=(unsigned long)(t/180.0*2400);
	else
		dt=(unsigned long)(16*16*16*16+t/180.0*2400);
	
	char setPos[]={0x81,0x01,0x06,0x02,0x06,0x06,0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0xFF,0xFF};
	setPos[6]=(char)(dp>>12);
	dp-=dp>>12<<12;
	setPos[7]=(char)(dp>>8);
	dp-=dp>>8<<8;
	setPos[8]=(char)(dp>>4);
	dp-=dp>>4<<4;
	setPos[9]=(char)(dp);

	setPos[10]=(char)(dt>>12);
	dt-=dt>>12<<12;
	setPos[11]=(char)(dt>>8);
	dt-=dt>>8<<8;
	setPos[12]=(char)(dt>>4);
	dt-=dt>>4<<4;
	setPos[13]=(char)(dt);
	MyWrite2(setPos,15);

	char setZoom[]={0x81,0x01,0x04,0x47,0x01,0x01,0x01,0x01,0xFF,0xFF};

	double	a=-42799.32,
			b=0.49368463,
			c=21118.889,
			d=0.63396036;

	double xd=pow(z,d);
	double px=(a*b+c*xd)/(b+xd);
	if(px<0)
		px=0;
	long dz=px;
	
	
	setZoom[4]=(char)(dz>>12);
	dz-=dz>>12<<12;
	setZoom[5]=(char)(dz>>8);
	dz-=dz>>8<<8;
	setZoom[6]=(char)(dz>>4);
	dz-=dz>>4<<4;
	setZoom[7]=(char)(dz);

	MyWrite2(setZoom,9);

}*/

void get_cur_pan_tilt(double *p, double *t)
{
	*p = cur_pan;
	*t = cur_tilt;
}

void set_target_pan_tilt(double p,double t)
{
	targetPan=p+offset_pan;
	targetTilt=t+offset_tilt;
}

void set_zoom(double z)
{
	targetZoom=z;
}

static void MyWrite(char *bt,int len)
{
	bt[0]+=ID;
	//printf("writing %d\n", len);
	int i = sendto(tty_fd, bt, len, 0, (struct sockaddr *)&sersa, sizeof(sersa));	
	//int i = write(tty_fd, bt, len);
	if (i < 0) {
		log("write failed: %s\n", strerror(errno));
		exit(1);
	}
	//printf("write ok %d\n", i);
}

static void readPanTilt(int *bt)
{
	short pp=0,tt=0;
	pp+=bt[2];
	pp=pp<<4;
	pp+=bt[3];
	pp=pp<<4;
	pp+=bt[4];
	pp=pp<<4;
	pp+=bt[5];

	tt+=bt[6];
	tt=tt<<4;
	tt+=bt[7];
	tt=tt<<4;
	tt+=bt[8];
	tt=tt<<4;
	tt+=bt[9];

	//后面加入多协议支持
	double p=180.0*(double)(pp)/2400;
	double t=180.0*(double)(tt)/2400;
/*
	int k;
	for(k=0;k<11;k++)
	{
		printf("%02X ",bt[k]);
	}
	printf("\n");
*/
	if(1)//结果合理判断
	{
		double dp, dt;
		pan=p;
		tilt=t;
	
		dp = (targetPan - pan);
		dt = -(targetTilt - tilt);
		if (fabs(dp) < PTAccuracy) {
			dp = 0;
		} else {
			dp = dp * PTTemp;
			if (dp > 0) 
				dp += 1;
			else
				dp -= 1;
		}

		if (fabs(dt) < PTAccuracy) {
			dt = 0;
		} else {
			dt = dt * PTTemp;
			if (dt > 0)
				dt += 1;
			else
				dt -= 1;
		}
		PTZMove(dp, dt);
	}

	cur_pan = p;
	cur_tilt = t;
	//printf("-------------------------------------- cur_pan %lf cur_tilt %lf time %lf \n", p, t, now());
}

static void readZoom(int *bt)
{
	short zm=0;
	zm+=bt[2];
	zm=zm<<4;
	zm+=bt[3];
	zm=zm<<4;
	zm+=bt[4];
	zm=zm<<4;
	zm+=bt[5];

	//后面加入多协议支持

// for X18
	double  a=-42799.32,
			b=0.49368463,
			c=21118.889,
			d=0.63396036;

/*
// for X10
	double  a=-28249.3980628,
			b=0.771340788777,
			c=22270.2880859,
			d=0.747895056549;
*/
	double tm=(b*zm-a*b)/(c-zm);
	double z=pow(tm,1/d);
	if(z>18)
		z=18;
	//printf("cur_zoom %lf\n", z);
	if(z>0.99 && z<18.1)
	{
		zoom=z;
		double dz = -(targetZoom - zoom);
		if (fabs(dz) < ZAccuracy)
			dz = 0;
		else {
			dz = dz * ZTemp;
			if (dz > 0) 
				dz += 1;
			else
				dz -= 1;
		}
		PTZZoom(dz);
		mon("cur_zoom:%f\n", z);
	}
}


static void MyOnRead()
{
	static int k=0;
	int i;
	int len = MyRead(rbuf,100);
	for(i=0;i<len;i++)
	{
		if(k>=50)
 			 k=0;
		buf[k]=rbuf[i];
		k++;
		char a = 0xff;

		if(buf[k-1]==a)//buf[k-1]==0xFF
		{
			//printf("enter\n");
			if(k==7)
			{
				readZoom(buf);
			}
			if(k==11)
			{
				readPanTilt(buf);
			}
			k=0;
		}
	}
}


static void PTZMove(int h, int v)//移动，参数是速度
{

	if(h>8)
		h=8;
	if(h<-8)
		h=-8;

	if(v>8)
		v=8;
	if(v<-8)
		v=-8;


	//过滤多余指令
	static int lh=0,lv=0;
	if(lh==h && lv==v)
		return;
	lh=h;
	lv=v;

	char bt[]={0x80,0x01,0x06,0x01,0x11,0x22,0x03,0x03,0xFF};
	if(h>0)
		bt[6]=0x02;
	if(h<0)
		bt[6]=0x01;
	if(v>0)
		bt[7]=0x02;
	if(v<0)
		bt[7]=0x01;
	if(h<0)
		bt[4]=-h;
	else
		bt[4]=h;
	if(v<0)
		bt[5]=-v;
	else
		bt[5]=v;

	bMoving=1;
	MyWrite(bt,9);
	if(h==0 && v==0)
	{
		bMoving=0;
	}
}

static void PTZZoom(int speed)
{
	//过滤多余指令
	static int ls;

	if(ls==speed)
		return;
	if(speed>6)
		speed=6;
	if(speed<-6)
		speed=-6;
	ls=speed;

	char setZoom[]={0x80,0x01,0x04,0x07,0x00,0xFF};
	if(speed>0)
	{
		setZoom[4]=0x30;
	}
	if(speed<0)
	{
		setZoom[4]=0x20;
		speed=-speed;
	}

	setZoom[4]+=speed;

	MyWrite(setZoom,6);
}

static int MyRead(char *bt,int maxlen)
{
	//printf("reading %d\n", maxlen);
	struct sockaddr_in sa;
	int salen;
	int i = recvfrom(tty_fd, bt, maxlen, 0, (struct sockaddr *)&sa, (socklen_t *)&salen);	
	//int i = read(tty_fd, bt, maxlen);
	if (i < 0) {
		log("read failed: %s\n", strerror(errno));
		exit(1);
	}
	//printf("read ok %d\n", i);
	return i;
}

static pthread_t th;

static void *thread_routine(void *_d)
{
	log("comm thread running\n");
	while (1) {
		CallZoom();
		while (1) {
			struct timeval tv = {};
			tv.tv_sec = 0;
			tv.tv_usec = 1000*call_timeout;
			fd_set fdsr;
			FD_ZERO(&fdsr);
			FD_SET(tty_fd, &fdsr);
			if (select(tty_fd + 1, &fdsr, NULL, NULL, &tv) == 1) 
				MyOnRead();
			else 
				break;
		}
		CallPanTilt();
		usleep(call_delay*1000);
		while (1) {
			struct timeval tv = {};
			tv.tv_sec = 0;
			tv.tv_usec = 1000*call_timeout;
			fd_set fdsr;
			FD_ZERO(&fdsr);
			FD_SET(tty_fd, &fdsr);
			if (select(tty_fd + 1, &fdsr, NULL, NULL, &tv) == 1) 
				MyOnRead();
			else 
				break;
		}
		//else 
		//	printf("failed!!!: %s\n", strerror(errno));
		usleep(call_delay*1000);
	}
	return NULL;
}

void comm_exit()
{
//	log("kill comm thread\n");
//	pthread_kill(th, 9);
	log("close tty\n");
	close(tty_fd);
}

void comm_init()
{
	// FIX: init tty_fd -> socket
	int bufsiz = 1*1024*1024;
	tty_fd = socket(AF_INET, SOCK_DGRAM, 0);
	sersa.sin_family = AF_INET;
	sersa.sin_addr.s_addr = inet_addr("127.0.0.1");
	sersa.sin_port = htons(2010);
	setsockopt(tty_fd, SOL_SOCKET, SO_RCVBUF, (const char *)&bufsiz, sizeof(bufsiz));
	if (setsockopt(tty_fd, SOL_SOCKET, SO_SNDBUF, (const char *)&bufsiz, sizeof(bufsiz))) {
		log("setsockopt SO_SNDBUF failed\n");
		panic();
	}

	MyPTZ();
	Init();

	if (pthread_create(&th, NULL, thread_routine, NULL)) {
		log("comm thread create failed\n");
		panic();
	}

	log("comm init ok\n");
}



