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


// coloured pixel
typedef struct {
	uint16_t red;
	uint16_t green;
	uint16_t blue;
} pixel_t;

// image
typedef struct {
	pixel_t *pixels;
	size_t width;
	size_t height;
} bitmap_t;

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
static pixel_t *pixel_at(bitmap_t *bitmap, int x, int y);
static int save_png_to_file(bitmap_t *bitmap, const char *path);
static uint16_t average_red(pixel_t *a, pixel_t *b, pixel_t *c, pixel_t *d);
static uint16_t average_green(pixel_t *a, pixel_t *b, pixel_t *c, pixel_t *d);
static uint16_t average_blue(pixel_t *a, pixel_t *b, pixel_t *c, pixel_t *d);
static void fill(bitmap_t *frame, int start_x, int start_y, int width, int height, uint16_t red, uint16_t green, uint16_t blue);
static void number(int value, bitmap_t *frame, int start_x, int start_y, int width, int height);
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


// Given "bitmap", this returns the pixel of bitmap at the point ("x", "y")

static pixel_t *pixel_at(bitmap_t *bitmap, int x, int y) {
	return bitmap->pixels + bitmap->width * y + x;
}

// Write "bitmap" to a PNG file specified by "path"; returns 0 on success, non-zero on error

static int save_png_to_file(bitmap_t *bitmap, const char *path) {
	FILE *fp;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	size_t x, y;
	png_byte **row_pointers = NULL;
	// "status" contains the return value of this function. At first
	// it is set to a value which means 'failure'. When the routine
	// has finished its work, it is set to a value which means
	// 'success'
	int status = -1;
	// The following number is set by trial and error only. I cannot
	// see where it it is documented in the libpng manual.

	int pixel_size = 3*2;  // 3 colours @ 2 bytes
	int depth = 16;

	fp = fopen(path, "wb");
	if (!fp) {
		goto fopen_failed;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		goto png_create_write_struct_failed;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		goto png_create_info_struct_failed;
	}

	// Set up error handling

	if (setjmp(png_jmpbuf(png_ptr))) {
		goto png_failure;
	}

	// Set image attributes

	png_set_IHDR(png_ptr,
		      info_ptr,
		      bitmap->width,
		      bitmap->height,
		      depth,
		      PNG_COLOR_TYPE_RGB,
		      PNG_INTERLACE_NONE,
		      PNG_COMPRESSION_TYPE_DEFAULT,
		      PNG_FILTER_TYPE_DEFAULT);

	// Initialize rows of PNG

	row_pointers = png_malloc(png_ptr, bitmap->height * sizeof(png_byte *));
	for (y = 0; y < bitmap->height; ++y) {
		png_byte *row =
			png_malloc(png_ptr, sizeof(uint8_t) * bitmap->width * pixel_size);
		row_pointers[y] = row;
		for (x = 0; x < bitmap->width; ++x) {
			pixel_t *pixel = pixel_at(bitmap, x, y);
			*row++ = pixel->red;
			*row++ = pixel->red >> 8;
			*row++ = pixel->green;
			*row++ = pixel->green >> 8;
			*row++ = pixel->blue;
			*row++ = pixel->blue >> 8;
		}
	}

	// Write the image data to "fp"

	png_init_io(png_ptr, fp);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	// The routine has successfully written the file, so we set
	// "status" to a value which indicates success

	status = 0;

	for (y = 0; y < bitmap->height; y++) {
		png_free(png_ptr, row_pointers[y]);
	}
	png_free(png_ptr, row_pointers);

png_failure:
png_create_info_struct_failed:
	png_destroy_write_struct(&png_ptr, &info_ptr);
png_create_write_struct_failed:
	fclose(fp);
fopen_failed:
	return status;
}


static uint16_t average_red(pixel_t *a, pixel_t *b, pixel_t *c, pixel_t *d) {
	int count = 0;
	int sum = 0;
	if (NULL != a) {
		sum += (int)(a->red);
		++count;
	}
	if (NULL != b) {
		sum += (int)(b->red);
		++count;
	}
	if (NULL != c) {
		sum += (int)(c->red);
		++count;
	}
	if (NULL != d) {
		sum += (int)(d->red);
		++count;
	}

	if (0 == count) {
		return 0;
	}
	return sum / count;
}


static uint16_t average_green(pixel_t *a, pixel_t *b, pixel_t *c, pixel_t *d) {
	int count = 0;
	int sum = 0;
	if (NULL != a) {
		sum += (int)(a->green);
		++count;
	}
	if (NULL != b) {
		sum += (int)(b->green);
		++count;
	}
	if (NULL != c) {
		sum += (int)(c->green);
		++count;
	}
	if (NULL != d) {
		sum += (int)(d->green);
		++count;
	}

	if (0 == count) {
		return 0;
	}
	return sum / count;
}


static uint16_t average_blue(pixel_t *a, pixel_t *b, pixel_t *c, pixel_t *d) {
	int count = 0;
	int sum = 0;
	if (NULL != a) {
		sum += (int)(a->blue);
		++count;
	}
	if (NULL != b) {
		sum += (int)(b->blue);
		++count;
	}
	if (NULL != c) {
		sum += (int)(c->blue);
		++count;
	}
	if (NULL != d) {
		sum += (int)(d->blue);
		++count;
	}

	if (0 == count) {
		return 0;
	}
	return sum / count;
}

