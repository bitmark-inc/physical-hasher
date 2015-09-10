// create-png.c

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// A coloured pixel

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} pixel_t;

// A picture

typedef struct {
	pixel_t *pixels;
	size_t width;
	size_t height;
} bitmap_t;

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

	int pixel_size = 3;
	int depth = 8;

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
			*row++ = pixel->green;
			*row++ = pixel->blue;
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


static uint8_t average_red(pixel_t *a, pixel_t *b, pixel_t *c, pixel_t *d) {
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


static uint8_t average_green(pixel_t *a, pixel_t *b, pixel_t *c, pixel_t *d) {
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


static uint8_t average_blue(pixel_t *a, pixel_t *b, pixel_t *c, pixel_t *d) {
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

static void fill(bitmap_t *frame, int start_x, int start_y, int width, int height, uint8_t red, uint8_t green, uint8_t blue) {

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


int main(int argc, char *argv[]) {
	FILE *fp = fopen("frames.data", "rb");
	if (NULL == fp) {
		fprintf(stderr, "failed to open input file\n");
		return 1;
	}

	bitmap_t frame;
	frame.width = 1920;
	frame.height = 1080;

	frame.pixels = calloc(sizeof(pixel_t), frame.width * frame.height);
	if (NULL == frame.pixels) {
		fprintf(stderr, "failed to allocate pixels\n");
		return 1;
	}

	// only up to frame 9999
	for (int count = 0; count < 9999; ++count) {
		char output_name[256];
		if (snprintf(output_name, sizeof(output_name), "frame%04d.png", count) >= sizeof(output_name)) {
			fprintf(stderr, "failed to create output name\n");
			return 1;
		}

		// extract one frame from the input file
		uint8_t pixels[1920 * 1080 * 2];
		if (1 != fread(pixels, sizeof(pixels), 1, fp)) {
			break;
		}

		// rows are GR...,bg...
		typedef enum {
			MODE_GRBG = 0,
			MODE_RGGB = 1
		} mode_t;

		mode_t mode  = MODE_GRBG;

		pixel_t *pixel = frame.pixels;
		uint8_t *in = pixels;
		for (int y = 0; y < frame.height; ++y) {
			if (mode == (y & 1)) {
				for (int x = 0; x < frame.width; ++x) {
					uint16_t px = (uint16_t)(*in++);
					px |= (uint16_t)(*in++) << 8;
					px >>= 4;
					if (px > 255) {
						px = 255;
					}
					if (mode == (x & 1)) {
						pixel->green = px;
					} else {
						pixel->red = px;
					}
					++pixel;
				}
			} else {
				for (int x = 0; x < frame.width; ++x) {
					uint16_t px = (uint16_t)(*in++);
					px |= (uint16_t)(*in++) << 8;
					px >>= 4;
					if (px > 255) {
						px = 255;
					}
					if (mode == (x & 1)) {
						pixel->blue = px;
					} else {
						pixel->green = px;
					}
					++pixel;
				}
			}
		}

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

				if (mode == (y & 1)) {
					if (mode == (x & 1)) { // green(red)
						pixel->blue = average_blue(n, s, NULL, NULL);
						pixel->red  = average_red(w, e, NULL, NULL);
					} else {               // red
						pixel->green = average_green(n, s, e, w);
						pixel->blue  = average_blue(nw, ne, sw, se);
					}
				} else {
					if (mode == (x & 1)) { // blue
						pixel->green = average_green(n, s, e, w);
						pixel->red   = average_red(nw, ne, sw, se);
					} else {               // green(blue)
						pixel->blue = average_blue(w, e, NULL, NULL);
						pixel->red  = average_red(n, s, NULL, NULL);
					}
				}
			}
		}

		printf("creating: %s\n", output_name);
		fill(&frame, count + 0, 10, 20, 10, 0x00, 0x00, 0x00);
		fill(&frame, count + 5, 10, 10, 10, 0xff, 0xff, 0xff);
		number(count, &frame, 10, 30, 4, 4);
		save_png_to_file(&frame, output_name);
	}

	frame.pixels = calloc(sizeof(pixel_t), frame.width * frame.height);
	fclose(fp);
	return 0;
}
