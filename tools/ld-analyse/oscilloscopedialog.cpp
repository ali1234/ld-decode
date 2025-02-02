﻿/************************************************************************

    oscilloscopedialog.cpp

    ld-analyse - TBC output analysis
    Copyright (C) 2018-2019 Simon Inns

    This file is part of ld-decode-tools.

    ld-analyse is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

************************************************************************/

#include "oscilloscopedialog.h"
#include "ui_oscilloscopedialog.h"

OscilloscopeDialog::OscilloscopeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OscilloscopeDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window);

    maximumScanLines = 625;
    lastScopeLine = 0;
    lastScopeDot = 0;
    scopeWidth = 0;

    // Configure the GUI
    ui->scanLineSpinBox->setMinimum(1);
    ui->scanLineSpinBox->setMaximum(625);

    ui->previousPushButton->setAutoRepeat(true);
    ui->previousPushButton->setAutoRepeatInterval(50);

    ui->nextPushButton->setAutoRepeat(true);
    ui->nextPushButton->setAutoRepeatInterval(50);

    ui->previousPushButton->setFocusPolicy(Qt::NoFocus);
    ui->nextPushButton->setFocusPolicy(Qt::NoFocus);
}

OscilloscopeDialog::~OscilloscopeDialog()
{
    delete ui;
}

void OscilloscopeDialog::showTraceImage(TbcSource::ScanLineData scanLineData, qint32 scanLine, qint32 pictureDot, qint32 frameHeight)
{
    qDebug() << "OscilloscopeDialog::showTraceImage(): Called for scan-line" << scanLine << "with picture dot" << pictureDot;

    // Set the dialogue title based on the scan-line
    this->setWindowTitle("Oscilloscope trace for scan-line #" + QString::number(scanLine));

    // Get the raw field data for the selected line
    QImage traceImage;
    lastScopeLine = scanLine;
    lastScopeDot = pictureDot;

    traceImage = getFieldLineTraceImage(scanLineData, pictureDot);

    // Add the QImage to the QLabel in the dialogue
    ui->scopeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->scopeLabel->setAlignment(Qt::AlignCenter);
    ui->scopeLabel->setScaledContents(true);
    ui->scopeLabel->setPixmap(QPixmap::fromImage(traceImage));

    // Update the scan-line spinbox
    ui->scanLineSpinBox->setMaximum(frameHeight);
    ui->scanLineSpinBox->setValue(scanLine);

    // Update the maximum scan-lines limit
    maximumScanLines = frameHeight;
}

