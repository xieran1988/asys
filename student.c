#include <algo.h>

int MB_W=10; 	//宏块宽
int MB_H=10; 	//宏块高
int MB_Y=8; 	//向上搜索范围
int MB_X=1; 	//两侧搜索范围，增量是Y/X
int MB_D=4; 
int MB_th=600;  //启动搜索阈值
int MB_th2=200; //终止搜索阈值
int MB_th3=1500; //无效搜索阈值
int MM_W=5;		//统计块宽度
int MM_H=7;		//统计块高度
int MB_count=300;	//有效阈值
int X_line=150;		//忽略前排
int X_line2=150;	//忽略后排
float R_zoom=1;
float R_pan=0;
float R_tilt=0;
float zoom_min=1;
float zoom_max=3;
float PAN_RANGE=46;
float TILT_RANGE=25;
float Timer=-1;
float TimeOut=1000;
float MaxTimeOut=15000;
float MTimer=0;
int	TimerTODO=0;
int bRun=1;
int bPause=0;
int DirectT=1;
img_t PREV, COMP, PRECOMP, PRECOMP2, PRECOMP3, PRECOMP4, PRECOMP5, PRECOMP6, PRECOMP7, MOTION;

void OnTimer()
{
	switch(TimerTODO)
	{
	case 0:break;		
	case 1:
		if(DirectT!=1)
		{
			DirectT=1;
			mcu_send("192.168.1.3","PDI PDMINPUT=12576;#",21);//切T
			MTimer=-1;
		}		
		break;
	case 2:
		if(DirectT!=0)
		{
			DirectT=0;
			mcu_send("192.168.1.3","PDI PDMINPUT=12321;#",21);//切S
			MTimer=now();
		}
		break;
	default:break;
	}
}

int MM_count(int sx, int sy)
{
	int count=0;
	int i,j;
	for(i=0;i<MM_W*MB_W;i+=MB_W)
            for(j=0;j<MM_H*MB_H;j+=MB_H)
            {
                if(sx+i>=IMG_W || sy+j>=IMG_H)
                    continue;
				//应该是/20,/15是为了增加斜率
                count += (COMP.data[sy+j][sx+i]-128)/4;
                count += (PRECOMP.data[sy+j][sx+i]-128)/4;
                count += (PRECOMP2.data[sy+j][sx+i]-128)/4;
                count += (PRECOMP3.data[sy+j][sx+i]-128)/4;
                count += (PRECOMP4.data[sy+j][sx+i]-128)/4;
                count += (PRECOMP5.data[sy+j][sx+i]-128)/4;
                //count += (PRECOMP6.data[sy+j][sx+i]-128)>>4;
                //count += (PRECOMP7.data[sy+j][sx+i]-128)>>4;
            }

	return count;
}

int MB_compare(img_t *src, img_t *dst,
		int sx, int sy,
		int dx, int dy)
{
	int count=0;
	int i,j;
	for(i=0;i<MB_W;i++)
		for(j=0;j<MB_H;j++)
		{
			count += abs(src->data[sy+j][sx+i] - dst->data[dy+j][dx+i]);
		}
	return count;
}

int MB_search(img_t *src, img_t *dst,
		int mbx, int mby,int *rth)
{
        int x,y,k,th=10000,min_th,ds;
        ds=30000;
	min_th=30000;
        *rth=30000;
        //该部分太多临界值……
        for(k=1;k<=MB_Y;k++)
        {
            for(x=mbx-k/MB_X;x<=mbx+k/MB_X;x++)
            {
                if(x<0 || x>=IMG_W-MB_W)
                    continue;
                //向上
                y=mby-k;
                if(y>=0)
                {
                    th=MB_compare(src,dst,mbx,mby,x,y);
                    if(th<min_th)
                    {
                        min_th=th;
                        ds=k;
                        *rth=th;
                    }
                }

                //向下
                y=mby+k;
                if(y<IMG_H-MB_H)
                {
                    th=MB_compare(src,dst,mbx,mby,x,y);
                    if(th<min_th)
                    {
                        min_th=th;
                        ds=-k;
                        *rth=th;
                    }
                }

                //终止
                if(*rth<MB_th2)
                    return ds;
            }

        }

	if(min_th>MB_th3)
		return 0;

	return ds;
}

