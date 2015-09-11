// V4L2 video capture

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <bsd/string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>


#define DEFAULT_FRAME_COUNT 70

// AR0330 defaults
#define DEFAULT_BRIGHTNESS    168
#define DEFAULT_SHARPNESS  0x0080
#define DEFAULT_CONTRAST        0
#define DEFAULT_LEDS         0x0f

#define CLEAR(x) memset(&(x), 0, sizeof(x))

enum io_method {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
};

struct buffer {
	void *start;
	size_t length;
};

static const char *program_name;
static char *device_name;
static enum io_method io = IO_METHOD_MMAP;
struct buffer *buffers;
static unsigned int n_buffers;
static FILE *fout = NULL;


static void errno_exit(const char *s) {
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg) {
	int r;

	do {
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}


static bool process_image(const void *p, int size) {
	bool rc = true;
	if (NULL != fout && NULL != p && size > 0) {
		fwrite(p, size, 1, fout);
		fflush(stdout);
		fprintf(stderr, ".");
	} else {
		fprintf(stderr, "0");
		rc = false;
	}
	fflush(stderr);
	return rc;
}

static bool read_frame(int fd) {
	struct v4l2_buffer buf;
	unsigned int i;
	bool rc = true;

	switch (io) {
	case IO_METHOD_READ:
		if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				return false;

			case EIO:
				// Could ignore EIO, see spec
				// fall through

			default:
				errno_exit("read");
			}
		}

		rc = process_image(buffers[0].start, buffers[0].length);
		break;

	case IO_METHOD_MMAP:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				// Could ignore EIO, see spec
				// fall through

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		assert(buf.index < n_buffers);

		rc = process_image(buffers[buf.index].start, buf.bytesused);

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
			errno_exit("VIDIOC_QBUF");
		}
		break;

	case IO_METHOD_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				// Could ignore EIO, see spec
				// fall through

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		for (i = 0; i < n_buffers; ++i) {
			if (buf.m.userptr == (unsigned long)buffers[i].start
			    && buf.length == buffers[i].length) {
				break;
			}
		}

		assert(i < n_buffers);

		rc = process_image((void *)buf.m.userptr, buf.bytesused);

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
			errno_exit("VIDIOC_QBUF");
		}
		break;
	}

	return rc;
}


static void mainloop(int fd, unsigned int frame_count) {

	for (unsigned int count = 0; count < frame_count; ++count) {
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO(&fds);
			FD_SET(fd, &fds);

			// Timeout
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			r = select(fd + 1, &fds, NULL, NULL, &tv);

			if (-1 == r) {
				if (EINTR == errno) {
					continue;
				}
				errno_exit("select");
			}

			if (0 == r) {
				fprintf(stderr, "select timeout\n");
				exit(EXIT_FAILURE);
			}

			if (read_frame(fd)) {
				break;
			}
			// EAGAIN - continue select loop
		}
	}
}

static void stop_capturing(int fd) {
	enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		// Nothing to do
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type)) {
			errno_exit("VIDIOC_STREAMOFF");
		}
		break;
	}
}

static void start_capturing(int fd) {
	unsigned int i;
	enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		// Nothing to do
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
				errno_exit("VIDIOC_QBUF");
			}
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
			errno_exit("VIDIOC_STREAMON");
		}
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long)buffers[i].start;
			buf.length = buffers[i].length;

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {
				errno_exit("VIDIOC_QBUF");
			}
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
			errno_exit("VIDIOC_STREAMON");
		}
		break;
	}
}

static void uninit_device(void) {
	unsigned int i;

	switch (io) {
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i) {
			if (-1 == munmap(buffers[i].start, buffers[i].length)) {
				errno_exit("munmap");
			}
		}
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i) {
			free(buffers[i].start);
		}
		break;
	}

	free(buffers);
}

static void init_read(unsigned int buffer_size) {
	buffers = calloc(1, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

static void init_mmap(int fd) {
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				 "memory mapping\n", device_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n",
			 device_name);
		exit(EXIT_FAILURE);
	}

	buffers = calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
			errno_exit("VIDIOC_QUERYBUF");
		}

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
			mmap(NULL,                   // start anywhere
			     buf.length,
			     PROT_READ | PROT_WRITE, // required
			     MAP_SHARED,             // recommended
			     fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start) {
			errno_exit("mmap");
		}
	}
}

