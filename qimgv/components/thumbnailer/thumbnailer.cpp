#include "thumbnailer.h"

namespace {

// shared by every Thumbnailer instance; constructed on first use (after
// settings are available). Intentionally leaked: the pool is stopped
// explicitly in Thumbnailer::shutdown() before the app exits.
struct ThumbnailerShared {
    ThumbnailCache cache;
    QThreadPool pool;

    ThumbnailerShared() {
        int threads = settings->thumbnailerThreadCount();
        int globalThreads = QThreadPool::globalInstance()->maxThreadCount();
        if(threads > globalThreads)
            threads = globalThreads;
        pool.setMaxThreadCount(threads);
    }
};

ThumbnailerShared *sharedThumbnailer = nullptr;

ThumbnailerShared &thumbnailerShared() {
    if(!sharedThumbnailer)
        sharedThumbnailer = new ThumbnailerShared();
    return *sharedThumbnailer;
}

} // namespace

Thumbnailer::Thumbnailer() {
    auto &shared = thumbnailerShared();
    cache = &shared.cache;
    pool = &shared.pool;
}

Thumbnailer::~Thumbnailer() {
    // the cache and the pool are owned by thumbnailerShared()
}

void Thumbnailer::shutdown() {
    // don't instantiate the shared data on the way out
    if(!sharedThumbnailer)
        return;
    sharedThumbnailer->pool.clear();
    sharedThumbnailer->pool.waitForDone();
}

void Thumbnailer::waitForDone() {
    pool->waitForDone();
}

void Thumbnailer::clearTasks() {
    pool->clear();
}

std::shared_ptr<Thumbnail> Thumbnailer::getThumbnail(QString filePath, int size) {
    return ThumbnailerRunnable::generate(nullptr, filePath, size, false, false);
}

void Thumbnailer::getThumbnailAsync(QString path, int size, bool crop, bool force) {
    if(!runningTasks.contains(path, size))
        startThumbnailerThread(path, size, crop, force);
}

void Thumbnailer::startThumbnailerThread(QString filePath, int size, bool crop, bool force) {
    auto runnable = new ThumbnailerRunnable(settings->useThumbnailCache() ? cache : nullptr, filePath, size, crop, force);
    connect(runnable, &ThumbnailerRunnable::taskStart, this, &Thumbnailer::onTaskStart);
    connect(runnable, &ThumbnailerRunnable::taskEnd, this, &Thumbnailer::onTaskEnd);
    runnable->setAutoDelete(true);
    pool->start(runnable);
}

void Thumbnailer::onTaskStart(QString filePath, int size) {
    runningTasks.insert(filePath, size);
}

void Thumbnailer::onTaskEnd(std::shared_ptr<Thumbnail> thumbnail, QString filePath) {
    runningTasks.remove(filePath, thumbnail->size());
    emit thumbnailReady(thumbnail, filePath);
}
