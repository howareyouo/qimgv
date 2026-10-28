#pragma once

#include <QString>
#include <QImage>
#include <QPixmap>
#include <memory>

class Thumbnail {
public:
    Thumbnail(QString _name, QString _info, int _size, std::shared_ptr<QImage> _image);
    QString name();
    QString info();
    int size();
    bool hasAlphaChannel();
    std::shared_ptr<QPixmap> pixmap(); // converts mImage on first call; GUI thread only
private:
    QString mName, mInfo;
    std::shared_ptr<QImage> mImage;
    std::shared_ptr<QPixmap> mPixmap;
    int mSize;
    bool mHasAlphaChannel;
};
