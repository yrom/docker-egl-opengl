#include <libpng/png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef NDEBUG
#define log(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define log(fmt, ...)
#endif

static void write_linear_rgba_png(png_const_charp __restrict file,
                                  png_bytep __restrict rgba_data,
                                  png_uint_32 width, png_uint_32 height) {
  // printf("Start writing output to %s\n", file);
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  #ifndef NDEBUG
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  #endif
  FILE *fp = fopen(file, "wb");
  if (!fp) {
    printf("Failed to open file %s for writing\n", file);
    goto error;
  }

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    printf("Failed to create PNG write struct\n");
    goto error;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    printf("Failed to create PNG info struct\n");
    goto error;
  }

  png_init_io(png_ptr, fp);
  png_set_compression_level(png_ptr, 6);
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_set_gAMA(png_ptr, info_ptr, 1.0); // Linear RGB
  png_write_info(png_ptr, info_ptr);

  png_bytep *row_pointers = calloc(height, sizeof(png_bytep));

  for (unsigned int y = 0; y < height; y++) {
    // flip the image vertically
    row_pointers[y] = (png_bytep)rgba_data + (height - 1 - y) * width * 4;
  }

  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, NULL);
  free(row_pointers);
  #ifndef NDEBUG
  clock_gettime(CLOCK_MONOTONIC, &end);
  printf("PNG (%s) write completed in %.3f seconds\n", file,
         (double)(end.tv_sec - start.tv_sec) +
             (double)(end.tv_nsec - start.tv_nsec) / 1e9);
  #endif
error:
  if (png_ptr)
    png_destroy_write_struct(&png_ptr, &info_ptr);
  if (fp)
    fclose(fp);
}

static int readFile(const char *__restrict filename, char **__restrict dst, 
                    size_t *size) {

  FILE *file = fopen(filename, "rb");
  if (!file) {
    printf("Failed to open file %s for reading\n", filename);
    return -1;
  }

  fseek(file, 0, SEEK_END);
  long fsize = ftell(file);
  fseek(file, 0, SEEK_SET);

  *dst = malloc(fsize + 1);
  *size = fsize;
  if (!*dst) {
    printf("Memory allocation failed\n");
    fclose(file);
    return -1;
  }

  fread((void *)*dst, fsize, 1, file);
  (*dst)[fsize] = '\0'; // Null-terminate the string
  fclose(file);
  return 0;
}
