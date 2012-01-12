#include <algo.h>

static int frame_no;
int T_FRM =160;

static void doexit(int i)
{
	log("got sig %d\n", i);
	comm_exit();
	cam_exit();
	log("all done\n");
	exit(0);
}

void cam_callback()
{
	static float t_frame, t_process, cp, ct, t_last, t_total;

	t_total = now();

	if (frame_no == 0) 
		log("got first frame\n");

	t_frame = now() - t_last;
	t_last = now();

	mon("no: %d fps: %.2f\n", frame_no, 1000 / t_frame);

	t_process = now();
	process();
	t_process = now() - t_process;

	get_cur_pan_tilt(&cp, &ct);
	mon("cur_pan %.2f cur_tilt %.2f\n", cp, ct);

	mon("T_frame: %.2fms\n", t_frame);
	mon("T_process: %.2fms\n", t_process);

	ui_int();

	t_total = now() - t_total;

	printf("%d set %.2fms used %.2fms\n", 
			frame_no, T_FRM*1., t_total);

	frame_no++;
}

float float_test(float k) {
	float dd = 33*k;
	dd += 44.2;
	return dd;
}

void run(int argc, char *argv[])
{
	signal(2, doexit);
	signal(6, doexit);
	signal(9, doexit);
	signal(15, doexit);
	signal(13, doexit);
	setbuf(stdout, NULL);

	utils_init();
//	log("%f\n", float_test((float)getpid()));
//	log("%f\n", now());
//	log("%f\n", now());
//	comm_init();
	cam_init();
	log("%f\n", now());
	ui_init();
	log("%f\n", now());
#ifdef UITEST
	return ;
#endif

	cam_loop(argc, argv);
}


