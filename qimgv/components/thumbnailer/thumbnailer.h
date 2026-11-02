#pragma once

#include <QThreadPool>
#include "components/thumbnailer/thumbnailerrunnable.h"
#include "components/cache/thumbnailcache.h"
#include "settings.h"

class Thumbnailer : public QObject
{
    Q_OBJECT
public:
    explicit Thumbnailer();
    ~Thumbnailer();
    static std::shared_ptr<Thumbnail> getThumbnail(QString filePath, int size);
    void clearTasks();
    void waitForDone();
    // stops the shared worker pool and waits for it; call before the app exits
    static void shutdown();

public slots:
    void getThumbnailAsync(QString path, int size, bool crop, bool force);

private:
    // the thumbnail cache and the thread pool are shared between all
    // Thumbnailer instances (there is one per directory view), so that the
    // cache directory is only scanned once and there is a single pool
    ThumbnailCache *cache;
    QThreadPool *pool;
    void startThumbnailerThread(QString filePath, int size, bool crop, bool force);
    QMultiMap<QString, int> runningTasks;

private slots:
    void onTaskStart(QString filePath, int size);
    void onTaskEnd(std::shared_ptr<Thumbnail> thumbnail, QString filePath);

signals:
    void thumbnailReady(std::shared_ptr<Thumbnail> thumbnail, QString filePath);
};
