#ifndef WINDOWSWATCHERWORKER_H
#define WINDOWSWATCHERWORKER_H

#include <windows.h>
#include "../watcherworker.h"
#include <QDebug>

class WindowsWorker : public WatcherWorker {
    Q_OBJECT
public:
    WindowsWorker();

    void setDirectoryHandle(HANDLE hDir);
    virtual void run() override;

signals:
    void notifyEvent(PFILE_NOTIFY_INFORMATION);

private:
    HANDLE hDir;
    uint POLL_RATE_MS = 1000;
    void freeHandle();
};

#endif // WINDOWSWATCHERWORKER_H
