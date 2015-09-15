// create-png.c

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <bsd/string.h>
#include <getopt.h>

#include "ahd_bayer.h"


// image processing oprions
typedef struct {
	bool slider;
	bool number;
	bool compensation;
} image_options_t;

// global variables
static const char *program_name;
static const char *prefix = "frame";
static int verbose = 0; // incremented by --verbose / -v

// prototypes
static bool write_png(const ahd_pixel_t *image, int width, int height, const char *path);
static void fill(ahd_pixel_t *image, int x1, int y1, int x2, int y2, int width, int height, uint16_t red, uint16_t green, uint16_t blue);
static void number(int value, ahd_pixel_t *image, int start_x, int start_y, int size, int width, int height);
static int make_frames(int start, int limit, image_options_t options, const char *output_prefix, const char *input_file);


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
		"-v | --verbose       Output messages\n"
		"-d | --no-comp       Disable compensation\n"
		"-s | --slider        Add slider\n"
		"-n | --number        Add frame number\n"
		"-p | --prefix T      Prefix [%s]\n"
		"-c | --count N       Limit number of frames [no-limit]\n"
		"",
		program_name, prefix);
	exit(EXIT_FAILURE);
}


static const char short_options[] = "hvdsnp:c:";

static const struct option
long_options[] = {
	{ "help",       no_argument,       NULL, 'h' },
	{ "verbose",    no_argument,       NULL, 'v' },
	{ "slider",     no_argument,       NULL, 's' },
	{ "number",     no_argument,       NULL, 'n' },
	{ "no-comp",    no_argument,       NULL, 'd' },
	{ "prefix",     required_argument, NULL, 'p' },
	{ "count",      required_argument, NULL, 'c' },
	{ 0, 0, 0, 0 }
};


int main(int argc, char **argv) {
	program_name = argv[0];

	image_options_t options = {
		.slider = true,
		.number = true,
		.compensation = true
	};
	int frame_count = 0;
	for (;;) {
		int idx = 0;
		int c = getopt_long(argc, argv, short_options, long_options, &idx);

		if (-1 == c) {
			break;
		}

		switch (c) {
		case 0: // getopt_long() flag
			break;

		case 'h':
			usage(NULL);

		case 'v':
			++verbose;
			break;

		case 's':
			options.slider = true;
			break;

		case 'n':
			options.number = true;
			break;

		case 'd':
			options.compensation = false;
			break;

		case 'p':
		{
			size_t n = strlen(optarg) + 1;
			if (n < 2) {
				usage("missing output file name");
			}
			char *prefix = malloc(n);
			if (NULL == prefix) {
				usage("unable to allocate memory for prefix: '%s'", optarg);
			}
			strlcpy(prefix, optarg, n);
		}
		break;

		case 'c':
			errno = 0;
			frame_count = strtol(optarg, NULL, 0);
			if (0 != errno) {
				usage("invalid count '%s': %d, %s", optarg, errno, strerror(errno));
			}
			break;

		default:
			usage("invalid option: '%c'", c);
		}
	}

	if (optind >= argc) {
		usage("missing arguments");
	}

	if (verbose > 1) {
		printf("verbose level: %d\n", verbose);
	}
	int count = 0;
	for (int i = optind; i < argc; ++i) {
		count = make_frames(count, frame_count, options, prefix, argv[i]);
	}

	return EXIT_SUCCESS;
}


