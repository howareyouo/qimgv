#include "thumbnail.h"
#include <QApplication>

Thumbnail::Thumbnail(QString _name, QString _info, int _size, std::shared_ptr<QImage> _image)
    : mName(_name),
      mInfo(_info),
      mImage(std::move(_image)),
      mSize(_size)
{
    if(mImage)
        mHasAlphaChannel = mImage->hasAlphaChannel();
}

QString Thumbnail::name() {
    return mName;
}

QString Thumbnail::info() {
    return mInfo;
}

int Thumbnail::size() {
    return mSize;
}

bool Thumbnail::hasAlphaChannel() {
    return mHasAlphaChannel;
}

std::shared_ptr<QPixmap> Thumbnail::pixmap() {
    if(!mPixmap && mImage) {
        auto pix = std::make_shared<QPixmap>(QPixmap::fromImage(*mImage));
        pix->setDevicePixelRatio(qApp->devicePixelRatio());
        mPixmap = std::move(pix);
    }
    return mPixmap;
}
