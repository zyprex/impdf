
#include "impdf_common.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#include "include/stb_image.h"

/* cmd args */
int arg_jpg_quality = 85;
int arg_start_idx = 0;
int arg_end_idx = 0;
char* arg_filename_format = NULL;
char* arg_filename_ext = NULL;
char* arg_filename_ext_out = NULL;

int check_file_exist(char* filename) {
  FILE* fp = fopen(filename, "rb");
  if (fp != NULL) {
    fclose(fp);
    return 1;
  }
  return 0;
}

int image_convert() {
  int success_count = 0;
  for (int i = arg_start_idx; i <= arg_end_idx; ++i) {
    char input_fname[200];
    char output_fname[200];
    char fmt_str[100];

    sprintf(fmt_str, IMPDF_DIR PATH_DELIM "%s.%s", arg_filename_format, arg_filename_ext);
    sprintf(input_fname, fmt_str, i);

    if (!check_file_exist(input_fname)) {
      continue;
    }

    sprintf(fmt_str, IMPDF_DIR PATH_DELIM "%s.%s", arg_filename_format, arg_filename_ext_out);
    sprintf(output_fname, fmt_str, i);

    int w, h, comp;
    void* data;
    data = stbi_load(input_fname, &w, &h, &comp, 0);

    int is_ok = 0;

    if (!strcmp("jpg", arg_filename_ext_out))
      is_ok = stbi_write_jpg(output_fname, w, h, comp, data, arg_jpg_quality);
    if (!strcmp("png", arg_filename_ext_out))
      is_ok = stbi_write_png(output_fname, w, h, comp, data, w * comp);
    if (!strcmp("bmp", arg_filename_ext_out))
      is_ok = stbi_write_bmp(output_fname, w, h, comp, data);

    success_count += is_ok;
    stbi_image_free(data);
  }

  return success_count;
}

void args_help(const char* prog_name) {
  printf(
      "Description: jpg,png,bmp image convert tool.\n"
      "Usage:\n"
      "%s [options...]\n"
      "\t-q <85>      jpg quality 0~100\n"
      "\t-p <0,0>     define start,end index (1st page = 0)\n"
      "\t-F <%%04d>    input file format\n"
      "\t-e <png>     input file extension (jpg,png,bmp)\n"
      "\t-E <jpg>     output file extension (jpg,png,bmp)\n"
      , prog_name);
}

int args_in(int argc, char* argv[]) {
  if (argc == 1) {
    args_help(argv[0]);
    return 1;
  }
  arg_filename_format = strdup("%04d");
  arg_filename_ext = strdup("png");
  arg_filename_ext_out = strdup("jpg");
  for (int i = 0; i < argc; ++i) {
    if (ARG_EXIST_PARAM("-q")) {
      arg_jpg_quality = atoi(argv[i+1]);
      printf("[ARGS_IN] jpg quality:%d\n", arg_jpg_quality);
    }
    if (ARG_EXIST_PARAM("-p")) {
      char* colon = strstr(argv[i+1], ",");
      if (colon) {
        arg_end_idx = atoi(colon+1);
        colon = '\0';
        arg_start_idx = atoi(argv[i+1]);
        printf("[ARGS_IN] start idx: %d; end idx:%d\n", arg_start_idx, arg_end_idx);
      }
    }
    if (ARG_EXIST_PARAM("-F")) {
      free(arg_filename_format);
      arg_filename_format = NULL;
      arg_filename_format = strdup(argv[i+1]);
      printf("[ARGS_IN] filename format %s\n", arg_filename_format);
    }
    if (ARG_EXIST_PARAM("-e")) {
      free(arg_filename_ext);
      arg_filename_ext = NULL;
      if (strcmp("png", argv[i+1])
          && strcmp("jpg", argv[i+1])
          && strcmp("bmp", argv[i+1])) {
        puts("[ARGS_IN] ERROR ON FILENAME EXTENSION !");
        return 1;
      }
      arg_filename_ext = strdup(argv[i+1]);
      printf("[ARGS_IN] filename extension %s\n", arg_filename_ext);
    }
    if (ARG_EXIST_PARAM("-E")) {
      free(arg_filename_ext_out);
      arg_filename_ext_out = NULL;
      if (strcmp("png", argv[i+1])
          && strcmp("jpg", argv[i+1])
          && strcmp("bmp", argv[i+1])) {
        puts("[ARGS_IN] ERROR ON FILENAME EXTENSION !");
        return 1;
      }
      arg_filename_ext_out = strdup(argv[i+1]);
      printf("[ARGS_IN] filename extension output %s\n", arg_filename_ext_out);
    }
  }
  return 0;
}

void args_free() {
  if (arg_filename_format != NULL)
    free(arg_filename_format);
  if (arg_filename_ext != NULL)
    free(arg_filename_ext);
  if (arg_filename_ext_out != NULL)
    free(arg_filename_ext_out);
}

int main(int argc, char *argv[]) {
  if (args_in(argc, argv)) {
    args_free();
    return 0;
  }
  int total = arg_end_idx - arg_start_idx + 1;
  int ok = image_convert();
  printf("convert %d, successed %d\n", total, ok);
  args_free();
  return 0;
}
