#include "thumbnailcache.h"
#include <QFile>
#include <QPainter>

ThumbnailCache::ThumbnailCache() {
    cacheDirPath = settings->thumbnailCacheDir();
    removeLegacyCache();
}

// older versions cached thumbnails as lossless PNG; those are never read now
void ThumbnailCache::removeLegacyCache() {
    QMutexLocker lock(&mutex);
    QDir dir(cacheDirPath);
    dir.setNameFilters(QStringList("*.png"));
    dir.setFilter(QDir::Files);
    const auto legacyFiles = dir.entryInfoList();
    for(const QFileInfo &entry : legacyFiles)
        QFile::remove(entry.absoluteFilePath());
}

QString ThumbnailCache::thumbnailPath(QString id) {
    return QString(cacheDirPath + id + ".jpg");
}

bool ThumbnailCache::exists(QString id) {
    QString filePath = thumbnailPath(id);
    QFileInfo file(filePath);
    return file.exists() && file.isReadable();
}

void ThumbnailCache::saveThumbnail(QImage *image, QString id) {
    if(!image || image->isNull())
        return;
    QImage toSave;
    if(image->hasAlphaChannel()) {
        toSave = QImage(image->size(), QImage::Format_RGB32);
        toSave.fill(Qt::white);
        QPainter painter(&toSave);
        painter.drawImage(0, 0, *image);
    } else {
        toSave = *image;
    }
    // JPEG keeps only the "Description" text field; pack metadata there so
    // the cache-hit path in generate() can restore it
    toSave.setText("Description",
                   image->text("lastModified") + "\n" +
                   image->text("originalWidth") + "\n" +
                   image->text("originalHeight") + "\n" +
                   image->text("label"));
    QMutexLocker lock(&mutex);
    toSave.save(thumbnailPath(id), "JPG", 85);
}

QImage *ThumbnailCache::readThumbnail(QString id) {
    QMutexLocker lock(&mutex);
    QString filePath = thumbnailPath(id);
    QFileInfo file(filePath);
    if(!file.exists() || !file.isReadable())
        return nullptr;
    QImage *thumb = new QImage();
    if(!thumb->load(filePath)) {
        delete thumb;
        return nullptr;
    }
    QString description = thumb->text("Description");
    if(!description.isEmpty()) {
        static const char *fields[] = { "lastModified", "originalWidth", "originalHeight", "label" };
        const QStringList values = description.split('\n');
        for(int i = 0; i < 4 && i < values.count(); i++)
            thumb->setText(fields[i], values.at(i));
    }
    return thumb;
}
