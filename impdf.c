#include "impdf_common.h"

#include "include/fpdfview.h"
#include "include/fpdf_edit.h"
#include "include/fpdf_text.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"


/* variables */
int pdf_page_count = 0;

/* cmd args */
int arg_jpg_quality = 85;
int arg_pdf_start_page_idx = 0;
int arg_pdf_end_page_idx = 0;
int arg_render_width = 0;
int arg_render_height = 0;
int arg_render_background_color = 0xffffffff;
int arg_render_grayscale = 0;
int arg_render_invert = 0;
char* arg_pdf_password = NULL;
int arg_pdf_rotate = 0;
char* arg_filename_format = NULL;
char* arg_filename_ext = NULL;
int arg_pdf_shift_page_idx = 0;
int arg_pdf_text_out = 0;

/* fn declarations */
void hexcolor_touppper(char* hx);
void bgrx2rgbx(uint8_t* buf, int w, int h, int comp);
void get_numbered_filename(char* out_name, int num);
void write_jpg(int num, int w, int h, int comp, void* data);
void write_png(int num, int w, int h, int comp, int stride, void* data);
void write_bmp(int num, int w, int h, int comp, void* data);
int fpdfbitmap_format2comp(int format);
void pdf_render_page_to_bmp(FPDF_PAGE page, int w, int h, int idx);
void pdf_extract_image_page(FPDF_PAGE page, int idx);
int pdf_has_text_in_page(FPDF_PAGE page);
void pdf_extract_text_page(FPDF_PAGE page, FILE* fp);
int pdf_do_page_range(FPDF_DOCUMENT doc, int start, int end);
void args_help(const char* prog_name);
void args_free();
int args_in(int argc, char* argv[]);

/* fn implementation */

void hexcolor_touppper(char* hx) {
  for (int i = 0; i < 6; ++i) {
     if (hx[i] >= 'a' && hx[i] <= 'f') {
       hx[i] = hx[i] - 'a' + 'A';
     }
  }
}

void bgrx2rgbx(uint8_t* buf, int w, int h, int comp) {
  uint8_t temp;
  int image_size = w * h * comp;

  if (arg_render_invert) {
    for (int i = 0; i < image_size; i+= comp) {
      temp = 255 - buf[i];
      buf[i] = 255 - buf[i+2];
      buf[i+2] = temp;
      buf[i+1] = 255 - buf[i+1];
      buf[i+3] = 0xff;
    }
    return;
  }

  for (int i = 0; i < image_size; i+= comp) {
    temp = buf[i];
    buf[i] = buf[i+2];
    buf[i+2] = temp;
    buf[i+3] = 0xff;
  }
}

/* int calc_padzero_width(int num) { */
  /* int i = 0; */
  /* while ((num /= 10) != 0) i++; */
  /* return i; */
/* } */

void get_numbered_filename(char* out_name, int num) {
  char fmt_str[100];
  sprintf(fmt_str, IMPDF_DIR PATH_DELIM "%s.%s", arg_filename_format, arg_filename_ext);
  sprintf(out_name, fmt_str, num + arg_pdf_shift_page_idx);
}

void write_jpg(int num, int w, int h, int comp, void* data) {
  bgrx2rgbx((uint8_t*)data, w, h, comp);
  char filename[200];
  get_numbered_filename(filename, num);
  if (!stbi_write_jpg(filename, w, h, comp, data, arg_jpg_quality)) {
    printf("write jpg failed: %s\n", filename);
  }
}

void write_png(int num, int w, int h, int comp, int stride, void* data) {
  bgrx2rgbx((uint8_t*)data, w, h, comp);
  char filename[200];
  get_numbered_filename(filename, num);
  if (!stbi_write_png(filename, w, h, comp, data, stride)) {
    printf("write png failed: %s\n", filename);
  }
}

void write_bmp(int num, int w, int h, int comp, void* data) {
  bgrx2rgbx((uint8_t*)data, w, h, comp);
  char filename[200];
  get_numbered_filename(filename, num);
  if(!stbi_write_bmp(filename, w, h, comp, data)) {
    printf("write bmp failed: %s\n", filename);
  }
}

int pdf_has_text_in_page(FPDF_PAGE page) {
  FPDF_PAGEOBJECT obj;
  int obj_cnt = FPDFPage_CountObject(page);
  for (int i = 0; i < obj_cnt; ++i) {
    obj = FPDFPage_GetObject(page, i);
    int type = FPDFPageObj_GetType(obj);
    if (type == FPDF_PAGEOBJ_TEXT) return 1;
  }
  return 0;
}

void pdf_extract_text_page(FPDF_PAGE page, FILE* fp) {
  FPDF_TEXTPAGE text_page = FPDFText_LoadPage(page);
  int total_chars = FPDFText_CountChars(text_page) + 1;
  int nbits = total_chars*sizeof(unsigned int);
  void* buffer = malloc(nbits);
  int n =  FPDFText_GetText(text_page, 0, total_chars, buffer);
  if (n != 0) {
    /* NOTICE:
     * n including the trailing terminator.
     * output file encoding with UTF-16LE */
    fwrite(buffer, (n-1)*sizeof(wchar_t), 1, fp);
    printf("extract %d unicode chars\n", n);
  }
  free(buffer);
  FPDFText_ClosePage(text_page);
}

