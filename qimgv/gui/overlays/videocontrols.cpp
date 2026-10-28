#include "videocontrols.h"
#include "ui_videocontrols.h"

VideoControls::VideoControls(FloatingWidgetContainer *parent) :
    OverlayWidget(parent),
    ui(new Ui::VideoControls)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_NoMousePropagation, true);
    hide();
    ui->pauseButton->setIconPath(":res/icons/common/buttons/videocontrols/play24.png");
    ui->pauseButton->setAction("pauseVideo");
    ui->prevFrameButton->setIconPath(":res/icons/common/buttons/videocontrols/skip-backwards24.png");
    ui->prevFrameButton->setAction("frameStepBack");
    ui->nextFrameButton->setIconPath(":res/icons/common/buttons/videocontrols/skip-forward24.png");
    ui->nextFrameButton->setAction("frameStep");

    lastPosition = -1;

    readSettings();
    connect(settings, &Settings::settingsChanged, this, &VideoControls::readSettings);

    connect(ui->seekBar, &VideoSlider::sliderMovedX, this, &VideoControls::seek);

    if(parent)
        setContainerSize(parent->size());
}

void VideoControls::readSettings() {
    if(settings->panelEnabled() && settings->panelPosition() == PanelPosition::PANEL_BOTTOM)
        setPosition(FloatingWidgetPosition::TOP);
    else
        setPosition(FloatingWidgetPosition::BOTTOM);
}

VideoControls::~VideoControls() {
    delete ui;
}

void VideoControls::setPlaybackDuration(int duration) {
    QString durationStr = QString::number(duration);
    ui->seekBar->setRange(0, duration - 1);
    ui->durationLabel->setText(durationStr);
    ui->positionLabel->setText(durationStr);
    recalculateGeometry();
    ui->positionLabel->setText("");
}

void VideoControls::setPlaybackPosition(int position) {
    if(position == lastPosition)
        return;
    QString positionStr = QString::number(position + 1);
    ui->positionLabel->setText(positionStr);
    ui->seekBar->blockSignals(true);
    ui->seekBar->setValue(position);
    ui->seekBar->blockSignals(false);
    lastPosition = position;
}

void VideoControls::onPlaybackPaused(bool mode) {
    if(mode)
        ui->pauseButton->setIconPath(":res/icons/common/buttons/videocontrols/play24.png");
    else
        ui->pauseButton->setIconPath(":res/icons/common/buttons/videocontrols/pause24.png");
}
