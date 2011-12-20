#include <algo.h>

static double PAN_RANGE=75;
double CAM_RANGE=48;
int compare_th = 14;
int compare_th2 = 22;
int UB_W = 80;
int UB_H = 50;
int	UB_T = 15000;
double UB_th = 0.01;
int CB_W = 15;
int CB_H = 20;
double CB_th = 0.1;
int LB_th = 1500;
int dark = 22;
double dark_th = 1.3;
double B_Alpha = 0.15;
double zoom_normal = 2.0;
double zoom_close = 2.5;
double R_time = 2000;
double R_zoom = 1;
double R_pan = 0;
double R_tilt = -10;
double zoom_limt=5000;
double S_time=3000;
double M_angle=8;
double SA_p=4;
double SA_z=0.2;

/*
static double PAN_RANGE=75;
//static double TILT_RANGE=25;
static double R_timeout = 20*1000;
static double R_zoom = 1;
static double R_pan = 0;
static double R_tilt = 0;
static double last_lost_tm = -1;
*/
static int bInited = 0;
imgsel_t mask;
img_t BACK,	//背景 
	PREV, 	//上一帧
	UBAK,	//背景更新参考值
	COMP, 	//帧间计算结果
	COMPB, 	//背景计算结果
	CLUS, 	//聚类计算结果
	LABEL;	//连通域结果
int bRun=1;
int bPause=0;
int DirectT=1;

typedef struct{ int left;
             int right;
             int top;
             int bottom;
             int XCenter;
             int YCenter;
             int area;
}box;
box Box[1000];

typedef struct{ int px;
             int py;
                struct NODE *link;

}NODE;
NODE *p,*q,*t,*queue=NULL,*tail=NULL, *Connected[1000];

int label()
{
    int nHeight=mask.y2-mask.y1;//二值图高，宽
    int nWidth=mask.x2-mask.x1;
	int x1=mask.x1;
	int y1=mask.y1;
    enum{BLACK=255,WHITE=0};
    
    //char (*LABEL)[2000]=new char[nHeight][2000];//二值图

         int i,j,k,cx,cy,currentX,currentY;

    for(k=0;k<1000;k++) Connected[k]=NULL;
    
    //扫描文档LABEL
       k=0;
       for(cy=y1;cy<y1+nHeight;cy++)
        for(cx=x1;cx<x1+nWidth;cx++) 
            if(LABEL.data[cy][cx]==BLACK)//若遇到一个前景像点p(x,y)
            { //把该点链接到Connected[k]
              p=(NODE *)malloc(sizeof(NODE));
              p->py=cy;  p->px=cx;
              p->link=Connected[k];//这个链表是后进的在前面
              Connected[k]=p;
              //把该点从LABEL文档中消去
              LABEL.data[cy][cx]=WHITE;
              //把该点进队到队列queue的尾部
              q=(NODE *)malloc(sizeof(NODE));
              q->py=cy; q->px=cx; q->link=NULL;  
              if(queue==NULL){ queue=q; tail=q; }
              else      { tail->link=q; tail=q; }
              while( queue!=NULL )// 队列queue 非空 
              {//取出队首结点作为当前点
               currentY=queue->py;  currentX=queue->px;
               //若当前点与LABEL中的前景像点Pi(x,y)在8邻域相邻接
                for(i=-1;i<=1;i++)
                    for(j=-1;j<=1;j++)//LABEL[currentY+0][currentX+0]已经消去
                                        //图像前景像点加、减1不能越过数组边界
                        if((currentY+i)>=y1 && (currentY+i)<y1+nHeight
                            && (currentX+j)>=x1 && (currentX+j)<x1+nWidth
                             && LABEL.data[currentY+i][currentX+j]==BLACK )
                        {//把这些前景像点链接到Connected[k]
                         p=(NODE *)malloc(sizeof(NODE));
                         p->py=currentY+i; p->px=currentX+j;
                         p->link=Connected[k];//这个链表是后进的在前面
                         Connected[k]=p;
                         //把这些前景像点从LABEL文档中消去
                         LABEL.data[currentY+i][currentX+j]=WHITE;
                         //把这些前景像点进队到队列queue的尾部
                         q=(NODE *)malloc(sizeof(NODE));
                         q->py=currentY+i; q->px=currentX+j; q->link=NULL;  
                         tail->link=q; tail=q; 
                        }
               //释放队首结点
                q=queue; queue=queue->link; free(q);
              }
             k=k+1;
            }


	int c=0;
    for(i=0;i<k;i++)
    { Box[c].left=IMG_W-1;
      Box[c].right=0;
      Box[c].top=IMG_H-1;
      Box[c].bottom=0;
	  Box[c].area=0;
      p=Connected[i]; 
      while(p!=NULL)
      { if(Box[c].left>p->px)   Box[c].left=p->px;
        if(Box[c].right<p->px)  Box[c].right=p->px;
        if(Box[c].top>p->py)    Box[c].top=p->py;
        if(Box[c].bottom<p->py) Box[c].bottom=p->py;
		t=p->link;
		free(p);
        p=t;
		++Box[c].area;
      }
	
	  if(Box[c].area < LB_th)
	  {
		continue;
	  }
	  //Box[c].XCenter=(Box[i].right+Box[i].left)/2;
	  //Box[c].YCenter=(Box[i].bottom+Box[i].top)/2;
	  img_fill_rect(&LABEL, Box[c].left, Box[c].top, Box[c].right-Box[c].left, Box[c].bottom-Box[c].top, 255/(c+1));
	  ++c;
	  
    }
	return c;
}