static void fill(bitmap_t *frame, int start_x, int start_y, int width, int height, uint16_t red, uint16_t green, uint16_t blue) {

	int end_x = start_x + width;
	int end_y = start_y + height;

	for (int y = start_y; y < end_y; ++y) {
		for (int x = start_x; x < end_x; ++x) {
			pixel_t *pixel = pixel_at(frame, x, y);
			pixel->red = red;
			pixel->green = green;
			pixel->blue = blue;
		}
	}
}

static void number(int value, bitmap_t *frame, int start_x, int start_y, int width, int height) {
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
	for (int i = 0; i < 4; ++i, divisor /= 10) {
		int d = value / divisor;
		uint16_t bm = bitmaps[d % 10];
		int x_begin = start_x + 4 * width * i;
		for (int y = 0, ys = start_y; y < 5; ++y, ys += height) {
			for (int x = 0, xs = x_begin; x < 3; ++x, xs += width) {
				if (0 != (040000 & bm)) {
					fill(frame, xs, ys, width, height, 0xff, 0xff, 0xff);
				} else {
					fill(frame, xs, ys, width, height, 0, 0, 0);
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

	bitmap_t frame;
	frame.width = 1920;
	frame.height = 1080;

	frame.pixels = calloc(sizeof(pixel_t), frame.width * frame.height);
	if (NULL == frame.pixels) {
		usage("failed to allocate pixels");
		return 0;
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
		const int bytes_per_line = 1920 * 2;
		uint8_t pixels[1080 * bytes_per_line];
		if (1 != fread(pixels, sizeof(pixels), 1, fp)) {
			rc = count;
			break;
		}

		if (verbose > 2) {

			for (int line = 0; line < 8; ++line) {
				printf("line: %2d: ", line);
				const uint8_t *p = &pixels[bytes_per_line * line];
				for (int col = 0; col < 8; ++col) {
					printf(" %02x", *p++);
				}
				printf("\n");
			}
		}

		// even rows are: GRGR...
		// odd  rows are: bgbg...
		// note the case sensitive labelling:
		//      the GREEN(G) pixels are on RED/GREEN rows
		//      the green(g) pixels are on green/blue rows
		pixel_t *pixel = frame.pixels;
		uint8_t *in = pixels;
		for (int y = 0; y < frame.height; ++y) {
			if (0 == (y & 1)) {  // even: GRGR...
				for (int x = 0; x < frame.width; ++x) {
					uint16_t px = (uint16_t)(*in++);
					px |= (uint16_t)(*in++) << 8;
					if (0 == (x & 1)) {
						pixel->green = px;
					} else {
						pixel->red = px;
					}
					++pixel;
				}
			} else { // odd: bgbg...
				for (int x = 0; x < frame.width; ++x) {
					uint16_t px = (uint16_t)(*in++);
					px |= (uint16_t)(*in++) << 8;
					if (0 == (x & 1)) {
						pixel->blue = px;
					} else {
						pixel->green = px;
					}
					++pixel;
				}
			}
		}

		if (options.compensation) {
			int hm1 = frame.height - 1;
			int wm1 = frame.width - 1;
			for (int y = 0; y < frame.height; ++y) {
				for (int x = 0; x < frame.width; ++x) {
					pixel_t *sw = (0   == y) || (0   == x) ? NULL : pixel_at(&frame, x - 1, y - 1);
					pixel_t *s  = (0   == y)               ? NULL : pixel_at(&frame, x,     y - 1);
					pixel_t *se = (0   == y) || (wm1 == x) ? NULL : pixel_at(&frame, x + 1, y - 1);
					pixel_t *nw = (hm1 == y) || (0   == x) ? NULL : pixel_at(&frame, x - 1, y + 1);
					pixel_t *n  = (hm1 == y)               ? NULL : pixel_at(&frame, x,     y + 1);
					pixel_t *ne = (hm1 == y) || (wm1 == x) ? NULL : pixel_at(&frame, x + 1, y + 1);
					pixel_t *w  = (0   == x)               ? NULL : pixel_at(&frame, x - 1, y    );
					pixel_t *e  = (wm1 == x)               ? NULL : pixel_at(&frame, x + 1, y    );

					pixel_t *pixel = pixel_at(&frame, x, y);

					if (0 == (y & 1)) { // even line: GRGR...
						if (0 == (x & 1)) {
							pixel->blue = average_blue(n, s, NULL, NULL);
							pixel->red  = average_red(w, e, NULL, NULL);
						} else {
							pixel->green = average_green(n, s, e, w);
							pixel->blue  = average_blue(nw, ne, sw, se);
						}
					} else {             // odd line: bgbg...
						if (0 == (x & 1)) {
							pixel->green = average_green(n, s, e, w);
							pixel->red   = average_red(nw, ne, sw, se);
						} else {
							pixel->blue = average_blue(w, e, NULL, NULL);
							pixel->red  = average_red(n, s, NULL, NULL);
						}
					}
				}
			}
		}

		if (verbose > 0) {
			printf("creating: %s\n", output_name);
		}

		if (options.slider) {
			fill(&frame, count + 0, 10, 20, 10, 0x00, 0x00, 0x00);
			fill(&frame, count + 5, 10, 10, 10, 0xff, 0xff, 0xff);
		}

		if(options.number) {
			number(count, &frame, 10, 30, 4, 4);
		}
		save_png_to_file(&frame, output_name);
	}

	fclose(fp);
	free(frame.pixels);
	return rc;
}
