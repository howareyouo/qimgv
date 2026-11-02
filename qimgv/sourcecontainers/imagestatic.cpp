#include "imagestatic.h"
#include <time.h>

ImageStatic::ImageStatic(QString _path)
    : Image(_path)
{
    load();
}

ImageStatic::ImageStatic(std::unique_ptr<DocumentInfo> _info)
    : Image(std::move(_info))
{
    load();
}

ImageStatic::~ImageStatic() {
}

//load image data from disk
void ImageStatic::load() {
    if(isLoaded()) {
        return;
    }
    if(mDocInfo->mimeType().name() == "image/vnd.microsoft.icon")
        loadICO();
    else
        loadGeneric();
}


void ImageStatic::loadGeneric() {
    /* QImageReader::read() seems more reliable than just reading via QImage.
     * For example: "Invalid JPEG file structure: two SOF markers"
     * QImageReader::read() returns false, but still reads an image. Meanwhile QImage just fails.
     * I havent checked qimage's code, but it seems like it sees an exception
     * from libjpeg or whatever and just gives up on reading the file.
     *
     * tldr: qimage bad
     */
    // single reader for the whole decode: it applies the exif orientation
    // while reading (for jpeg this is done in the decoder itself), which
    // avoids both an extra header probe and a full-size copy of the image
    QImageReader r(mPath, mDocInfo->format().toStdString().c_str());
    r.setAutoTransform(true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    r.setAllocationLimit(settings->memoryAllocationLimit());
#endif
    QImage tmp;
    r.read(&tmp);
    if(tmp.isNull() && !mDocInfo->format().isEmpty()) {
        // the extension-based format guess can be wrong for mislabeled files,
        // so retry with content-based detection before giving up
        QImageReader fallback(mPath);
        fallback.setAutoTransform(true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        fallback.setAllocationLimit(settings->memoryAllocationLimit());
#endif
        fallback.read(&tmp);
    }
    std::unique_ptr<const QImage> img(new QImage(std::move(tmp)));
    // scaling this format via qt results in transparent background
    // it rare enough so lets just convert it to the closest working thing
    if(img->format() == QImage::Format_Mono) {
        QImage *imgConverted = new QImage();
        *imgConverted = img->convertToFormat(QImage::Format_Grayscale8);
        image.reset(imgConverted);
    } else {
        // set image
        image = std::move(img);
    }
    mLoaded = true;
}

// TODO: move this out somewhere to use in other places
void ImageStatic::loadICO() {
    // Big brain code. It's mostly for small ico files so whatever. I'm not patching Qt for this.
    QIcon icon(mPath);
    QList<QSize> sizes = icon.availableSizes();
    QSize maxSize(0, 0);
    for(auto sz : sizes)
        if(maxSize.width() < sz.width())
            maxSize = sz;
    QPixmap iconPix = icon.pixmap(maxSize);
    std::unique_ptr<const QImage> img(new QImage(iconPix.toImage()));
    image = std::move(img);
    mLoaded = true;
}

// TODO: move saving to directorymodel
bool ImageStatic::save(QString destPath) {
    bool success = ImageLib::saveImageWithBackup(isEdited() ? *imageEdited : *image, destPath);
    if(isEdited()) {
        image.swap(imageEdited);
        discardEditedImage();
    }
    if(destPath == mPath && success)
        mDocInfo->refresh();
    return success;
}

bool ImageStatic::save() {
    return save(mPath);
}

std::unique_ptr<QPixmap> ImageStatic::getPixmap() {
    std::unique_ptr<QPixmap> pix(new QPixmap());
    isEdited()?pix->convertFromImage(*imageEdited):pix->convertFromImage(*image, Qt::NoFormatConversion);
    return pix;
}

std::shared_ptr<const QImage> ImageStatic::getSourceImage() {
    return image;
}

std::shared_ptr<const QImage> ImageStatic::getImage() {
    return isEdited()?imageEdited:image;
}

int ImageStatic::height() {
    return isEdited()?imageEdited->height():image->height();
}

int ImageStatic::width() {
    return isEdited()?imageEdited->width():image->width();
}

QSize ImageStatic::size() {
    return isEdited()?imageEdited->size():image->size();
}

bool ImageStatic::setEditedImage(std::unique_ptr<const QImage> imageEditedNew) {
    if(imageEditedNew && imageEditedNew->width() != 0) {
        discardEditedImage();
        imageEdited = std::move(imageEditedNew);
        mEdited = true;
        return true;
    }
    return false;
}

bool ImageStatic::discardEditedImage() {
    if(imageEdited) {
        imageEdited.reset();
        mEdited = false;
        return true;
    }
    return false;
}