void updataBack()
{
	int x=0,y=0,m=0,n=0,count=0;
	for (y = mask.y1; y < mask.y2; y+=UB_H)
		for (x = mask.x1; x < mask.x2; x+=UB_W)
		{
			count=0;
			for(n = y; n < y+UB_H && n < mask.y2; ++n)
				for(m = x; m < x+UB_W && m < mask.x2; ++m)
				{
					count+=!!COMP.data[n][m];
				}
			
			if(count < (m-x)*(n-y)*UB_th)
			{
				++UBAK.data[y][x];
				if(UBAK.data[y][x] > UB_T/T_FRM)
				{
					for(n = y; n < y+UB_H && n < mask.y2; ++n)
						for(m = x; m < x+UB_W && m < mask.x2; ++m)
						{
							//高斯混合....
							BACK.data[n][m] = Y.data[n][m] * B_Alpha + BACK.data[n][m] * (1.0 - B_Alpha);
						}
				}
				
			}
			else
			{
				UBAK.data[y][x]=0;
			}
		}
}

void cluster()
{
	int x=0,y=0,m=0,n=0,count=0,ch=CB_H/3,cw=CB_W/3;
	for (y = mask.y1; y < mask.y2; y+=ch)//减小复杂度，从逐点计算改为折半计算
		for (x = mask.x1; x < mask.x2; x+=cw)
		{
			count=0;
			for(n = y; n < y+CB_H && n < mask.y2; ++n)
				for(m = x; m < x+CB_W && m < mask.x2; ++m)
				{
					count+=!!COMPB.data[n][m];
				}
			if(count < (m-x)*(n-y)*CB_th)
			{
				for(n = y; n < y+ch && n < mask.y2; ++n)
					for(m = x; m < x+cw && m < mask.x2; ++m)
					{
						CLUS.data[n][m]=0;
					}
			}
			else
			{
				for(n = y; n < y+CB_H && n < mask.y2; ++n)
					for(m = x; m < x+CB_W && m < mask.x2; ++m)
					{
						CLUS.data[n][m]=255;
					}
			}
		}
}

