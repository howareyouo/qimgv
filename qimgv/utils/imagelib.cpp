#include "imagelib.h"
#include "fileoperations.h"
#include <QFile>

void ImageLib::recolor(QPixmap &pixmap, QColor color) {
    QPainter p(&pixmap);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.setBrush(color);
    p.setPen(color);
    p.drawRect(pixmap.rect());
}

QImage *ImageLib::rotatedRaw(const QImage *src, int grad) {
    if(!src)
        return new QImage();
    QImage *img = new QImage();
    QTransform transform;
    transform.rotate(grad);
    // rotation by a multiple of 90 degrees maps pixels 1:1, interpolation is a
    // waste of time there (and gives the exact same result)
    *img = src->transformed(transform, (grad % 90) ? Qt::SmoothTransformation : Qt::FastTransformation);
    return img;
}
//------------------------------------------------------------------------------
QImage *ImageLib::rotated(std::shared_ptr<const QImage> src, int grad) {
    return rotatedRaw(src.get(), grad);
}
//------------------------------------------------------------------------------
QImage* ImageLib::croppedRaw(const QImage *src, QRect newRect) {
    if(src && src->rect().contains(newRect, false)) {
        QImage *img = new QImage(newRect.size(), src->format());
        *img = src->copy(newRect);
        return img;
    } else {
        return new QImage();
    }
}
//------------------------------------------------------------------------------
QImage* ImageLib::cropped(std::shared_ptr<const QImage> src, QRect newRect) {
    return croppedRaw(src.get(), newRect);
}
//------------------------------------------------------------------------------
QImage* ImageLib::flippedHRaw(const QImage *src) {
    if(!src)
        return new QImage();
    else
        return new QImage(src->mirrored(true, false));
}
//------------------------------------------------------------------------------
QImage* ImageLib::flippedH(std::shared_ptr<const QImage> src) {
    return flippedHRaw(src.get());
}
//------------------------------------------------------------------------------
QImage* ImageLib::flippedVRaw(const QImage *src) {
    if(!src)
        return new QImage();
    else
        return new QImage(src->mirrored(false, true));
}
//------------------------------------------------------------------------------
QImage* ImageLib::flippedV(std::shared_ptr<const QImage> src) {
    return flippedVRaw(src.get());
}
//------------------------------------------------------------------------------
std::unique_ptr<const QImage> ImageLib::exifRotated(std::unique_ptr<const QImage> src, int orientation) {
    switch(orientation) {
    case 1: {
        src.reset(ImageLib::flippedHRaw(src.get()));
    } break;
    case 2: {
        src.reset(ImageLib::flippedVRaw(src.get()));
    } break;
    case 3: {
        // flipH + flipV is a 180 degree rotation, do it in a single pass
        src.reset(new QImage(src->mirrored(true, true)));
    } break;
    case 4: {
        src.reset(ImageLib::rotatedRaw(src.get(), 90));
    } break;
    case 5: {
        src.reset(ImageLib::flippedHRaw(src.get()));
        src.reset(ImageLib::rotatedRaw(src.get(), 90));
    } break;
    case 6: {
        src.reset(ImageLib::flippedVRaw(src.get()));
        src.reset(ImageLib::rotatedRaw(src.get(), 90));
    } break;
    case 7: {
        src.reset(ImageLib::rotatedRaw(src.get(), -90));
    } break;
    default: {
    } break;
    }
    return src;
}
//------------------------------------------------------------------------------
std::unique_ptr<QImage> ImageLib::exifRotated(std::unique_ptr<QImage> src, int orientation) {
    switch(orientation) {
    case 1: {
        src.reset(ImageLib::flippedHRaw(src.get()));
    } break;
    case 2: {
        src.reset(ImageLib::flippedVRaw(src.get()));
    } break;
    case 3: {
        // flipH + flipV is a 180 degree rotation, do it in a single pass
        src.reset(new QImage(src->mirrored(true, true)));
    } break;
    case 4: {
        src.reset(ImageLib::rotatedRaw(src.get(), 90));
    } break;
    case 5: {
        src.reset(ImageLib::flippedHRaw(src.get()));
        src.reset(ImageLib::rotatedRaw(src.get(), 90));
    } break;
    case 6: {
        src.reset(ImageLib::flippedVRaw(src.get()));
        src.reset(ImageLib::rotatedRaw(src.get(), 90));
    } break;
    case 7: {
        src.reset(ImageLib::rotatedRaw(src.get(), -90));
    } break;
    default: {
    } break;
    }
    return src;
}
//------------------------------------------------------------------------------
/*

QImage *ImageLib::cropped(QRect newRect, QRect targetRes, bool upscaled) {
    QImage *cropped = new QImage(targetRes.size(), image->format());
    if(upscaled) {
        QImage temp = image->copy(newRect);
        *cropped = temp.scaled(targetRes.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QRect target(QPoint(0, 0), targetRes.size());
        target.moveCenter(cropped->rect().center());
        *cropped = cropped->copy(target);
    } else {
        newRect.moveCenter(image->rect().center());
        *cropped = image->copy(newRect);
    }
    return cropped;
}
*/