int fpdfbitmap_format2comp(int format) {
  switch (format) {
    case FPDFBitmap_Unknown: return 0;
    case FPDFBitmap_Gray: return 1;
    case FPDFBitmap_BGR: return 3;
    case FPDFBitmap_BGRx: return 4;
    case FPDFBitmap_BGRA: return 4;
  }
  return 0;
}

static const char* fpdfbitmap_format2str[] = {
    "Unknown", "Gray", "BGR", "BGRx", "BGRA",
};

void pdf_render_page_to_bmp(FPDF_PAGE page, int w, int h, int idx) {
  /* use predefined minimal width */
  if (w == 0 || h == 0) {
    int min_width = 640;
    // get the pt units
    w = FPDF_GetPageWidth(page); 
    h = FPDF_GetPageHeight(page);
    // keep page's aspect ratio
    double page_ratio = h * 1.0 / w * 1.0;
    w = min_width;
    h = page_ratio * min_width;
  }
  if (arg_render_width != 0 && arg_render_height != 0) {
    w = arg_render_width;
    h = arg_render_height;
    printf("page resized to (WxH): %dx%d\n", w, h);
  }
  else if (arg_render_width != 0 && arg_render_height == 0) {
    double page_ratio = h * 1.0 / w * 1.0;
    w = arg_render_width;
    h = page_ratio * arg_render_width;
    printf("page resized to (WxH): %dx%d\n", w, h);
  }
  else if (arg_render_width == 0 && arg_render_height != 0) {
    double page_ratio = w * 1.0 / h * 1.0;
    h = arg_render_height;
    w = page_ratio * arg_render_height;
    printf("page resized to (WxH): %dx%d\n", w, h);
  }
  int alpha = FPDFPage_HasTransparency(page) ? 1 : 0;
  FPDF_BITMAP bmp = FPDFBitmap_Create(w, h, alpha);

  // TODO: fill 32bit ARGB color, if colorspace is unknown
  FPDFBitmap_FillRect(bmp, 0, 0, w, h, arg_render_background_color);
  int flags = FPDF_ANNOT;
  if (arg_render_grayscale) flags |= FPDF_GRAYSCALE;
  FPDF_RenderPageBitmap(bmp, page, 0, 0, w, h, arg_pdf_rotate, flags);

  int stride = FPDFBitmap_GetStride(bmp);
  int format = FPDFBitmap_GetFormat(bmp);
  void* buffer = FPDFBitmap_GetBuffer(bmp);
  printf("get bitmap: format=%s, stride=%d\n",
      fpdfbitmap_format2str[format], stride);
  int comp = fpdfbitmap_format2comp(format);

  if (!strcmp("jpg", arg_filename_ext)) {
    write_jpg(idx, w, h, comp, buffer);
  }
  if (!strcmp("png", arg_filename_ext)) {
    write_png(idx, w, h, comp, stride, buffer);
  }
  if (!strcmp("bmp", arg_filename_ext)) {
    write_bmp(idx, w, h, comp, buffer);
  }
  FPDFBitmap_Destroy(bmp);
}



// See include/fpdf_edit.h line: 24
static const char* colorspace2str[] = {
  "unknown",
  "dev_gray",
  "dev_rgb",
  "dev_cmyk",
  "cal_gray",
  "cal_rgb",
  "lab",
  "iccbased",
  "separation",
  "devicen",
  "indexed",
  "pattern",
};

void pdf_extract_image_page(FPDF_PAGE page, int idx) {
  int image_rendered_flag = 0;
  int obj_cnt = FPDFPage_CountObject(page);
  /* scanned document only have one image object
   * try get the original image size if possible. */

  FPDF_PAGEOBJECT obj;
  if (obj_cnt == 1) {
     obj = FPDFPage_GetObject(page, 0);
     int type = FPDFPageObj_GetType(obj);
     if (type == FPDF_PAGEOBJ_IMAGE) {
       FPDF_IMAGEOBJ_METADATA metadata;
       FPDFImageObj_GetImageMetadata(obj, page, &metadata);
       int w = metadata.width;
       int h = metadata.height;
       int bpp = metadata.bits_per_pixel;
       int colorspace = metadata.colorspace;
       printf("[image metatadat: %dx%d,%dbits,%s] ", w, h, bpp, colorspace2str[colorspace]);
       pdf_render_page_to_bmp(page, w, h, idx);
       image_rendered_flag = 1;
     }
  }
  /* other document render with 640 width by default */
  if (!image_rendered_flag) {
    pdf_render_page_to_bmp(page, 0, 0, idx);
  }
  FPDF_ClosePage(page);
}