static bool write_png(const ahd_pixel_t *image, const int width, const int height, const char *path) {

	bool rc = false; // assume failure

	const int depth = 8 * sizeof(ahd_pixel_t);       // number of bits in a pixel

	FILE *fp = fopen(path, "wb");
	if (NULL == fp) {
		goto fopen_failed;
	}

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (NULL == png_ptr) {
		goto png_create_write_struct_failed;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (NULL == info_ptr) {
		goto png_create_info_struct_failed;
	}

	// error handling
	if (setjmp(png_jmpbuf(png_ptr))) {
		goto png_failure;
	}

	// image attributes
	png_set_IHDR(png_ptr, info_ptr,
		     width, height, depth,
		     PNG_COLOR_TYPE_RGB,
		     PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_DEFAULT,
		     PNG_FILTER_TYPE_DEFAULT);

	// PNG row pointers
	png_byte **row_pointers = png_malloc(png_ptr, height * sizeof(png_byte *));
	for (size_t y = 0; y < height; ++y) {
		row_pointers[y] = (png_byte *)&image[y * width * 3];  // R G B pixel order
	}

	// create PNG
	png_init_io(png_ptr, fp);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	// no need to free the actual rows as these are owned by the caller
	png_free(png_ptr, row_pointers);

	rc = true; // success

png_failure:
png_create_info_struct_failed:
	png_destroy_write_struct(&png_ptr, &info_ptr);
png_create_write_struct_failed:
	fclose(fp);
fopen_failed:
	return rc;  // success if true
}


// fill starting at (x1, y1) to  < (x2, y2) in RGB bitmap of size width, height
static void fill(ahd_pixel_t *image, int x1, int y1, int x2, int y2, int width, int height, uint16_t red, uint16_t green, uint16_t blue) {

	for (int y = y1; y < y2 && y < height; ++y) {
		ahd_pixel_t *pixel = &image[3 * x1 + 3 * width * y];
		for (int x = x1; x < x2 && x < width; ++x) {
			*pixel++ = red;
			*pixel++ = green;
			*pixel++ = blue;
		}
	}
}


// stamp a 4 digit number into the bitmap
static void number(int value, ahd_pixel_t *image, int start_x, int start_y, int size, int width, int height) {
	static const uint16_t bitmaps[] = {
		075557, // 0
		026227, // 1
		025127, // 2
		071717, // 3
		013571, // 4
		074616, // 5
		034652, // 6
		071122, // 7
		075757, // 8
		035311  // 9
	};

	int divisor = 1000;
	value %= 9999;
	for (int i = 0; i < 4; ++i, divisor /= 10) {
		int d = value / divisor;
		uint16_t bm = bitmaps[d % 10];
		int x_begin = start_x + 4 * size * i;
		for (int y = 0, ys = start_y; y < 5; ++y, ys += size) {
			for (int x = 0, xs = x_begin; x < 3; ++x, xs += size) {
				if (0 != (040000 & bm)) {
					fill(image, xs, ys, xs + size, ys + size, width, height, 0xff, 0xff, 0xff);
				} else {
					fill(image, xs, ys, xs + size, ys + size, width, height, 0x00, 0x00, 0x00);
				}
				bm <<= 1;
			}
		}
	}
}


static int make_frames(int start, int limit, image_options_t options, const char *output_prefix, const char *input_file) {
	FILE *fp = fopen(input_file, "rb");
	if (NULL == fp) {
		usage("failed to open input file: '%s'", input_file);
		return 0;
	}

	if (verbose > 1) {
		printf("opened input file: '%s'\n", input_file);
	}

	int rc = limit;
	for (int count = start; (0 == limit) || (count < limit); ++count) {
		char output_name[256];
		if (snprintf(output_name, sizeof(output_name), "%s%04d.png", output_prefix, count) >= sizeof(output_name)) {
			usage("failed to create output name - increase buffer size");
			rc = count;
			break;
		}

		// extract one frame from the input file
		const int width = 1920;
		const int height = 1080;
		ahd_pixel_t pixels[width * height];
		if (1 != fread(pixels, sizeof(pixels), 1, fp)) {
			rc = count;
			break;
		}

		if (verbose > 2) {
			for (int line = 0; line < 8; ++line) {
				printf("line: %2d: ", line);
				const uint8_t *p = (uint8_t *)&pixels[width * line];
				for (int col = 0; col < 8; ++col) {
					printf(" %02x", *p++);
				}
				printf("\n");
			}
		}

		ahd_pixel_t image[3 * width * height];
		if (!ahd_decode(pixels, width, height, image, BAYER_TILE_GRBG)) {
			usage("failed to demosaic");
			return 0;
		}

		if (verbose > 0) {
			printf("creating: %s\n", output_name);
		}

		if (options.slider) {
			fill(image, count + 0, 10, count + 20, 20, width, height, 0x00, 0x00, 0x00);
			fill(image, count + 5, 10, count + 10, 20, width, height, 0xff, 0xff, 0xff);
		}

		if(options.number) {
			number(count, image, 10, 30, 4, width, height);
		}
		write_png(image, width, height, output_name);
	}

	fclose(fp);
	return rc;
}