void myTimer(double *p,double *t,double *z,int c)
{
	static double last_tm = 0;
	double add=now()-last_tm;
	last_tm = now();
	static double stop_tm = 0; //bMoving==0的时间
	static double zoom_tm = 0; //上一次改变zoom的时间
	static double lost_tm = 0; //丢失目标时间
	static double zoom_state=0;
	static double last_p=0;
	static double last_t=0;
	static double last_z=0;
	if(zoom_state<0.99)
	{
		zoom_state=zoom_normal;
	}


	if(bMoving)
	{
		stop_tm=0;
	}else{
		
		stop_tm+=add;
	}
	
	if(c==0)
	{
		lost_tm+=add;
	}else{
		lost_tm=0;
		
		//在此插入学生跟踪continue指令
		track_send("192.168.1.31","Continue",9);
	}

	zoom_tm += add;


	if(c==0)//丢失目标
	{
		if(lost_tm > R_time)
		{
			//在此插入学生跟踪pause指令
			track_send("192.168.1.31","Pause",6);
			*p=R_pan;
			*t=R_tilt;
		
			if(zoom_tm > zoom_limt)
			{
				*z=R_zoom;
				zoom_tm = 0;
			}
		}
		else
		{
			*p=last_p;
			*t=last_t;
			*z=last_z;
		}
	}else if(c==1){
		
		if(fabs(*p+offset_pan-cur_pan)>M_angle && zoom_tm > zoom_limt)//移动过大
		{
			*z=zoom_normal;
			zoom_state=zoom_normal;
			zoom_tm = 0;
		}else if(stop_tm>S_time && zoom_tm > zoom_limt){//静止超时
			*z=zoom_close;
			zoom_state=zoom_close;
			zoom_tm = 0;
		}else{//正常状态
			if(*z>zoom_state)
				*z=zoom_state;
		}
	}else{//多目标
		if(*z>zoom_normal)
			*z=zoom_normal;
	}


	//防抖
	if(fabs(*p-last_p) < SA_p)
		*p=last_p;
	else
		last_p=*p;

	last_t=*t;

	//防抖
	if(fabs(*z-last_z) < SA_z)
		*z=last_z;
	else	
		last_z=*z;
	
}

void process()
{
/*
	static int tmc=0;
	tmc++;
	if(tmc==30)
	{
		tmc=0;
		set_indoor();
	}
*/
	int x, y;


	//初始化背景
	if(bInited==0)
	{
		static int initCount=0;
		if(initCount<30)
		{
			initCount++;
			img_copy(&BACK, &Y);
		}
		else
		{
			bInited=1;
			img_fill(&UBAK, 0);
			set_target_pan_tilt(0, 0);
			set_zoom(1);
		}
		return;
	}

	img_fill(&COMP, 0);
	img_fill(&COMPB, 0);
	img_fill(&CLUS, 0);
	img_fill(&LABEL, 0); 

	//计算准备
	for (y = mask.y1; y < mask.y2; y++)
		for (x = mask.x1; x < mask.x2; x++)
		{
			COMP.data[y][x] = abs(Y.data[y][x] - PREV.data[y][x]) > compare_th ? 255 : 0;
			if(Y.data[y][x] < dark || PREV.data[y][x] < dark)
				COMP.data[y][x] = abs(Y.data[y][x] - PREV.data[y][x]) > compare_th*dark_th ? 255 : 0;

			COMPB.data[y][x] = abs(Y.data[y][x] - BACK.data[y][x]) > compare_th2 ? 255 : 0;
			if(Y.data[y][x] < dark || BACK.data[y][x] < dark)
				COMPB.data[y][x] = abs(Y.data[y][x] - BACK.data[y][x]) > compare_th2*dark_th ? 255 : 0;
		}
	img_copy(&PREV, &Y);

	

	//选择性更新背景
	updataBack();

	//聚类
	cluster();

	img_copy(&LABEL, &CLUS);

	//连通域
	int k=label();

	img_plot_rect(&Y, mask.x1, mask.y1, 
			mask.x2 - mask.x1, mask.y2 - mask.y1, 128);

	
	//输出连通域信息
	int i=0;
	mon("count: %d\n", k);
	for(i=0;i<k;i++)
	{
		mon("area%d: %d\n", i, Box[i].area);
		
	}

	double p=0,t=0,z=1;
	int ml=IMG_W,mr=0,cx=0,dx;
	for(i=0;i<k;i++)
	{
		if(Box[i].left < ml) ml = Box[i].left;
		if(Box[i].right > mr) mr = Box[i].right;
	}
	cx=(ml+mr)/2;
	dx=(mr-ml)*1.3;
	
	p = (double)(cx - IMG_W/2) / (IMG_W/2) * (PAN_RANGE/2);
	

	z = CAM_RANGE / ((double)dx / IMG_W * PAN_RANGE);

	if(z<1.0)
		z=1;
	
	printf("--------%d\n",k);

	myTimer(&p,&t,&z,k);
	set_target_pan_tilt(p, t);
	set_zoom(z);
	
	
	mon("PZ:%f,%f\n", p, z);
	
}