int pdf_do_page_range(FPDF_DOCUMENT doc, int start, int end) {
  if (start < 0 || start >= pdf_page_count || end < 0 || end >= pdf_page_count) {
    puts("error range!");
    return 1;
  }

  if (arg_pdf_text_out) {
    FILE* fp = fopen(IMPDF_DIR PATH_DELIM "a.txt","wb");
    for (int i = start; i <= end; ++i) {
      printf("P%d / ", i);
      FPDF_PAGE page = FPDF_LoadPage(doc, i);
      if (pdf_has_text_in_page(page)) {
          pdf_extract_text_page(page, fp);
      }
    }
    fclose(fp);
    return 0;
  }

  for (int i = start; i <= end; ++i) {
    printf("P%d / ", i);
    FPDF_PAGE page = FPDF_LoadPage(doc, i);
    pdf_extract_image_page(page, i);
  }

  return 0;
}

void args_help(const char* prog_name) {
  printf(
      "Description:\n"
      "\t 1. convert PDF file to images.\n"
      "\t 2. extract texts from PDF file (if possible).\n"
      "Usage:\n"
      "%s <input_file.pdf> [options...]\n"
      "\t-q <85>        jpg quality 0~100\n"
      "\t-p <0,0>       define start,end page index (1st page = 0)\n"
      "\t-w <0>         px width to render\n"
      "\t-h <0>         px height to render\n"
      "\t-b <FFFFFF>    tint background color (RRGGBB)\n"
      "\t--grayscale    render grayscale\n"
      "\t--invert       render invert color\n"
      "\t-P <*>         pdf password\n"
      "\t-R <0>         pdf rotate degree\n"
      "\t               (0=0deg,1=90deg,2=180deg,3=270deg)\n"
      "\t-F <%%04d>      output file format\n"
      "\t-e <jpg>       output file extension (jpg,png,bmp)\n"
      "\t-s <0>         output file shift index, add to page index\n"
      "\t               (only affect output file number)\n"
      "\t--text         extract texts and append to 'a.txt'\n"
      , prog_name);
}

void args_free() {
  if (arg_filename_format != NULL)
    free(arg_filename_format);
  if (arg_filename_ext != NULL)
    free(arg_filename_ext);
  if (arg_pdf_password != NULL)
    free(arg_pdf_password);
}


int args_in(int argc, char* argv[]) {
  if (argc == 1) {
    args_help(argv[0]);
    return 1;
  }

  arg_filename_format = strdup("%04d");
  arg_filename_ext = strdup("jpg");

  for (int i = 0; i < argc; ++i) {
    if (ARG_EXIST_PARAM("-q")) {
      arg_jpg_quality = atoi(argv[i+1]);
      printf("[ARGS_IN] jpg quality:%d\n", arg_jpg_quality);
    }
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
    if (ARG_EXIST_PARAM("-b")) {
      int len = strlen(argv[i+1]);
      if (len < 6) continue;
      hexcolor_touppper(argv[i+1]);
      int rgb = 0x000000;
      sscanf(argv[i+1], "%06X", &rgb);
      arg_render_background_color = rgb + 0xff000000;
      printf("[ARGS_IN] render background color: 0x%08X\n", arg_render_background_color);
    }
    if (ARG_EXIST("--grayscale")) {
      arg_render_grayscale = 1;
      printf("[ARGS_IN] render grayscale on\n");
    }
    if (ARG_EXIST("--invert")) {
      arg_render_invert = 1;
      printf("[ARGS_IN] render invert color on\n");
    }
    if (ARG_EXIST_PARAM("-P")) {
      arg_pdf_password = strdup(argv[i+1]);
      printf("[ARGS_IN] use password %s\n", arg_pdf_password);
    }
    if (ARG_EXIST_PARAM("-R")) {
      arg_pdf_rotate = atoi(argv[i+1]);
      printf("[ARGS_IN] pdf rotate %d\n", arg_pdf_rotate);
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
    if (ARG_EXIST_PARAM("-s")) {
      arg_pdf_shift_page_idx = atoi(argv[i+1]);
      printf("[ARGS_IN] page shift %d\n", arg_pdf_shift_page_idx);
    }
    if (ARG_EXIST("--text")) {
      arg_pdf_text_out = 1;
      printf("[ARGS_IN] text out on\n");

    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (args_in(argc, argv)) {
    args_free();
    return 0;
  }

  IMPDF_DIR_CREATE;

  FPDF_STRING test_doc = "test.pdf";
  if (argc > 1) test_doc = argv[1];

  FPDF_InitLibrary();

  FPDF_DOCUMENT doc = FPDF_LoadDocument(test_doc, arg_pdf_password);
  if (!doc) {
    puts("FPDF_LoadDocument failed!");
    goto exit;
  }
  printf("load pdf doc: %s\n", test_doc);

  pdf_page_count = FPDF_GetPageCount(doc);
  printf("pdf page count: %d\n", pdf_page_count);

  pdf_do_page_range(doc, arg_pdf_start_page_idx, arg_pdf_end_page_idx);

  FPDF_CloseDocument(doc);

exit:
  args_free();
  FPDF_DestroyLibrary();

  return 0;
}
