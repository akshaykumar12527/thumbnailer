#include "string.h"
#include "thumbnailer.h"

unsigned long maxX = 150, maxY = 150, JPEGCompressionLevel = 90;

int thumbnail(const void *src, const size_t size, struct Thumbnail *thumb,
	      bool jpeg, ExceptionInfo *ex)
{
	ImageInfo *info = NULL;
	Image *img = NULL, *sampled = NULL, *scaled = NULL;
	double scale;

	// Read image
	info = CloneImageInfo(NULL);
	GetExceptionInfo(ex);
	img = BlobToImage(info, src, size, ex);
	if (img == NULL) {
		goto end;
	}

	// Image already fits thumbnail
	if (img->columns <= maxX && img->rows <= maxY) {
		thumb->width = img->columns;
		thumb->height = img->rows;
		strncpy(thumb->buf, src, size);
		thumb->size = size;
		goto end;
	}

	// Maintain aspect ratio
	if (img->columns >= img->rows) {
		scale = (double)(img->columns) / (double)(maxX);
	} else {
		scale = (double)(img->rows) / (double)(maxY);
	}
	thumb->width = (unsigned long)(img->columns / scale);
	thumb->height = (unsigned long)(img->rows / scale);

	// Subsample to twice the thumbnail size. A decent enough compromise
	// between quality and performance for images arround the thumbnail size
	// and much bigger ones.
	sampled = SampleImage(img, thumb->width * 2, thumb->height * 2, ex);
	if (sampled == NULL) {
		goto end;
	}

	// Scale to thumbnail size
	scaled = ResizeImage(sampled, thumb->width, thumb->height, CubicFilter,
			     1, ex);
	if (scaled == NULL) {
		goto end;
	}

	// Write thumbnail
	DestroyImageInfo(info);
	info = CloneImageInfo(NULL);
	if (jpeg) {
		info->quality = JPEGCompressionLevel;
		strcpy(info->magick, "JPEG");
		strcpy(scaled->magick, "JPEG");
	} else {
		// Will pass through libimagequant, so comression and filters
		// are pointeless
		info->quality = 0;
		strcpy(info->magick, "PNG");
		strcpy(scaled->magick, "PNG");
	}
	thumb->buf = ImageToBlob(info, scaled, &thumb->size, ex);

end:
	if (img != NULL) {
		DestroyImage(img);
	}
	if (info != NULL) {
		DestroyImageInfo(info);
	}
	if (sampled != NULL) {
		DestroyImage(sampled);
	}
	if (scaled != NULL) {
		DestroyImage(scaled);
	}
	return thumb->buf == NULL;
}