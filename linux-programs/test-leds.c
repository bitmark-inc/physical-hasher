// test program to set values into the HUE parameter

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#if defined(__linux__)
#include <bsd/string.h>
#endif
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>


static char *program_name = NULL;
static char *device_name = NULL;

static void usage(const char *message, ...)
{
	if (NULL != message) {
		va_list ap;
		va_start(ap, message);
		fprintf(stderr, "error: ");
		vfprintf(stderr, message, ap);
		fprintf(stderr, "\n");
		va_end(ap);
	}
	fprintf(stderr,
		 "usage: %s [options]\n\n"
		 "options:\n"
		 "-h | --help          this message\n"
		 "-v | --verbose       verbose output\n"
		 "-c | --count N       number of cycles\n"
		 "-d | --device name   video device name [%s]\n"
		 "", program_name, device_name);
	exit(EXIT_FAILURE);
}


static int open_device(const char *device_name) {
	struct stat st;

	if (-1 == stat(device_name, &st)) {
		usage("cannot identify '%s': %d, %s", device_name, errno, strerror(errno));
	}

	if (!S_ISCHR(st.st_mode)) {
		usage("'%s' is not a device", device_name);
	}

	int fd = open(device_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == fd) {
		usage("cannot open '%s': %d, %s\n", device_name, errno, strerror(errno));
	}
	return fd;
}


static void *allocate(size_t length) {
	void *p = malloc(length);
	if (NULL == p) {
		perror("malloc failed");
		exit(2);
	}
	return p;
}


int main(int argc, char *argv[]) {

	if (argc >= 1 && NULL != argv[0]) {
		size_t n = strlen(argv[0]) + 1;
		program_name = allocate(n);
		strlcpy(program_name, argv[0], n);
	}

	static const char short_options[] = "hvd:c:";

	static const struct option
		long_options[] = {
		{ "help",    no_argument,       NULL, 'h' },
		{ "verbose", no_argument,       NULL, 'v' },
		{ "device",  required_argument, NULL, 'd' },
		{ "count",   required_argument, NULL, 'c' },
		{ 0, 0, 0, 0 }
	};

	device_name = "/dev/video0";

	int verbose = 0;
	int count = 0;

	for (;;) {
		int idx;
		int c;

		c = getopt_long(argc, argv, short_options, long_options, &idx);

		if (-1 == c) {
			break;
		}

		switch (c) {
		case 0: // getopt_long() flag
			break;

		case 'v':
			++verbose;
			break;

		case 'd':
			device_name = optarg;
			break;

		case 'c':
			errno = 0;
			count = strtol(optarg, NULL, 0);
			if (0 != errno) {
				usage("invalid count '%s': %d, %s", optarg, errno, strerror(errno));
			}
			break;

		case 'h':
			usage(NULL);
			return 0;

		default:
			usage("invalid option: %c", c);
		}
	}

	if (count <= 0) {
		usage("invalid count: %d", count);
	}

	int fd = open_device(device_name);


	struct v4l2_queryctrl queryctrl;
	memset(&queryctrl, 0, sizeof(queryctrl));
	queryctrl.id = V4L2_CID_HUE;

	if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
		if (errno != EINVAL) {
			usage("VIDIOC_QUERYCTRL error: (%d) '%s'", errno, strerror(errno));
		} else {
			usage("V4L2_CID_HUE is not supported");
		}
	} else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
		usage("V4L2_CID_HUE is not supported");
	} else {
		printf("%s type: 0x%08x\n"
		       "  minimum: 0x%08x  %10d\n"
		       "  maximum: 0x%08x  %10d\n"
		       "  step :   0x%08x  %10d\n"
		       "  default: 0x%08x  %10d\n"
		       "  flags:   0x%08x  %10d\n"
		       "",
		       queryctrl.name, queryctrl.type,
		       queryctrl.minimum, queryctrl.minimum,
		       queryctrl.maximum, queryctrl.maximum,
		       queryctrl.step, queryctrl.step,
		       queryctrl.default_value, queryctrl.default_value,
		       queryctrl.flags, queryctrl.flags);

		struct v4l2_control control;
		memset(&control, 0, sizeof (control));
		control.id = V4L2_CID_HUE;
		control.value = queryctrl.default_value + 100;

		if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control)) {
			usage("VIDIOC_S_CTRL: (%d) '%s'", errno, strerror(errno));
		}

	}


	for (int n = 0; n < count; ++n) {

		printf("loop: %d\n", n);
#if 0
		for (int32_t value = 0; value < 16; ++value) {
			struct v4l2_control control;
			memset(&control, 0, sizeof (control));
			control.id = V4L2_CID_HUE;
			control.value = value;
			if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control) && errno != ERANGE) {
				usage("VIDIOC_S_CTRL: (%d) '%s'", errno, strerror(errno));
			}
			usleep(100000);
		}
#else
		for (int32_t value = 1; value < 16; value <<= 1) {
			printf("  set: 0x%04x\n", value);

			struct v4l2_control control;
			memset(&control, 0, sizeof (control));
			control.id = V4L2_CID_HUE;
			control.value = value;
			if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control) && errno != ERANGE) {
				usage("VIDIOC_S_CTRL: (%d) '%s'", errno, strerror(errno));
			}

#if 0
			if (0 == ioctl(fd, VIDIOC_G_CTRL, &control)) {
				control.value = value;
				// The driver may clamp the value or return ERANGE, ignored here
				if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control) && errno != ERANGE) {
					usage("VIDIOC_S_CTRL: (%d) '%s'", errno, strerror(errno));
				}
			} else if (errno != EINVAL) {
				usage("VIDIOC_G_CTRL: (%d) '%s'", errno, strerror(errno));
			}
#endif
			usleep(500000);
		}
#endif
	}

	return 0;
}
