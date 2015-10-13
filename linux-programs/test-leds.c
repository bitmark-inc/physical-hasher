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

// returns the default value
#define CHECK(fd, control) check_control(fd, #control, control)
static uint32_t check_control(int fd, const char *name, uint32_t id) {

	uint32_t result = 0;

	struct v4l2_queryctrl queryctrl;
	memset(&queryctrl, 0, sizeof(queryctrl));
	queryctrl.id = id;

	if (-1 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
		if (errno != EINVAL) {
			usage("VIDIOC_QUERYCTRL(%s) error: (%d) '%s'",  name, errno, strerror(errno));
		} else {
			usage("%s is not supported", name);
		}
	} else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
		usage("%s is not supported", name);
	} else {
		struct v4l2_control control;
		memset(&control, 0, sizeof (control));
		control.id = id;
		control.value = 0;
		if (-1 == ioctl(fd, VIDIOC_G_CTRL, &control)) {
			usage("VIDIOC_G_CTRL: (%d) '%s'", errno, strerror(errno));
		}
		printf("%s type: 0x%08x\n"
		       "  minimum: 0x%08x  %10d\n"
		       "  maximum: 0x%08x  %10d\n"
		       "  step :   0x%08x  %10d\n"
		       "  default: 0x%08x  %10d\n"
		       "  current: 0x%08x  %10d\n"
		       "  flags:   0x%08x  %10d\n"
		       "",
		       queryctrl.name, queryctrl.type,
		       queryctrl.minimum, queryctrl.minimum,
		       queryctrl.maximum, queryctrl.maximum,
		       queryctrl.step, queryctrl.step,
		       queryctrl.default_value, queryctrl.default_value,
		       control.value, control.value,
		       queryctrl.flags, queryctrl.flags);
		result = queryctrl.default_value;
	}
	return result;
}


static void set_control(int fd, uint32_t id, uint32_t value) {
	struct v4l2_control control;
	memset(&control, 0, sizeof (control));
	control.id = id;
	control.value = value;
	if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control) && errno != ERANGE) {
		usage("VIDIOC_S_CTRL: (%d) '%s'", errno, strerror(errno));
	}
}


static void set_leds(int fd, uint32_t value) {
	set_control(fd, V4L2_CID_HUE, value);
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

	uint32_t default_hue = CHECK(fd, V4L2_CID_HUE);
	uint32_t default_contrast = CHECK(fd, V4L2_CID_CONTRAST);
	uint32_t default_sharpness = CHECK(fd, V4L2_CID_SHARPNESS);
	uint32_t default_brightness = CHECK(fd, V4L2_CID_BRIGHTNESS);

	for (int n = 0; n < count; ++n) {

		printf("loop: %d\n", n);

		for (int32_t value = 1; value < 16; value <<= 1) {
			printf("  set: 0x%04x\n", value);
			set_leds(fd, value);
			usleep(500000);
		}
	}

	// set all off
	printf("turn off\n");
	set_leds(fd, 0);

	set_control(fd, V4L2_CID_HUE, default_hue);
	set_control(fd, V4L2_CID_CONTRAST, default_contrast);
	set_control(fd, V4L2_CID_SHARPNESS, default_sharpness);
	set_control(fd, V4L2_CID_BRIGHTNESS, default_brightness);


	// finished
	close(fd);
	return 0;
}
