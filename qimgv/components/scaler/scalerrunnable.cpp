#include "scalerrunnable.h"

ScalerRunnable::ScalerRunnable() {
}

void ScalerRunnable::setRequest(ScalerRequest r) {
    req = r;
}

void ScalerRunnable::run() {
    emit started(req);
    QImage *scaled = nullptr;
    if(req.filter == 0 || (req.size.width() > req.image->width() && !settings->smoothUpscaling())) {
        scaled = ImageLib::scaled(req.image->getImage(), req.size, QI_FILTER_NEAREST);
    } else {
        scaled = ImageLib::scaled(req.image->getImage(), req.size, req.filter);
    }
    emit finished(scaled, req);
}