void decl()
{

	decl_img("Y", &Y);
	decl_img("BACK", &BACK);
	decl_img("COMP", &COMP);
	decl_img("COMPB", &COMPB);
	decl_img("CLUS", &CLUS);
	decl_img("LABEL", &LABEL);

	decl_var_int("X1", &mask.x1, 0, IMG_W);
	decl_var_int("Y1", &mask.y1, 0, IMG_H);
	decl_var_int("X2", &mask.x2, 0, IMG_W);
	decl_var_int("Y2", &mask.y2, 0, IMG_H);
	decl_var_double("PAN_RANGE", &PAN_RANGE, 0, 120);
	decl_var_double("CAM_RANGE", &CAM_RANGE, 30, 120);
	decl_var_int("LB_th", &LB_th, 200, 10000);
	decl_var_double("zoom_normal", &zoom_normal , 1, 18);
	decl_var_double("zoom_close", &zoom_close , 1, 18);
	decl_var_double("zoom_limt", &zoom_limt , 0, 50000);
	decl_var_double("S_time", &S_time , 0, 50000);
	decl_var_double("M_angle", &M_angle , 0, 30);
	decl_var_double("SA_p", &SA_p , 0, 30);
	decl_var_double("R_zoom", &R_zoom, 1, 10);
	decl_var_double("R_pan", &R_pan, -90, 90);
	decl_var_double("R_tilt", &R_tilt, -90, 90);
	decl_var_double("R_time", &R_time, 100, 100*1000);
	decl_var_int("compare_th", &compare_th, 0, 50);
	decl_var_int("compare_th2", &compare_th2, 0, 50);
	decl_var_int("UB_W", &UB_W, 0, 200);
	decl_var_int("UB_H", &UB_H, 0, 200);
	decl_var_int("UB_T", &UB_T, 0, 50000);	
	decl_var_double("UB_th", &UB_th, 0, 0.2);
	decl_var_int("CB_W", &CB_W, 0, 200);
	decl_var_int("CB_H", &CB_H, 0, 200);
	decl_var_double("CB_th", &CB_th, 0, 1);	
	decl_var_int("dark", &dark, 0, 50);
	decl_var_double("dark_th", &dark_th, 0, 5);
	decl_var_double("B_Alpha", &B_Alpha, 0, 1);
	decl_var_double("SA_z", &SA_z, 0, 1);

/*	
	//decl_imgsel("maskarea", &mask);
	decl_var_int("compare_th", &compare_th, 0, 255);
	decl_var_int("teacher_width", &teacher_w, 5, 80);
	
		
	
//	decl_var_double("TILT_RANGE", &TILT_RANGE, 0, 120);
*/
}

int main(int argc, char *argv[]) 
{
	mask.stat = IMGSEL_OK;
	mask.x1=IMG_W*0.1;
	mask.y1=IMG_H*0.4;
	mask.x2=IMG_W*0.9;
	mask.y2=IMG_H*0.65;
	decl();
	
	run(argc, argv);

	return 0;
}