QImage OscilloscopeDialog::getFieldLineTraceImage(TbcSource::ScanLineData scanLineData, qint32 pictureDot)
{
    // Get the display settings from the UI
    bool showYC = ui->YCcheckBox->isChecked();
    bool showY = ui->YcheckBox->isChecked();
    bool showC = ui->CcheckBox->isChecked();
    bool showDropouts = ui->dropoutsCheckBox->isChecked();

    // These are fixed, but may be changed to options later
    qint32 scopeHeight = 2048;
    scopeWidth = scanLineData.data.size();

    qint32 scopeScale = 65536 / scopeHeight;

    // Define image with width, height and format
    QImage scopeImage(scanLineData.data.size(), scopeHeight, QImage::Format_RGB888);
    QPainter scopePainter;

    // Ensure we have valid data
    if (scanLineData.data.isEmpty()) {
        qWarning() << "Did not get valid data for the requested field!";
        return scopeImage;
    }

    // Set the background to black
    scopeImage.fill(Qt::black);

    // Attach the scope image to the painter
    scopePainter.begin(&scopeImage);

    // Add the black and white levels
    // Note: For PAL this should be black at 64 and white at 211

    // Scale to 512 pixel height
    qint32 blackIre = scopeHeight - (scanLineData.blackIre / scopeScale);
    qint32 whiteIre = scopeHeight - (scanLineData.whiteIre / scopeScale);
    qint32 midPointIre = scanLineData.blackIre + ((scanLineData.whiteIre - scanLineData.blackIre) / 2);
    midPointIre = scopeHeight - (midPointIre / scopeScale);

    scopePainter.setPen(Qt::white);
    scopePainter.drawLine(0, blackIre, scanLineData.data.size(), blackIre);
    scopePainter.drawLine(0, whiteIre, scanLineData.data.size(), whiteIre);

    // If showing C - draw the IRE mid-point
    if (showC) {
        scopePainter.setPen(Qt::gray);
        scopePainter.drawLine(0, midPointIre, scanLineData.data.size(), midPointIre);
    }

    // Draw the indicator lines
    scopePainter.setPen(Qt::blue);
    scopePainter.drawLine(scanLineData.colourBurstStart, 0, scanLineData.colourBurstStart, scopeHeight);
    scopePainter.drawLine(scanLineData.colourBurstEnd, 0, scanLineData.colourBurstEnd, scopeHeight);
    scopePainter.setPen(Qt::cyan);
    scopePainter.drawLine(scanLineData.activeVideoStart, 0, scanLineData.activeVideoStart, scopeHeight);
    scopePainter.drawLine(scanLineData.activeVideoEnd, 0, scanLineData.activeVideoEnd, scopeHeight);

    // Get the signal data
    QVector<qint32> signalDataYC; // Luminance (Y) and Chrominance (C) combined
    QVector<bool> dropOutYC; // Drop out locations within YC data
    QVector<qint32> signalDataY; // Luminance (Y) only
    QVector<qint32> signalDataC; // Chrominance (C) only
    signalDataYC.resize(scanLineData.data.size());
    dropOutYC.resize(scanLineData.data.size());
    signalDataY.resize(scanLineData.data.size());
    signalDataC.resize(scanLineData.data.size());

    // To extract Y from PAL, a LPF of 3.8MHz is required
    // To extract Y from NTSC, a LPF of 3.0MHz is required
    // To extract C from PAL, a HPF of 3.8MHz is required
    // To extract C from NTSC, a HPF of 3.0MHz is required

    // Get the YC data
    for (qint32 xPosition = 0; xPosition < scanLineData.data.size(); xPosition++) {
        // Get the data for the current pixel
        signalDataYC[xPosition] = scanLineData.data[xPosition];
        dropOutYC[xPosition] = scanLineData.isDropout[xPosition];
    }

    if (showY) {
        // Filter out the Y data (with a simple LPF)
        qreal cutOffFrequency;
        if (scanLineData.isSourcePal) cutOffFrequency = 380000; // 3.8MHz for PAL
        else cutOffFrequency = 300000; // 3.0MHz for NTSC
        qreal sampleRate = 17734476; // details.sampleRate = 17734476;
        qreal rc = 1.0 / (cutOffFrequency * 2.0 * 3.1415927);
        qreal dt = 1.0 / sampleRate; // Sample rate
        qreal alpha = dt / (rc + dt);
        signalDataY[0] = signalDataYC[0];
        for(qint32 i = 1; i < signalDataYC.size(); i++) {
            qreal result = signalDataY[i-1] + (alpha * (signalDataYC[i] - signalDataY[i - 1]));
            signalDataY[i] = static_cast<qint32>(result);
        }
    }

    if (showC) {
        // Filter out the C data (with a simple HPF)
        qreal cutOffFrequency;
        if (scanLineData.isSourcePal) cutOffFrequency = 380000; // 3.8MHz for PAL
        else cutOffFrequency = 300000; // 3.0MHz for NTSC
        qreal sampleRate = 17734476; // details.sampleRate = 17734476;
        qreal rc = 1.0 / (cutOffFrequency * 2.0 * 3.1415927);
        qreal dt = 1.0 / sampleRate; // Sample rate
        qreal alpha = rc / (rc + dt);
        signalDataC[0] = signalDataYC[0];
        for(qint32 i = 1; i < signalDataYC.size(); i++) {
            qreal result = alpha * (signalDataC[i-1] + signalDataYC[i] - signalDataYC[i-1]);
            signalDataC[i] = static_cast<qint32>(result);
        }
    }

    // Draw the scope image
    scopePainter.setPen(Qt::yellow);
    qint32 lastSignalLevelYC = 0;
    qint32 lastSignalLevelY = 0;
    qint32 lastSignalLevelC = 0;
    for (qint32 xPosition = 0; xPosition < scanLineData.data.size(); xPosition++) {
        if (showYC) {
            // Scale (to 0-512) and invert
            qint32 signalLevelYC = scopeHeight - (signalDataYC[xPosition] / scopeScale);

            if (xPosition != 0) {
                // Non-active video area YC is yellow, active is white
                if (!showY && !showC) scopePainter.setPen(Qt::white); else scopePainter.setPen(Qt::darkGray);
                if (xPosition < scanLineData.colourBurstEnd || xPosition > scanLineData.activeVideoEnd) scopePainter.setPen(Qt::yellow);

                // Highlight dropouts
                if (dropOutYC[xPosition] && showDropouts) scopePainter.setPen(Qt::red);

                // Draw a line from the last YC signal to the current one
                scopePainter.drawLine(xPosition - 1, lastSignalLevelYC, xPosition, signalLevelYC);
            }

            // Remember the current signal's level
            lastSignalLevelYC = signalLevelYC;
        }

        if (showC) {
            // Scale (to 0-512) and invert
            qint32 signalLevelC = (scopeHeight - (signalDataC[xPosition] / scopeScale)) - (scopeHeight - midPointIre);

            if (xPosition != 0) {
                // Draw a line from the last Y signal to the current one (signal green, out of range in yellow)
                if (xPosition > scanLineData.colourBurstEnd && xPosition < scanLineData.activeVideoEnd) {
                    if (signalLevelC > blackIre || signalLevelC < whiteIre) scopePainter.setPen(Qt::yellow);
                    else scopePainter.setPen(Qt::green);
                    scopePainter.drawLine(xPosition - 1, lastSignalLevelC, xPosition, signalLevelC);
                }
            }

            // Remember the current signal's level
            lastSignalLevelC = signalLevelC;
        }

        if (showY) {
            // Scale (to 0-512) and invert
            qint32 signalLevelY = scopeHeight - (signalDataY[xPosition] / scopeScale);

            if (xPosition != 0) {
                // Draw a line from the last Y signal to the current one (signal white, out of range in red)
                if (xPosition > scanLineData.colourBurstEnd && xPosition < scanLineData.activeVideoEnd) {
                    if (signalLevelY > blackIre || signalLevelY < whiteIre) scopePainter.setPen(Qt::red);
                    else scopePainter.setPen(Qt::white);
                    scopePainter.drawLine(xPosition - 1, lastSignalLevelY, xPosition, signalLevelY);
                }
            }

            // Remember the current signal's level
            lastSignalLevelY = signalLevelY;
        }
    }

    // Draw the picture dot position line
    scopePainter.setPen(QColor(0, 255, 0, 127));
    scopePainter.drawLine(pictureDot, 0, pictureDot, scopeHeight);

    // Return the QImage
    scopePainter.end();
    return scopeImage;
}