static void init_userp(int fd, unsigned int buffer_size) {
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count  = 4;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
				 "user pointer i/o\n", device_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	buffers = calloc(4, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = malloc(buffer_size);

		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

static void init_device(int fd, int force_format) {
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n",
				 device_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n",
			 device_name);
		exit(EXIT_FAILURE);
	}

	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o\n",
				 device_name);
			exit(EXIT_FAILURE);
		}
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o\n",
				 device_name);
			exit(EXIT_FAILURE);
		}
		break;
	}


	// Select video input, video standard and tune here


	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; // reset to default

		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				// Cropping not supported
				break;
			default:
				// Errors ignored
				break;
			}
		}
	} else {
		// Errors ignored
	}


	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	switch (force_format) {
	case 0:
		break;
	case 1:
		fmt.fmt.pix.width       = 640;
		fmt.fmt.pix.height      = 480;
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

		if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
			errno_exit("VIDIOC_S_FMT");
		}
		break;
	case 2:
		fmt.fmt.pix.width       = 1920;
		fmt.fmt.pix.height      = 1080;
		fmt.fmt.pix.field       = V4L2_PIX_FMT_SRGGB12;
		fmt.fmt.pix.field       = V4L2_FIELD_ANY;

		// Note VIDIOC_S_FMT may change width and height
		if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
			errno_exit("VIDIOC_S_FMT 1080");
		}
		break;

	default:
		// Preserve original settings as set by v4l2-ctl for example
		if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt)) {
			errno_exit("VIDIOC_G_FMT");
		}
	}

	// Buggy driver paranoia
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min) {
		fmt.fmt.pix.bytesperline = min;
	}
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min) {
		fmt.fmt.pix.sizeimage = min;
	}

	switch (io) {
	case IO_METHOD_READ:
		init_read(fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		init_mmap(fd);
		break;

	case IO_METHOD_USERPTR:
		init_userp(fd, fmt.fmt.pix.sizeimage);
		break;
	}
}

static void close_device(int fd) {
	if (-1 == close(fd)) {
		errno_exit("close");
	}
	fd = -1;
}

