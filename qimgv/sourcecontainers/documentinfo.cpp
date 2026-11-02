#include "documentinfo.h"
#include <QFile>

namespace {

// reads a small slice of the file header; used for cheap animated-format
// detection without a full QImageReader/mime lookup
QByteArray readHeader(const QString &path, int offset, int length) {
    QFile f(path);
    if(!f.open(QFile::ReadOnly))
        return QByteArray();
    if(offset > 0 && !f.seek(offset))
        return QByteArray();
    return f.read(length);
}

} // namespace

DocumentInfo::DocumentInfo(QString path)
    : mDocumentType(DocumentType::NONE),
      mOrientation(0),
      mFormat(""),
      exifLoaded(false),
      mOrientationLoaded(false)
{
    fileInfo.setFile(path);
    if(!fileInfo.isFile()) {
        qDebug() << "FileInfo: cannot open: " << path;
        return;
    }
    detectFormat();
}

DocumentInfo::~DocumentInfo() {
}

// ##############################################################
// ####################### PUBLIC METHODS #######################
// ##############################################################

QString DocumentInfo::directoryPath() const {
    return fileInfo.absolutePath();
}

QString DocumentInfo::filePath() const {
    return fileInfo.absoluteFilePath();
}

QString DocumentInfo::fileName() const {
    return fileInfo.fileName();
}

QString DocumentInfo::baseName() const {
    return fileInfo.baseName();
}

// bytes
qint64 DocumentInfo::fileSize() const {
    return fileInfo.size();
}

DocumentType DocumentInfo::type() const {
    return mDocumentType;
}

QMimeType DocumentInfo::mimeType() const {
    return mMimeType;
}

QString DocumentInfo::format() const {
    return mFormat;
}

QDateTime DocumentInfo::lastModified() const {
    return fileInfo.lastModified();
}

// For cases like orientation / even mimetype change we just reload
// Image from scratch, so don`t bother handling it here
void DocumentInfo::refresh() {
    fileInfo.refresh();
}

int DocumentInfo::exifOrientation() {
    if(!mOrientationLoaded) {
        mOrientationLoaded = true;
        loadExifOrientation();
    }
    return mOrientation;
}

// ##############################################################
// ####################### PRIVATE METHODS ######################
// ##############################################################
void DocumentInfo::detectFormat() {
    if(mDocumentType != DocumentType::NONE)
        return;

    // one instance per thread: QMimeDatabase is reentrant, not thread-safe,
    // and document info is also created by the thumbnail worker threads
    thread_local QMimeDatabase mimeDb;
    const QByteArray suffix = fileInfo.suffix().toLower().toUtf8();

    // Looking the type up by extension is free, while MatchContent opens the
    // file and reads its header. Only sniff the contents when the extension is
    // unknown or gives no usable type.
    mMimeType = mimeDb.mimeTypeForFile(fileInfo, QMimeDatabase::MatchExtension);
    if(!mMimeType.isValid() || mMimeType.name() == "application/octet-stream")
        mMimeType = mimeDb.mimeTypeForFile(fileInfo.filePath(), QMimeDatabase::MatchContent);

    auto mimeName = mMimeType.name().toUtf8();
    if(mimeName == "image/jpeg") {
        mFormat = "jpg";
        mDocumentType = DocumentType::STATIC;
    } else if(mimeName == "image/png" || mimeName == "image/apng") {
        if(settings->supportedFormats().contains("apng") && detectAPNG()) {
            mFormat = "apng";
            mDocumentType = DocumentType::ANIMATED;
        } else {
            mFormat = "png";
            mDocumentType = DocumentType::STATIC;
        }
    } else if(mimeName == "image/gif") {
        mFormat = "gif";
        mDocumentType = DocumentType::ANIMATED;
    } else if(mimeName == "image/webp" || (mimeName == "audio/x-riff" && suffix == "webp")) {
        mFormat = "webp";
        mDocumentType = detectAnimatedWebP() ? DocumentType::ANIMATED : DocumentType::STATIC;
    } else if(mimeName == "image/jxl") {
        mFormat = "jxl";
        mDocumentType = detectAnimatedJxl() ? DocumentType::ANIMATED : DocumentType::STATIC;
        if(mDocumentType == DocumentType::ANIMATED && !settings->jxlAnimation()) {
            mDocumentType = DocumentType::NONE;
            qDebug() << "animated jxl is off; skipping file";
        }
    } else if(mimeName == "image/avif") {
        mFormat = "avif";
        mDocumentType = detectAnimatedAvif() ? DocumentType::ANIMATED : DocumentType::STATIC;
    } else if(mimeName == "image/bmp") {
        mFormat = "bmp";
        mDocumentType = DocumentType::STATIC;
    } else {
        // just try to open via suffix if all of the above fails
        mFormat = suffix;
        if(mFormat.compare("jfif", Qt::CaseInsensitive) == 0)
            mFormat = "jpg";
        mDocumentType = DocumentType::STATIC;
    }
    // the orientation is not needed here: the image loader lets QImageReader
    // apply it while decoding, and only the thumbnailer asks for it explicitly
}

