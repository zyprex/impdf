#include "impdf_common.h"

#include "pdfgen.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#include "include/stb_image.h"

/* variables */

/* cmd args */
int arg_pdf_start_page_idx = 0;
int arg_pdf_end_page_idx = 0;
char* arg_filename_format = NULL;
char* arg_filename_ext = NULL;
int arg_render_width = 0;
int arg_render_height = 0;
char arg_image_fit = 'h';

void get_img_wxh(char* filename, int* w, int* h);
void pdf_add_image_files(struct pdf_doc* pdf, int start, int end, int page_width, int page_height);

void get_img_wxh(char* filename, int* w, int* h) {
  /* int w, h, colorspace; */
  stbi_info(filename, w, h, NULL);
  /* printf("image:%dx%d(%d)\n", w, h, colorspace); */
}

void get_numbered_filename(char* out_name, int num) {
  char fmt_str[100];
  sprintf(fmt_str, IMPDF_DIR PATH_DELIM "%s.%s", arg_filename_format, arg_filename_ext);
  sprintf(out_name, fmt_str, num);
}

int check_file_exist(char* filename) {
  FILE* fp = fopen(filename, "rb");
  if (fp != NULL) {
    fclose(fp);
    return 1;
  }
  return 0;
}

void pdf_add_image_files(struct pdf_doc* pdf, int start, int end,
    int page_width, int page_height) {

  char filename[200];

  for (int i = start; i <= end; ++i) {
    get_numbered_filename(filename, i);
    // if filename not exist
    if(!check_file_exist(filename)) continue;
    // create new page in pdf
    pdf_append_page(pdf);
    int iw = page_width;
    int ih = page_height;
    int x = 0;
    int y = 0;

    if (arg_image_fit == 'f') {
      // do nothing
    }
    else if (arg_image_fit == 'w') {
      get_img_wxh(filename, &iw, &ih);
      double ratio = ih * 1.0 / iw * 1.0;
      // fit page width
      iw = page_width;
      // put image to v-center
      ih = ratio * ih;
      y = (page_height - ih) / 2;
    }
    /* else if (arg_image_fit == 'h') { */
    else {
      get_img_wxh(filename, &iw, &ih);
      double ratio = iw * 1.0 / ih * 1.0;
      // fit page height
      ih = page_height;
      // put image to center
      iw = ratio * ih;
      x = (page_width - iw) / 2;
    }
    if (!strcmp("jpg", arg_filename_ext)) {
      pdf_add_image_file(pdf, NULL, x, y, iw, ih, filename);
    }
    if (!strcmp("png", arg_filename_ext)) {
      /* pdf_add_image_file(pdf, NULL, x, y, iw, ih, filename); */
      int w = 0, h = 0;
      void* data =  stbi_load(filename, &w, &h, NULL, 3);
      pdf_add_rgb24(pdf, NULL, x, y, iw, ih, data, w, h);
      stbi_image_free(data);
    }
    if (!strcmp("bmp", arg_filename_ext)) {
      int w = 0, h = 0;
      void* data =  stbi_load(filename, &w, &h, NULL, 3);
      pdf_add_rgb24(pdf, NULL, x, y, iw, ih, data, w, h);
      stbi_image_free(data);
    }
    printf("added '%s' (%dx%d)\n", filename, iw, ih);
  }
}

void args_help(const char* prog_name) {
  printf(
      "Description: merge images to one PDF file (WARN: auto rewrite any exist file).\n"
      "Usage:\n"
      "%s <output_file.pdf> [options...]\n"
      "\t-p <0,0>       define start,end page index (1st page = 0)\n"
      "\t-w <0>         px width to render, default A4 width\n"
      "\t-h <0>         px height to render, default A4 height\n"
      "\t-F <%%04d>      input file format\n"
      "\t-e <jpg>       input file extension (jpg,png,bmp)\n"
      "\t-f <h>         fit image to ... h=height, w=width, f=full page\n"
      , prog_name);
}

void args_free() {
  if (arg_filename_format != NULL)
    free(arg_filename_format);
  if (arg_filename_ext != NULL)
    free(arg_filename_ext);
}


int args_in(int argc, char* argv[]) {
  if (argc == 1) {
    args_help(argv[0]);
    return 1;
  }

  arg_filename_format = strdup("%04d");
  arg_filename_ext = strdup("jpg");

  for (int i = 0; i < argc; ++i) {
    if (ARG_EXIST_PARAM("-p")) {
      char* colon = strstr(argv[i+1], ",");
      if (colon) {
        arg_pdf_end_page_idx = atoi(colon+1);
        colon = '\0';
        arg_pdf_start_page_idx = atoi(argv[i+1]);
        printf("[ARGS_IN] start page: %d; end page:%d\n", arg_pdf_start_page_idx, arg_pdf_end_page_idx);
      }
    }
    if (ARG_EXIST_PARAM("-w")) {
      arg_render_width = atoi(argv[i+1]);
      printf("[ARGS_IN] render width: %d\n", arg_render_width);
    }
    if (ARG_EXIST_PARAM("-h")) {
      arg_render_height = atoi(argv[i+1]);
      printf("[ARGS_IN] render height: %d\n", arg_render_height);
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
    if (ARG_EXIST_PARAM("-f")) {
      arg_image_fit = argv[i+1][0];
      printf("[ARGS_IN] image fit %c\n", arg_image_fit);
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (args_in(argc, argv)) {
    args_free();
    return 0;
  }

  if (access(IMPDF_DIR,0) != 0) {
    puts("error: the directory '" IMPDF_DIR "' not found!");
    return 0;
  }

  int w = arg_render_width ? arg_render_width : PDF_A4_WIDTH;
  int h = arg_render_height ? arg_render_height : PDF_A4_HEIGHT;
  struct pdf_doc *pdf = pdf_create(w, h, NULL);

  pdf_add_image_files(pdf, arg_pdf_start_page_idx, arg_pdf_end_page_idx, w, h);

  pdf_save(pdf, argv[1]);
  pdf_destroy(pdf);

  if (arg_filename_format != NULL)
    free(arg_filename_format);
  return 0;
}