static int open_device(void) {
	struct stat st;

	if (-1 == stat(device_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
			 device_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", device_name);
		exit(EXIT_FAILURE);
	}

	int fd = open(device_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n",
			 device_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return fd;
}


static void set_control(int fd, int parameter, int32_t value) {
	struct v4l2_control control;
	memset(&control, 0, sizeof (control));
	control.id = parameter;
	control.value = value;

	if (-1 == ioctl(fd, VIDIOC_S_CTRL, &control)) {
		errno_exit("VIDIOC_S_CTRL");
	}
}

// print usage message and exit
static void usage(const char *message, ...) {
	if (NULL != message) {
		va_list ap;
		va_start(ap, message);
		fprintf(stderr, "error: ");
		vfprintf(stderr, message, ap);
		fprintf(stderr, "\n");
		va_end(ap);
	}
	fprintf(stderr,
		"Usage: %s [options]\n\n"
		"Version 1.3\n"
		"Options:\n"
		"-h | --help          Print this message\n"
		"-d | --device name   Video device name [%s]\n"
		"-m | --mmap          Use memory mapped buffers [default]\n"
		"-r | --read          Use read() calls\n"
		"-u | --userp         Use application allocated buffers\n"
		"-o | --output        Outputs stream to stdout\n"
		"-f | --format        Force format to 640x480 YUYV\n"
		"-t | --ten           Force format to 1920x1080 Bayer12\n"
		"-c | --count N       Number of frames to grab [%i]\n"
		"-b | --brightness N  Brightness value\n"
		"-s | --sharpness N   Sharpness value\n"
		"-n | --contrast N    Contrast value\n"
		"-l | --leds N        LEDs bitmask value\n"
		"",
		program_name, device_name, DEFAULT_FRAME_COUNT);
	exit(EXIT_FAILURE);
}


static const char short_options[] = "d:hmruo:ftc:b:s:n:l:";

static const struct option
long_options[] = {
	{ "help",       no_argument,       NULL, 'h' },
	{ "device",     required_argument, NULL, 'd' },
	{ "mmap",       no_argument,       NULL, 'm' },
	{ "read",       no_argument,       NULL, 'r' },
	{ "userp",      no_argument,       NULL, 'u' },
	{ "output",     required_argument, NULL, 'o' },
	{ "format",     no_argument,       NULL, 'f' },
	{ "ten",        no_argument,       NULL, 't' },
	{ "count",      required_argument, NULL, 'c' },
	{ "brightness", required_argument, NULL, 'b' },
	{ "sharpness",  required_argument, NULL, 's' },
	{ "contrast",   required_argument, NULL, 'n' },
	{ "leds",       required_argument, NULL, 'l' },
	{ 0, 0, 0, 0 }
};


int main(int argc, char **argv) {
	program_name = argv[0];
	device_name = "/dev/video0";

	unsigned int frame_count = DEFAULT_FRAME_COUNT;
	int force_format = 0;

	int32_t brightness = DEFAULT_BRIGHTNESS;
	int32_t sharpness = DEFAULT_SHARPNESS;
	int32_t contrast = DEFAULT_CONTRAST;
	int32_t led_value = DEFAULT_LEDS;

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

		case 'd':
			device_name = optarg;
			break;

		case 'h':
			usage(NULL);

		case 'm':
			io = IO_METHOD_MMAP;
			break;

		case 'r':
			io = IO_METHOD_READ;
			break;

		case 'u':
			io = IO_METHOD_USERPTR;
			break;

		case 'o':
		{
			size_t n = strlen(optarg) + 1;
			if (n < 2) {
				usage("missing output file name");
			}
			char *output_name = malloc(n);
			if (NULL == output_name) {
				usage("unable to allocate memory for output file name: '%s'", optarg);
			}
			strlcpy(output_name, optarg, n);
			fout = fopen(output_name, "wb");
			if (NULL == fout) {
				usage("unable to create output file name: '%s'\n", optarg);
			}
		}
		break;

		case 'f':
			force_format = 1;
			break;

		case 't':
			force_format = 2;
			break;

		case 'c':
			errno = 0;
			frame_count = strtol(optarg, NULL, 0);
			if (0 != errno) {
				usage("invalid count '%s': %d, %s", optarg, errno, strerror(errno));
			}
			break;

		case 'b':
			errno = 0;
			brightness = strtol(optarg, NULL, 0);
			if (0 != errno) {
				usage("invalid brightness '%s': %d, %s", optarg, errno, strerror(errno));
			}
			break;
		case 's':
			errno = 0;
			sharpness = strtol(optarg, NULL, 0);
			if (0 != errno) {
				usage("invalid sharpness '%s': %d, %s", optarg, errno, strerror(errno));
			}
			break;

		case 'n':
			errno = 0;
			contrast = strtol(optarg, NULL, 0);
			if (0 != errno) {
				usage("invalid contrast '%s': %d, %s", optarg, errno, strerror(errno));
			}
			break;

		case 'l':
			errno = 0;
			led_value = strtol(optarg, NULL, 0);
			if (0 != errno) {
				usage("invalid LEDs value  '%s': %d, %s", optarg, errno, strerror(errno));
			}
			break;

		default:
			usage("invalid option: '%c'", c);
		}
	}

	int fd = open_device();
	set_control(fd, V4L2_CID_BRIGHTNESS, brightness);
	set_control(fd, V4L2_CID_CONTRAST, contrast);
	set_control(fd, V4L2_CID_SHARPNESS, sharpness);
	set_control(fd, V4L2_CID_HUE, led_value);
	init_device(fd, force_format);
	start_capturing(fd);
	mainloop(fd, frame_count);
	stop_capturing(fd);
	set_control(fd, V4L2_CID_HUE, 0); // LEDs off
	uninit_device();
	close_device(fd);
	fprintf(stderr, "\n");
	fclose(fout);
	fout = NULL;
	return 0;
}
