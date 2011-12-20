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
	static double t_frame, t_process, cp, ct, t_last, t_ui, t_total;

	t_total = now();

	net_send_and_recv();

	if (frame_no == 0) 
		log("got first frame\n");

	t_frame = now() - t_last;
	t_last = now();

	mon("no: %d fps: %.2lf\n", frame_no, 1000 / t_frame);
	ui_mon();

	t_process = now();
	process();
	t_process = now() - t_process;

	get_cur_pan_tilt(&cp, &ct);
	mon("cur_pan %.2lf cur_tilt %.2lf\n", cp, ct);

	mon("T_frame: %.2lfms\n", t_frame);
	mon("T_process: %.2lfms\n", t_process);

	t_ui = now();
	ui_send();
	t_ui = now() - t_ui;
	mon("T_ui: %.2lfms\n", t_ui);

	t_total = now() - t_total;

	printf("%d real %.2lfms set %.2lfms used %.2lfms\n", 
			frame_no, t_frame, T_FRM*1., t_total);

	frame_no++;
}

void run(int argc, char *argv[])
{
	signal(2, doexit);
	signal(6, doexit);
	signal(9, doexit);
	signal(15, doexit);
	signal(13, doexit);
	setbuf(stdout, NULL);

	log("Copyright 2011. Build: %s. pid %d\n", __DATE__, getpid());

	comm_init();
	net_init();
	cam_init();
	ui_init();
	decl_var_int("T_FRM", &T_FRM, 50, 500);

	cam_loop(argc, argv);
}