QImage* ImageLib::scaled(std::shared_ptr<const QImage> source, QSize destSize, ScalingFilter filter) {
    if(!source)
        return new QImage();
    auto scaleTarget = source;
    if(source->format() == QImage::Format_Indexed8) {
        auto newFmt = QImage::Format_RGB32;
        if(source->hasAlphaChannel())
            newFmt = QImage::Format_ARGB32;
        scaleTarget.reset(new QImage(source->convertToFormat(newFmt)));
    }
#ifdef USE_OPENCV
    if(filter > 1 && !QtOcv::isSupported(scaleTarget->format()))
        filter = QI_FILTER_BILINEAR;
#endif
    switch (filter) {
        case QI_FILTER_NEAREST:
            return scaled_Qt(scaleTarget, destSize, false);
        case QI_FILTER_BILINEAR:
            return scaled_Qt(scaleTarget, destSize, true);
#ifdef USE_OPENCV
        case QI_FILTER_CV_BILINEAR_SHARPEN:
            return scaled_CV(scaleTarget, destSize, cv::INTER_LINEAR, 0);
        case QI_FILTER_CV_CUBIC:
            return scaled_CV(scaleTarget, destSize, cv::INTER_CUBIC, 0);
        case QI_FILTER_CV_CUBIC_SHARPEN:
            return scaled_CV(scaleTarget, destSize, cv::INTER_CUBIC, 1);
#endif
        default:
            return scaled_Qt(scaleTarget, destSize, true);
    }
}

QImage* ImageLib::scaled_Qt(std::shared_ptr<const QImage> source, QSize destSize, bool smooth) {
    if(!source)
        return new QImage();
    QImage *dest = new QImage();
    Qt::TransformationMode mode = smooth ? Qt::SmoothTransformation : Qt::FastTransformation;
    *dest = source->scaled(destSize.width(), destSize.height(), Qt::IgnoreAspectRatio, mode);
    return dest;
}

#ifdef USE_OPENCV
// this probably leaks, needs checking
QImage* ImageLib::scaled_CV(std::shared_ptr<const QImage> source, QSize destSize, cv::InterpolationFlags filter, int sharpen) {
    if(!source)
        return new QImage();
    QtOcv::MatColorOrder order;
    cv::Mat srcMat = QtOcv::image2Mat_shared(*source.get(), &order);
    cv::Size destSizeCv(destSize.width(), destSize.height());
    QImage *dest = new QImage();
    if(destSize == source->size()) {
        // TODO: should this return a copy?
        //result.reset(new StaticImageContainer(std::make_shared<cv::Mat>(srcMat)));
    } else if(destSize.width() > source.get()->width()) { // upscale
        cv::Mat dstMat(destSizeCv, srcMat.type());
        cv::resize(srcMat, dstMat, destSizeCv, 0, 0, filter);
        *dest = QtOcv::mat2Image(dstMat, order, source->format());
    } else { // downscale
        float scale = (float)destSize.width() / source->width();
        if(scale < 0.5f && filter != cv::INTER_NEAREST) {
            if(filter == cv::INTER_CUBIC)
                sharpen = 1;
            filter = cv::INTER_AREA;
        }
        cv::Mat dstMat(destSizeCv, srcMat.type());
        cv::resize(srcMat, dstMat, destSizeCv, 0, 0, filter);
        if(!sharpen || filter == cv::INTER_NEAREST) {
            *dest = QtOcv::mat2Image(dstMat, order, source->format());
        } else {
            // todo: tweak this
            double amount = 0.25 * sharpen;
            // unsharp mask
            cv::Mat dstMat_sharpened;
            cv::GaussianBlur(dstMat, dstMat_sharpened, cv::Size(0, 0), 2);
            cv::addWeighted(dstMat, 1.0 + amount, dstMat_sharpened, -amount, 0, dstMat_sharpened);
            *dest = QtOcv::mat2Image(dstMat_sharpened, order, source->format());
        }
    }
    //qDebug() << "Filter:" << filter << " sharpen=" << sharpen << " source size:" << source->size() << "->" << (float)destSize.width() / source->width() << ": " << t.elapsed() << " ms.";
    return dest;
}
#endif
//------------------------------------------------------------------------------
bool ImageLib::saveImageWithBackup(const QImage &src, const QString &destPath) {
    QString tmpPath = destPath + "_" + FileOperations::generateHash(destPath);
    QFileInfo fi(destPath);
    QString ext = fi.suffix();
    // png compression note from libpng
    // Note that tests have shown that zlib compression levels 3-6 usually perform as well
    // as level 9 for PNG images, and do considerably fewer caclulations
    int quality = 95;
    if(ext.compare("png", Qt::CaseInsensitive) == 0)
        quality = 30;
    else if(ext.compare("jpg", Qt::CaseInsensitive) == 0 || ext.compare("jpeg", Qt::CaseInsensitive) == 0)
        quality = settings->JPEGSaveQuality();

    bool backupExists = false, success = false, originalExists = false;

    if(QFile::exists(destPath))
        originalExists = true;

    // backup the original file if possible
    if(originalExists) {
        QFile::remove(tmpPath);
        if(!QFile::copy(destPath, tmpPath)) {
            qDebug() << "ImageLib::saveImageWithBackup() - Could not create file backup.";
            return false;
        }
        backupExists = true;
    }
    // save file
    success = src.save(destPath, ext.toStdString().c_str(), quality);

    if(backupExists) {
        if(success) {
            // everything ok - remove the backup
            QFile file(tmpPath);
            file.remove();
        } else if(originalExists) {
            // revert on fail
            QFile::remove(destPath);
            QFile::copy(tmpPath, destPath);
            QFile::remove(tmpPath);
        }
    }
    return success;
}