// GUI signal handlers ------------------------------------------------------------------------------------------------

void OscilloscopeDialog::on_previousPushButton_clicked()
{
    if (ui->scanLineSpinBox->value() != 1) {
        emit scanLineChanged(ui->scanLineSpinBox->value() - 1, lastScopeDot);
    }
}

void OscilloscopeDialog::on_nextPushButton_clicked()
{
    if (ui->scanLineSpinBox->value() < maximumScanLines) {
        emit scanLineChanged(ui->scanLineSpinBox->value() + 1, lastScopeDot);
    }
}

void OscilloscopeDialog::on_scanLineSpinBox_valueChanged(int arg1)
{
    (void)arg1;
    if (ui->scanLineSpinBox->value() != lastScopeLine)
        emit scanLineChanged(ui->scanLineSpinBox->value(), lastScopeDot);
}

void OscilloscopeDialog::on_YCcheckBox_clicked()
{
    emit scanLineChanged(ui->scanLineSpinBox->value(), lastScopeDot);
}

void OscilloscopeDialog::on_YcheckBox_clicked()
{
    emit scanLineChanged(ui->scanLineSpinBox->value(), lastScopeDot);
}

void OscilloscopeDialog::on_CcheckBox_clicked()
{
    emit scanLineChanged(ui->scanLineSpinBox->value(), lastScopeDot);
}

void OscilloscopeDialog::on_dropoutsCheckBox_clicked()
{
    emit scanLineChanged(ui->scanLineSpinBox->value(), lastScopeDot);
}

// Mouse press event handler
void OscilloscopeDialog::mousePressEvent(QMouseEvent *event)
{
    // Get the mouse position relative to our scene
    QPoint origin = ui->scopeLabel->mapFromGlobal(QCursor::pos());

    // Check that the mouse click is within bounds of the current picture
    qint32 oX = origin.x();
    qint32 oY = origin.y();

    if (oX + 1 >= 0 &&
            oY >= 0 &&
            oX + 1 <= ui->scopeLabel->width() &&
            oY <= ui->scopeLabel->height()) {

        mousePictureDotSelect(oX);
        event->accept();
    }
}

// Mouse drag event handler
void OscilloscopeDialog::mouseMoveEvent(QMouseEvent *event)
{
    // Get the mouse position relative to our scene
    QPoint origin = ui->scopeLabel->mapFromGlobal(QCursor::pos());

    // Check that the mouse click is within bounds of the current picture
    qint32 oX = origin.x();
    qint32 oY = origin.y();

    if (oX + 1 >= 0 &&
            oY >= 0 &&
            oX + 1 <= ui->scopeLabel->width() &&
            oY <= ui->scopeLabel->height()) {

        mousePictureDotSelect(oX);
        event->accept();
    }
}

void OscilloscopeDialog::mousePictureDotSelect(qint32 oX)
{
    qreal unscaledXR = (static_cast<qreal>(scopeWidth) /
                        static_cast<qreal>(ui->scopeLabel->width())) * static_cast<qreal>(oX);

    qint32 unscaledX = static_cast<qint32>(unscaledXR);
    if (unscaledX > scopeWidth - 1) unscaledX = scopeWidth - 1;
    if (unscaledX < 0) unscaledX = 0;

    // Remember the last dot selected
    lastScopeDot = unscaledX;

    emit scanLineChanged(ui->scanLineSpinBox->value(), lastScopeDot);
}