void process()
{
/*
	static int tmc=0;
	tmc++;
	if(tmc==30)
	{
		tmc=0;
		printf("+++++++\n");
		set_indoor();
		
	}
*/	
	if(DirectT!=1)
	{
		if(MTimer!=-1 && now()-MTimer>MaxTimeOut)
		{
			MTimer=-1;
			Timer=now();
			TimerTODO=1;
			set_target_pan_tilt(R_pan,R_tilt);
			set_zoom(R_zoom);
		}
		
	}

	if(bRun==0 || bPause==1)
	{
		set_target_pan_tilt(R_pan,R_tilt);
		set_zoom(R_zoom);
		TimerTODO=0;
		return;
	}
	img_fill_rect(&Y, 0, IMG_H-X_line, IMG_W, X_line, 0);
	img_fill_rect(&Y, 0, 0, IMG_W, X_line2, 0);
	img_fill(&COMP, 128);
	img_fill(&MOTION, 128);
//\033[2J

//	printf("\033[2J");

        int x=0,y=0,ds,fc,cs,ms,mx=0,my=0,rth=0,test=0;
	for(x=0;x<IMG_W-MB_W;x+=MB_W)
        for(y=X_line2;y<IMG_H-X_line && y<IMG_H-MB_H;y+=MB_H)
        {
            if(MB_compare(&PREV,&Y,x,y,x,y)>MB_th)
            {
				test++;
                ds=MB_search(&PREV,&Y,x,y,&rth);
				if(abs(ds)<MB_D)
					ds=0;
                fc=128+ds*5;
                if(fc>255)
                    fc=255;
                if(fc<0)
                    fc=0;
                if(fc!=128)
                    img_fill_rect(&COMP, x, y, MB_W, MB_H, fc);
            }
        }
	mon("test:%d\n", test);

	cs=0;
	ms=0;
	for(x=0;x<IMG_W-MB_W;x+=MB_W)
		for(y=X_line2;y<IMG_H-X_line && y<IMG_H-MB_H;y+=MB_H)
		{
			cs=MM_count(x,y);
			if(abs(cs)>abs(ms))
			{
				ms=cs;
				mx=x;
				my=y;
			}
		}

	if(Timer>0)
	{
		if(now()-Timer>TimeOut)
		{
			Timer=-1;
			OnTimer();
			TimerTODO=0;
		}
	}

	

	if(ms>MB_count)
	{
		img_fill_rect(&MOTION, mx, my, MB_W*MM_W, MB_H*MM_H, 255);
		float tp = (float)(mx - IMG_W/2) / (IMG_W/2) * (PAN_RANGE/2);
		float tt = (float)(IMG_H/2-my-MB_H*MM_H/2) / (IMG_H/2) * (TILT_RANGE/2);
		set_target_pan_tilt(tp, tt);
		float zz=zoom_min+((float)(IMG_H-X_line2-my)/(IMG_H-X_line2-X_line))*(zoom_max-zoom_min);
		if(zz<1)
			zz=1;
		set_zoom(zz);
		Timer=now();
		TimerTODO=2;
		printf("0000000000000000000000000000000000000000000000000000000000000000 %f \n",zz);
	}

	if(ms<-MB_count*0.8)
	{
		img_fill_rect(&MOTION, mx, my, MB_W*MM_W, MB_H*MM_H, 0);
		set_target_pan_tilt(R_pan,R_tilt);
		set_zoom(R_zoom);
		Timer=now();
		TimerTODO=1;
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	}
	printf("ms ------------------------------------------------------------ %d,%d \n",ms,my);
	img_copy(&PREV, &Y);
	//img_copy(&PRECOMP7, &PRECOMP6);
	//img_copy(&PRECOMP6, &PRECOMP5);
	img_copy(&PRECOMP5, &PRECOMP4);
	img_copy(&PRECOMP4, &PRECOMP3);
	img_copy(&PRECOMP3, &PRECOMP2);
	img_copy(&PRECOMP2, &PRECOMP);
	img_copy(&PRECOMP, &COMP);
	
}

void decl()
{
	decl_img("y", &Y);
	decl_img("comp", &COMP);
	decl_img("motion", &MOTION);

	decl_var_int("MB_count", &MB_count, 0, 500);
	decl_var_int("X_line", &X_line, 0, 240);
	decl_var_int("X_line2", &X_line2, 0, 240);
	decl_var_float("PAN_RANGE", &PAN_RANGE, 0, 120);
	decl_var_float("TILT_RANGE", &TILT_RANGE, 0, 120);
	decl_var_float("R_zoom", &R_zoom, 1, 10);
	decl_var_float("R_pan", &R_pan, -90, 90);
	decl_var_float("R_tilt", &R_tilt, -90, 90);
	decl_var_float("zoom_min", &zoom_min, 1, 10);
	decl_var_float("zoom_max", &zoom_max, 1, 10);
	decl_var_float("PD_Delay", &TimeOut, 0, 10000);
	decl_var_float("PD_Timeout", &MaxTimeOut, 0, 100000);


	decl_var_int("MB_W", &MB_W, 2, 50);
	decl_var_int("MB_H", &MB_H, 2, 50);
	decl_var_int("MB_D", &MB_D, 0, 10);
	decl_var_int("MB_Y", &MB_Y, 0, 50);
	decl_var_int("MB_X", &MB_X, 0, 20);
	decl_var_int("MB_th", &MB_th, 100, 10000);
	decl_var_int("MB_th2", &MB_th2, 100, 10000);
	decl_var_int("MB_th3", &MB_th3, 100, 10000);
	decl_var_int("MM_W", &MM_W, 0, 50);
	decl_var_int("MM_H", &MM_H, 0, 50);	
}

int main(int argc, char *argv[]) 
{
	run(argc, argv);
	return 0;
}