// dumb apng detector
bool DocumentInfo::detectAPNG() {
    return readHeader(fileInfo.filePath(), 0, 120).contains("acTL");
}

bool DocumentInfo::detectAnimatedWebP() {
    // "VP8X" chunk header, 4 bytes of chunk size, then flags (bit 1 = animated)
    const QByteArray header = readHeader(fileInfo.filePath(), 12, 9);
    if(header.size() < 9 || header.left(4) != "VP8X")
        return false;
    return (header.at(8) & (1 << 1)) != 0;
}

// TODO avoid creating multiple QImageReader instances
bool DocumentInfo::detectAnimatedJxl() {
    QImageReader r(fileInfo.filePath(), "jxl");
    return r.supportsAnimation();
}

bool DocumentInfo::detectAnimatedAvif() {
    return readHeader(fileInfo.filePath(), 4, 8) == "ftypavis";
}

void DocumentInfo::loadExifTags() {
    QMutexLocker locker(&exifMutex);
    loadExifTagsImpl();
}

void DocumentInfo::loadExifTagsImpl() {
    if(exifLoaded)
        return;
    exifLoaded = true;
    exifTags.clear();
#ifdef USE_EXIV2
    try {
        // exiv2 >= 0.28 dropped the wide-char path API on Windows, so feed the
        // file contents directly. This also keeps non-ASCII paths working.
        QFile exifFile(fileInfo.filePath());
        if(!exifFile.open(QFile::ReadOnly))
            return;
        const QByteArray exifBuffer = exifFile.readAll();
        if(exifBuffer.isEmpty())
            return;

        std::unique_ptr<Exiv2::Image> image;

        image = Exiv2::ImageFactory::open(
            reinterpret_cast<const Exiv2::byte*>(exifBuffer.constData()),
            static_cast<size_t>(exifBuffer.size()));

        assert(image.get() != 0);
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();
        if(exifData.empty())
            return;

        Exiv2::ExifKey make("Exif.Image.Make");
        Exiv2::ExifKey model("Exif.Image.Model");
        Exiv2::ExifKey dateTime("Exif.Image.DateTime");
        Exiv2::ExifKey exposureTime("Exif.Photo.ExposureTime");
        Exiv2::ExifKey fnumber("Exif.Photo.FNumber");
        Exiv2::ExifKey isoSpeedRatings("Exif.Photo.ISOSpeedRatings");
        Exiv2::ExifKey flash("Exif.Photo.Flash");
        Exiv2::ExifKey focalLength("Exif.Photo.FocalLength");
        Exiv2::ExifKey userComment("Exif.Photo.UserComment");

        Exiv2::ExifData::const_iterator it;

        it = exifData.findKey(make);
        if(it != exifData.end() /* && it->count() */)
            exifTags.insert(QObject::tr("Make"), QString::fromStdString(it->value().toString()));

        it = exifData.findKey(model);
        if(it != exifData.end())
            exifTags.insert(QObject::tr("Model"), QString::fromStdString(it->value().toString()));

        it = exifData.findKey(dateTime);
        if(it != exifData.end())
            exifTags.insert(QObject::tr("Date/Time"), QString::fromStdString(it->value().toString()));

        it = exifData.findKey(exposureTime);
        if(it != exifData.end()) {
            Exiv2::Rational r = it->toRational();
            if(r.first < r.second) {
                qreal exp = round(static_cast<qreal>(r.second) / r.first);
                exifTags.insert(QObject::tr("ExposureTime"), "1/" + QString::number(exp) + QObject::tr(" sec"));
            } else {
                qreal exp = round(static_cast<qreal>(r.first) / r.second);
                exifTags.insert(QObject::tr("ExposureTime"), QString::number(exp) + QObject::tr(" sec"));
            }
        }

        it = exifData.findKey(fnumber);
        if(it != exifData.end()) {
            Exiv2::Rational r = it->toRational();
            qreal fn = static_cast<qreal>(r.first) / r.second;
            exifTags.insert(QObject::tr("F Number"), "f/" + QString::number(fn, 'g', 3));
        }

        it = exifData.findKey(isoSpeedRatings);
        if(it != exifData.end())
            exifTags.insert(QObject::tr("ISO Speed ratings"), QString::fromStdString(it->value().toString()));

        it = exifData.findKey(flash);
        if(it != exifData.end())
            exifTags.insert(QObject::tr("Flash"), QString::fromStdString(it->value().toString()));

        it = exifData.findKey(focalLength);
        if(it != exifData.end()) {
            Exiv2::Rational r = it->toRational();
            qreal fn = static_cast<qreal>(r.first) / r.second;
            exifTags.insert(QObject::tr("Focal Length"), QString::number(fn, 'g', 3) + QObject::tr(" mm"));
        }

        it = exifData.findKey(userComment);
        if(it != exifData.end()) {
            // crop out 'charset=ascii' etc"
            auto comment = QString::fromStdString(it->value().toString());
            if(comment.startsWith("charset="))
                comment.remove(0, comment.indexOf(" ") + 1);
            exifTags.insert(QObject::tr("UserComment"), comment);
        }
    }

// this should work with both 0.28 and <0.28
#if not EXIV2_TEST_VERSION(0, 28, 0)
#ifdef __WIN32
    catch (Exiv2::BasicError<wchar_t>& e) {
        qDebug() << "Caught Exiv2::BasicError exception:\n" << e.what() << "\n";
        return;
    }
#else
    catch (Exiv2::BasicError<char>& e) {
        qDebug() << "Caught Exiv2::BasicError exception:\n" << e.what() << "\n";
        return;
    }
#endif
#endif

    catch (Exiv2::Error& e) {
        qDebug() << "Caught Exiv2 exception:\n" << e.what() << "\n";
        return;
    }
#endif
}

QMap<QString, QString> DocumentInfo::getExifTags() {
    QMutexLocker locker(&exifMutex);
    loadExifTagsImpl();
    return exifTags;
}

void DocumentInfo::loadExifOrientation() {
    if(mDocumentType == DocumentType::NONE)
        return;

    QString path = filePath();
    QImageReader reader(path, mFormat.isEmpty() ? nullptr : mFormat.toUtf8().constData());
    if(reader.canRead()) {
        mOrientation = static_cast<int>(reader.transformation());
        return;
    }
    // the format guessed from the extension can be wrong for mislabeled files,
    // so let qt detect the format from the contents
    if(!mFormat.isEmpty()) {
        QImageReader sniffed(path);
        if(sniffed.canRead())
            mOrientation = static_cast<int>(sniffed.transformation());
    }
}
