#define NULL 0
#define libSceSystemService 38
#define libkernel 0x2001

using int32_t = int;

static_assert(sizeof(int32_t) == 4, "sizeof(int32_t != 4)");

struct LocalProcessArgs {
	int fds[2];
	int a;
	int b;
	unsigned long long pad[2];
};

struct AppStatus {
	unsigned int id;
	unsigned char pad[28];
};

struct Args {
	struct Result {
		int32_t state;
		int32_t err;
	} result;
	int (*sceSystemServiceGetAppStatus)(AppStatus *status);
	int (*sceSystemServiceAddLocalProcess)(unsigned int id, const char *path, const char **argv, LocalProcessArgs *args);
	int (*socketpair)(int domain, int type, int protocol, int *fds);
	int (*usleep)(int usec);
	int *(*errno)();
};

void respawn(Args *args) {

	// NOTE: The way these values are assigned is important
	// everything must be kept on the stack, no .rodata
	// this will simplify loading

	AppStatus status;
	int res = args->sceSystemServiceGetAppStatus(&status);
	if (res < 0) [[unlikely]] {
		args->result = {2, res};
		while (true) {args->usleep(1000000);}
	}

	// NOTE: volatile to prevent the use of data
	volatile LocalProcessArgs param;
	// {{0, 0}, 1, -1, {0, 0}};
	param.fds[0] = 0;
	param.fds[1] = 0;
	param.a = 1;
	param.b = -1;
	param.pad[0] = 0;
	param.pad[1] = 0;

	if (args->socketpair(1, 1, 0, (int *) param.fds) == -1) [[unlikely]] {
		args->result = {3, *args->errno()};
		while (true) {args->usleep(1000000);}
	}

	// "/app0/eboot.bin"
	volatile unsigned long long path[2];
	path[0] = 0x62652F307070612F;
	path[1] = 0x6E69622E746F6F;

	/*
	volatile unsigned long long path[5];
	path[0] = 0x5F6D65747379732F;
	path[1] = 0x422F7070612F7865;
	path[2] = 0x3030303030574552;
	path[3] = 0x622E746F6F62652F;
	path[4] = 0x6E69;
	*/

	const char *argv[] = {NULL};

	res = args->sceSystemServiceAddLocalProcess(status.id, (char *)path, argv, (LocalProcessArgs *) &param);
	if (res < 0) [[unlikely]] {
		args->result = {2, res};
		while (true) {args->usleep(1000000);}
	}

	args->result.state = 1;

	for (;;) {
		// sleep for one second intervals
		// nothing too crazy so that redis can properly resume
		args->usleep(1000000);
	}
}
