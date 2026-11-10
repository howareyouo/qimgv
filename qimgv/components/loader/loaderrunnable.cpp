#include "loaderrunnable.h"

LoaderRunnable::LoaderRunnable(QString _path) : path(_path) {
}

void LoaderRunnable::run() {
    auto image = ImageFactory::createImage(path);
    emit finished(image, path);
}
