#pragma once

#include <QString>
#include <QSize>
#include <QUrl>
#include <QMimeDatabase>
#include <QMutex>
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>
#include <cmath>
#include <cstring>
#include "utils/stuff.h"
#include "settings.h"

#ifdef USE_EXIV2

#include <exiv2/exiv2.hpp>
#include <iostream>
#include <iomanip>
#include <cassert>

#endif

#include <QImageReader>

enum DocumentType { NONE, STATIC, ANIMATED };

class DocumentInfo {
public:
    DocumentInfo(QString path);
    ~DocumentInfo();
    
    QString directoryPath() const;
    QString filePath() const;
    QString fileName() const;
    QString baseName() const;
    qint64 fileSize() const;
    DocumentType type() const;
    QMimeType mimeType() const;

    // file extension (guessed from mime-type)
    QString format() const;
    // note: the orientation is detected lazily, only when requested
    // (the main image path lets QImageReader apply it while decoding)
    int exifOrientation();

    QDateTime lastModified() const;
    void refresh();
    void loadExifTags();
    QMap<QString, QString> getExifTags();

private:
    QFileInfo fileInfo;
    DocumentType mDocumentType;
    int mOrientation;
    QString mFormat;
    bool exifLoaded;
    bool mOrientationLoaded;

    // guesses file type from its contents
    // and sets extension
    void detectFormat();
    void loadExifOrientation();
    bool detectAPNG();
    bool detectAnimatedWebP();
    bool detectAnimatedJxl();
    bool detectAnimatedAvif();

    QMutex exifMutex;
    void loadExifTagsImpl();
    QMap<QString, QString> exifTags;
    QMimeType mMimeType;
};
