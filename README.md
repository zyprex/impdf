# impdf intro

The `impdf` is a command line tool that covert pdf to images and vice verse.
```
impdf PDF->image
impdf_merge image->PDF
impdf_image image->image
```
Possible image format are:
- jpg
- png
- bmp

And any other format like `webp`, consider use `ffmpeg`.

No advanced function support like trim blank edge, because I think it's the
reader's duty.

## How to get help

There is no `--help` options, just execute `impdf` on terminal,
and you can see help strings.

For Linux, execute `impdf_run.sh` to run `impdf`.

## Bonus

- You can extract text from PDF with `--text` options.

## Why I made impdf ?

- Sometimes I only get image file and as far as I knows, many
ebook reader or image viewer doesn't support text reflow on
images. So I have to convert them to PDF.

- Some PDF have logo or watermark, it's easier remove logo or
watermark from images than PDF.

- There have many good image viewer but comes with PDF viewer, not so many.
Another defect is that a lot of PDF editor's size is very huge (although
they have a bunch of functions). I found that many functions they provide
use a image viewer can do the same or even better. And use the online PDF
editor, the data safe is always a big problem.

# Credits

- [PDFGen](https://github.com/AndreRenaud/PDFGen)
- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)
- [stb_image_write.h](https://github.com/nothings/stb/blob/master/stb_image_write.h)
- [pdfium-reader](https://github.com/ikuokuo/pdfium-reader)
- [pdfium-binaries](https://github.com/bblanchon/pdfium-binaries)
- [c_api_reference_pdfium](https://developers.foxit.com/resources/pdf-sdk/c_api_reference_pdfium/index.html)
