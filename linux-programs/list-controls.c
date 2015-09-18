// simple test program to list all supported controls

#include <stdio.h>
#include <stdlib.h>
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
		 "-d | --device name   video device name [%s]\n"
		 "", program_name, device_name);
	exit(EXIT_FAILURE);
}

static void enumerate_menu(const int fd, struct v4l2_queryctrl *queryctrl) {
	struct v4l2_querymenu querymenu;

	printf("Menu items:\n");

	memset(&querymenu, 0, sizeof(querymenu));
	querymenu.id = queryctrl->id;

	for (querymenu.index = queryctrl->minimum; querymenu.index <= queryctrl->maximum; ++querymenu.index) {
		if (0 == ioctl(fd, VIDIOC_QUERYMENU, &querymenu)) {
			printf("  %s\n", querymenu.name);
		} else {
			usage("VIDIOC_QUERYMENU error: (%d) '%s'", errno, strerror(errno));
		}
	}
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
		exit(EXIT_FAILURE);
	}
	return p;
}


int main(int argc, char *argv[]) {

	if (argc >= 1 && NULL != argv[0]) {
		size_t n = strlen(argv[0]) + 1;
		program_name = allocate(n);
		strlcpy(program_name, argv[0], n);
	}

	static const char short_options[] = "hvd:";

	static const struct option
		long_options[] = {
		{ "help",    no_argument,       NULL, 'h' },
		{ "verbose", no_argument,       NULL, 'v' },
		{ "device",  required_argument, NULL, 'd' },
		{ 0, 0, 0, 0 }
	};

	device_name = "/dev/video0";

	int verbose = 0;

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

		case 'h':
			usage(NULL);
			return 0;

		default:
			usage("invalid option: %c", c);
		}
	}

	int fd = open_device(device_name);

	// iterate through all controls and display their data
	struct v4l2_queryctrl queryctrl;
	memset(&queryctrl, 0, sizeof(queryctrl));

	for (queryctrl.id = V4L2_CID_BASE; queryctrl.id < V4L2_CID_LASTP1; ++queryctrl.id) {
		if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
			if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
				continue;
			}

			struct v4l2_control control;
			memset (&control, 0, sizeof (control));
			control.id = queryctrl.id;

			if (0 != ioctl(fd, VIDIOC_G_CTRL, &control)) {
				usage("VIDIOC_G_CTRL error: (%d) '%s'", errno, strerror(errno));
			}

			printf("Control %s type: 0x%08x:\n"
			       "  value:   0x%08x  %10d\n"
			       "  minimum: 0x%08x  %10d\n"
			       "  maximum: 0x%08x  %10d\n"
			       "  step :   0x%08x  %10d\n"
			       "  default: 0x%08x  %10d\n"
			       "  flags:   0x%08x  %10d\n"
			       "",
			       queryctrl.name, queryctrl.type,
			       control.value, control.value,
			       queryctrl.minimum, queryctrl.minimum,
			       queryctrl.maximum, queryctrl.maximum,
			       queryctrl.step, queryctrl.step,
			       queryctrl.default_value, queryctrl.default_value,
			       queryctrl.flags, queryctrl.flags);

			if (queryctrl.type == V4L2_CTRL_TYPE_MENU) {
				enumerate_menu(fd, &queryctrl);
			}
		} else {
			if (errno == EINVAL) {
				continue;
			}
			usage("VIDIOC_QUERYCTRL error: (%d) '%s'", errno, strerror(errno));
		}
	}

	// check for any private controls
	for (queryctrl.id = V4L2_CID_PRIVATE_BASE; ; ++queryctrl.id) {
		if (0 == ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl)) {
			if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
				continue;
			}
			printf("Private Control %s type: 0x%08x:\n"
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

			if (queryctrl.type == V4L2_CTRL_TYPE_MENU) {
				enumerate_menu(fd, &queryctrl);
			}
		} else {
			if (errno == EINVAL) {
				break;
			}
			usage("VIDIOC_QUERYCTRL error: (%d) '%s'", errno, strerror(errno));
		}
	}

	close(fd);
	return 0;
}
