/************************************************************************

    mainwindow.cpp

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

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QString inputFilenameParam, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Set up dialogues
    oscilloscopeDialog = new OscilloscopeDialog(this);
    aboutDialog = new AboutDialog(this);
    vbiDialog = new VbiDialog(this);
    dropoutAnalysisDialog = new DropoutAnalysisDialog(this);
    snrAnalysisDialog = new SnrAnalysisDialog(this);
    busyDialog = new BusyDialog(this);
    closedCaptionDialog = new ClosedCaptionsDialog(this);
    palChromaDecoderConfigDialog = new PalChromaDecoderConfigDialog(this);

    // Add a status bar to show the state of the source video file
    ui->statusBar->addWidget(&sourceVideoStatus);
    ui->statusBar->addWidget(&fieldNumberStatus);
    sourceVideoStatus.setText(tr("No source video file loaded"));
    fieldNumberStatus.setText(tr(" -  Fields: ./."));

    // Set the initial frame number
    currentFrameNumber = 1;

    // Connect to the scan line changed signal from the oscilloscope dialogue
    connect(oscilloscopeDialog, &OscilloscopeDialog::scanLineChanged, this, &MainWindow::scanLineChangedSignalHandler);
    lastScopeLine = 1;
    lastScopeDot = 1;

    // Connect to the PAL decoder configuration changed signal
    connect(palChromaDecoderConfigDialog, &PalChromaDecoderConfigDialog::palChromaDecoderConfigChanged, this, &MainWindow::palConfigurationChangedSignalHandler);

    // Connect to the TbcSource signals (busy loading and finished loading)
    connect(&tbcSource, &TbcSource::busyLoading, this, &MainWindow::on_busyLoading);
    connect(&tbcSource, &TbcSource::finishedLoading, this, &MainWindow::on_finishedLoading);

    // Load the window geometry and settings from the configuration
    restoreGeometry(configuration.getMainWindowGeometry());
    scaleFactor = configuration.getMainWindowScaleFactor();
    vbiDialog->restoreGeometry(configuration.getVbiDialogGeometry());
    oscilloscopeDialog->restoreGeometry(configuration.getOscilloscopeDialogGeometry());
    dropoutAnalysisDialog->restoreGeometry(configuration.getDropoutAnalysisDialogGeometry());
    snrAnalysisDialog->restoreGeometry(configuration.getSnrAnalysisDialogGeometry());
    closedCaptionDialog->restoreGeometry(configuration.getClosedCaptionDialogGeometry());
    palChromaDecoderConfigDialog->restoreGeometry(configuration.getPalChromaDecoderConfigDialogGeometry());

    // Store the current button palette for the show dropouts button
    buttonPalette = ui->dropoutsPushButton->palette();

    // Set the GUI to unloaded
    updateGuiUnloaded();

    // Was a filename specified on the command line?
    if (!inputFilenameParam.isEmpty()) {
        tbcSource.loadSource(inputFilenameParam);
    }
}

MainWindow::~MainWindow()
{
    // Save the window geometry and settings to the configuration
    configuration.setMainWindowGeometry(saveGeometry());
    configuration.setMainWindowScaleFactor(scaleFactor);
    configuration.setVbiDialogGeometry(vbiDialog->saveGeometry());
    configuration.setOscilloscopeDialogGeometry(oscilloscopeDialog->saveGeometry());
    configuration.setDropoutAnalysisDialogGeometry(dropoutAnalysisDialog->saveGeometry());
    configuration.setSnrAnalysisDialogGeometry(snrAnalysisDialog->saveGeometry());
    configuration.setClosedCaptionDialogGeometry(closedCaptionDialog->saveGeometry());
    configuration.setPalChromaDecoderConfigDialogGeometry(palChromaDecoderConfigDialog->saveGeometry());
    configuration.writeConfiguration();

    // Close the source video if open
    if (tbcSource.getIsSourceLoaded()) {
        tbcSource.unloadSource();
    }

    delete ui;
}

// Update GUI methods for when TBC source files are loaded and unloaded -----------------------------------------------

// Method to update the GUI when a file is loaded
void MainWindow::updateGuiLoaded()
{
    // Enable the frame controls
    ui->frameNumberSpinBox->setEnabled(true);
    ui->previousPushButton->setEnabled(true);
    ui->nextPushButton->setEnabled(true);
    ui->startFramePushButton->setEnabled(true);
    ui->endFramePushButton->setEnabled(true);
    ui->frameHorizontalSlider->setEnabled(true);
    ui->mediaControl_frame->setEnabled(true);

    // Update the current frame number
    currentFrameNumber = 1;
    ui->frameNumberSpinBox->setMinimum(1);
    ui->frameNumberSpinBox->setMaximum(tbcSource.getNumberOfFrames());
    ui->frameNumberSpinBox->setValue(1);
    ui->frameHorizontalSlider->setMinimum(1);
    ui->frameHorizontalSlider->setMaximum(tbcSource.getNumberOfFrames());
    ui->frameHorizontalSlider->setPageStep(tbcSource.getNumberOfFrames() / 100);
    ui->frameHorizontalSlider->setValue(1);

    // Allow the next and previous frame buttons to auto-repeat
    ui->previousPushButton->setAutoRepeat(true);
    ui->previousPushButton->setAutoRepeatDelay(500);
    ui->previousPushButton->setAutoRepeatInterval(1);
    ui->nextPushButton->setAutoRepeat(true);
    ui->nextPushButton->setAutoRepeatDelay(500);
    ui->nextPushButton->setAutoRepeatInterval(1);

    // Enable menu options
    ui->actionLine_scope->setEnabled(true);
    ui->actionVBI->setEnabled(true);
    ui->actionNTSC->setEnabled(true);
    ui->actionVideo_metadata->setEnabled(true);
    ui->actionVITS_Metrics->setEnabled(true);
    ui->actionZoom_In->setEnabled(true);
    ui->actionZoom_Out->setEnabled(true);
    ui->actionZoom_1x->setEnabled(true);
    ui->actionZoom_2x->setEnabled(true);
    ui->actionZoom_3x->setEnabled(true);
    ui->actionDropout_analysis->setEnabled(true);
    ui->actionSNR_analysis->setEnabled(true);
    ui->actionSave_frame_as_PNG->setEnabled(true);
    ui->actionSave_metadata_as_CSV->setEnabled(true);
    ui->actionClosed_Captions->setEnabled(true);
    if (tbcSource.getIsSourcePal()) ui->actionPAL_Chroma_decoder->setEnabled(true);
    else ui->actionPAL_Chroma_decoder->setEnabled(false);

    // Set option button states
    ui->videoPushButton->setText(tr("Chroma"));
    ui->dropoutsPushButton->setText(tr("Show dropouts"));
    ui->fieldOrderPushButton->setText(tr("Reverse Field-order"));

    // Set zoom button states
    ui->zoomInPushButton->setEnabled(true);
    ui->zoomOutPushButton->setEnabled(true);
    ui->originalSizePushButton->setEnabled(true);

    ui->zoomInPushButton->setAutoRepeat(true);
    ui->zoomInPushButton->setAutoRepeatDelay(500);
    ui->zoomInPushButton->setAutoRepeatInterval(100);
    ui->zoomOutPushButton->setAutoRepeat(true);
    ui->zoomOutPushButton->setAutoRepeatDelay(500);
    ui->zoomOutPushButton->setAutoRepeatInterval(100);

    // Update the status bar
    QString statusText;
    if (tbcSource.getIsSourcePal()) statusText += "PAL";
    else statusText += "NTSC";
    statusText += " source loaded with ";
    statusText += QString::number(tbcSource.getNumberOfFrames());
    statusText += " sequential frames available";
    sourceVideoStatus.setText(statusText);

    // Update the PAL chroma configuration dialogue
    palChromaDecoderConfigDialog->setConfiguration(tbcSource.getPalColourConfiguration());

    // Show the current frame
    showFrame();

    // Ensure the busy dialogue is hidden
    busyDialog->hide();
}

// Method to update the GUI when a file is unloaded
void MainWindow::updateGuiUnloaded()
{
    // Disable the frame controls
    ui->frameNumberSpinBox->setEnabled(false);
    ui->previousPushButton->setEnabled(false);
    ui->nextPushButton->setEnabled(false);
    ui->startFramePushButton->setEnabled(false);
    ui->endFramePushButton->setEnabled(false);
    ui->frameHorizontalSlider->setEnabled(false);
    ui->mediaControl_frame->setEnabled(false);

    // Update the current frame number
    currentFrameNumber = 1;
    ui->frameNumberSpinBox->setValue(currentFrameNumber);
    currentFrameNumber = 1;
    ui->frameHorizontalSlider->setValue(currentFrameNumber);
    currentFrameNumber = 1;

    // Set the window title
    this->setWindowTitle(tr("ld-analyse"));

    // Set the status bar text
    sourceVideoStatus.setText(tr("No source video file loaded"));
    fieldNumberStatus.setText(tr(" -  Fields: ./."));

    // Disable menu options
    ui->actionLine_scope->setEnabled(false);
    ui->actionVBI->setEnabled(false);
    ui->actionNTSC->setEnabled(false);
    ui->actionVideo_metadata->setEnabled(false);
    ui->actionVITS_Metrics->setEnabled(false);
    ui->actionZoom_In->setEnabled(false);
    ui->actionZoom_Out->setEnabled(false);
    ui->actionZoom_1x->setEnabled(false);
    ui->actionZoom_2x->setEnabled(false);
    ui->actionZoom_3x->setEnabled(false);
    ui->actionDropout_analysis->setEnabled(false);
    ui->actionSNR_analysis->setEnabled(false);
    ui->actionSave_frame_as_PNG->setEnabled(false);
    ui->actionSave_metadata_as_CSV->setEnabled(false);
    ui->actionClosed_Captions->setEnabled(false);
    ui->actionPAL_Chroma_decoder->setEnabled(false);

    // Set option button states
    ui->videoPushButton->setText(tr("Chroma"));
    ui->dropoutsPushButton->setText(tr("Show dropouts"));
    ui->fieldOrderPushButton->setText(tr("Reverse Field-order"));

    // Set zoom button states
    ui->zoomInPushButton->setEnabled(false);
    ui->zoomOutPushButton->setEnabled(false);
    ui->originalSizePushButton->setEnabled(false);

    // Hide the displayed frame
    hideFrame();

    // Hide graphs
    snrAnalysisDialog->hide();
    dropoutAnalysisDialog->hide();

    // Hide configuration dialogues
    palChromaDecoderConfigDialog->hide();
}

// Frame display methods ----------------------------------------------------------------------------------------------

// Method to display a sequential frame
void MainWindow::showFrame()
{
    // Show the field numbers
    fieldNumberStatus.setText(" -  Fields: " + QString::number(tbcSource.getFirstFieldNumber(currentFrameNumber)) + "/" +
                              QString::number(tbcSource.getSecondFieldNumber(currentFrameNumber)));

    // If there are dropouts in the frame, highlight the show dropouts button
    if (tbcSource.getIsDropoutPresent(currentFrameNumber)) {
        QPalette tempPalette = buttonPalette;
        tempPalette.setColor(QPalette::Button, QColor(Qt::lightGray));
        ui->dropoutsPushButton->setAutoFillBackground(true);
        ui->dropoutsPushButton->setPalette(tempPalette);
        ui->dropoutsPushButton->update();
    } else {
        ui->dropoutsPushButton->setAutoFillBackground(true);
        ui->dropoutsPushButton->setPalette(buttonPalette);
        ui->dropoutsPushButton->update();
    }

    // Update the VBI dialogue
    if (vbiDialog->isVisible()) vbiDialog->updateVbi(tbcSource.getFrameVbi(currentFrameNumber),
                                                     tbcSource.getIsFrameVbiValid(currentFrameNumber)); 

    // Add the QImage to the QLabel in the dialogue
    ui->frameViewerLabel->clear();
    ui->frameViewerLabel->setScaledContents(false);
    ui->frameViewerLabel->setAlignment(Qt::AlignCenter);
    updateFrameViewer();

    // If the scope window is open, update it too (using the last scope line selected by the user)
    if (oscilloscopeDialog->isVisible()) {
        // Show the oscilloscope dialogue for the selected scan-line
        updateOscilloscopeDialogue(lastScopeLine, lastScopeDot);
    }

    // Update the closed caption dialog
    if (!tbcSource.getIsSourcePal()) {
        closedCaptionDialog->addData(currentFrameNumber, tbcSource.getCcData0(currentFrameNumber), tbcSource.getCcData1(currentFrameNumber));
    }
}

// Redraw the frame viewer (for example, when scaleFactor has been changed)
void MainWindow::updateFrameViewer()
{
    QImage frameImage = tbcSource.getFrameImage(currentFrameNumber);

    if (ui->mouseModePushButton->isChecked()) {
        // Create a painter object
        QPainter imagePainter;
        imagePainter.begin(&frameImage);

        // Draw lines to indicate the current scope position
        imagePainter.setPen(QColor(0, 255, 0, 127));
        imagePainter.drawLine(0, lastScopeLine - 1, tbcSource.getFrameWidth(), lastScopeLine - 1);
        imagePainter.drawLine(lastScopeDot, 0, lastScopeDot, tbcSource.getFrameHeight());

        // End the painter object
        imagePainter.end();
    }

    ui->frameViewerLabel->setPixmap(QPixmap::fromImage(frameImage));
    ui->frameViewerLabel->setPixmap(ui->frameViewerLabel->pixmap()->scaled(scaleFactor * ui->frameViewerLabel->pixmap()->size(),
                                                                           Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

// Method to hide the current frame
void MainWindow::hideFrame()
{
    ui->frameViewerLabel->clear();
}

// Misc private methods -----------------------------------------------------------------------------------------------

// Load a TBC file based on the passed file name
void MainWindow::loadTbcFile(QString inputFileName)
{
    // Update the GUI
    updateGuiUnloaded();

    // Close current source video (if loaded)
    if (tbcSource.getIsSourceLoaded()) tbcSource.unloadSource();

    // Load the source
    tbcSource.loadSource(inputFileName);

    // Note: loading continues in the background...
}

// Method to update the line oscilloscope based on the frame number and scan line
void MainWindow::updateOscilloscopeDialogue(qint32 scanLine, qint32 pictureDot)
{
    // Update the oscilloscope dialogue
    oscilloscopeDialog->showTraceImage(tbcSource.getScanLineData(currentFrameNumber, scanLine),
                                       scanLine, pictureDot, tbcSource.getFrameHeight());
}

// Menu bar signal handlers -------------------------------------------------------------------------------------------

void MainWindow::on_actionExit_triggered()
{
    qDebug() << "MainWindow::on_actionExit_triggered(): Called";

    // Quit the application
    qApp->quit();
}

// Load a TBC file based on the file selection from the GUI
void MainWindow::on_actionOpen_TBC_file_triggered()
{
    qDebug() << "MainWindow::on_actionOpen_TBC_file_triggered(): Called";

    QString inputFileName = QFileDialog::getOpenFileName(this,
                tr("Open TBC file"),
                configuration.getSourceDirectory()+tr("/ldsample.tbc"),
                tr("TBC output (*.tbc);;All Files (*)"));

    // Was a filename specified?
    if (!inputFileName.isEmpty() && !inputFileName.isNull()) {
        loadTbcFile(inputFileName);
    }
}

// Display the scan line oscilloscope view
void MainWindow::on_actionLine_scope_triggered()
{
    if (tbcSource.getIsSourceLoaded()) {
        // Show the oscilloscope dialogue for the selected scan-line
        updateOscilloscopeDialogue(lastScopeLine, lastScopeDot);
        oscilloscopeDialog->show();
    }
}

// Show the about window
void MainWindow::on_actionAbout_ld_analyse_triggered()
{
    aboutDialog->show();
}

// Show the VBI window
void MainWindow::on_actionVBI_triggered()
{
    // Show the VBI dialogue
    vbiDialog->updateVbi(tbcSource.getFrameVbi(currentFrameNumber), tbcSource.getIsFrameVbiValid(currentFrameNumber));
    vbiDialog->show();
}

// Show the drop out analysis graph
void MainWindow::on_actionDropout_analysis_triggered()
{
    // Show the dropout analysis dialogue
    dropoutAnalysisDialog->show();
}

// Show the SNR analysis graph
void MainWindow::on_actionSNR_analysis_triggered()
{
    // Show the SNR analysis dialogue
    snrAnalysisDialog->show();
}

// Save current frame as PNG
void MainWindow::on_actionSave_frame_as_PNG_triggered()
{
    qDebug() << "MainWindow::on_actionSave_frame_as_PNG_triggered(): Called";

    // Create a suggestion for the filename
    QString filenameSuggestion = configuration.getPngDirectory();
    if (tbcSource.getIsSourcePal()) filenameSuggestion += tr("/frame_pal_");
    else filenameSuggestion += tr("/frame_ntsc_");
    if (!tbcSource.getChromaDecoder()) filenameSuggestion += tr("source_");
    else filenameSuggestion += tr("comb_");
    filenameSuggestion += QString::number(currentFrameNumber) + tr(".png");

    QString pngFilename = QFileDialog::getSaveFileName(this,
                tr("Save PNG file"),
                filenameSuggestion,
                tr("PNG image (*.png);;All Files (*)"));

    // Was a filename specified?
    if (!pngFilename.isEmpty() && !pngFilename.isNull()) {
        // Save the current frame as a PNG
        qDebug() << "MainWindow::on_actionSave_frame_as_PNG_triggered(): Saving current frame as" << pngFilename;

        // Generate the current frame and save it
        if (!tbcSource.getFrameImage(currentFrameNumber).save(pngFilename)) {
            qDebug() << "MainWindow::on_actionSave_frame_as_PNG_triggered(): Failed to save file as" << pngFilename;

            QMessageBox messageBox;
            messageBox.warning(this, "Warning","Could not save a PNG using the specified filename!");
            messageBox.setFixedSize(500, 200);
        }

        // Update the configuration for the PNG directory
        QFileInfo pngFileInfo(pngFilename);
        configuration.setPngDirectory(pngFileInfo.absolutePath());
        qDebug() << "MainWindow::on_actionSave_frame_as_PNG_triggered(): Setting PNG directory to:" << pngFileInfo.absolutePath();
        configuration.writeConfiguration();
    }
}

// Save the VITS metadata as a CSV file
void MainWindow::on_actionSave_metadata_as_CSV_triggered()
{
    qDebug() << "MainWindow::on_actionSave_metadata_as_CSV_triggered(): Called";

    // Create a suggestion for the filename
    QString filenameSuggestion = configuration.getCsvDirectory() + tr("/");
    filenameSuggestion += tbcSource.getCurrentSourceFilename() + tr(".csv");

    QString csvFilename = QFileDialog::getSaveFileName(this,
                tr("Save CSV file"),
                filenameSuggestion,
                tr("CSV file (*.csv);;All Files (*)"));

    // Was a filename specified?
    if (!csvFilename.isEmpty() && !csvFilename.isNull()) {
        // Save the metadata as CSV
        qDebug() << "MainWindow::on_actionSave_metadata_as_CSV_triggered(): Saving VITS metadata as" << csvFilename;

        if (tbcSource.saveVitsAsCsv(csvFilename)) {
            // Update the configuration for the CSV directory
            QFileInfo csvFileInfo(csvFilename);
            configuration.setCsvDirectory(csvFileInfo.absolutePath());
            qDebug() << "MainWindow::on_actionSave_metadata_as_CSV_triggered(): Setting CSV directory to:" << csvFileInfo.absolutePath();
            configuration.writeConfiguration();
        } else {
            // Save as CSV failed
            qDebug() << "MainWindow::on_actionSave_metadata_as_CSV_triggered(): Failed to save file as" << csvFilename;

            QMessageBox messageBox;
            messageBox.warning(this, "Warning","Could not save a CSV file using the specified filename!");
            messageBox.setFixedSize(500, 200);
        }
    }
}

// Zoom in menu option
void MainWindow::on_actionZoom_In_triggered()
{
    on_zoomInPushButton_clicked();
}

// Zoom out menu option
void MainWindow::on_actionZoom_Out_triggered()
{
    on_zoomOutPushButton_clicked();
}

// Original size 1:1 zoom menu option
void MainWindow::on_actionZoom_1x_triggered()
{
    on_originalSizePushButton_clicked();
}

// 2:1 zoom menu option
void MainWindow::on_actionZoom_2x_triggered()
{
    scaleFactor = 2.0;
    updateFrameViewer();
}

// 3:1 zoom menu option
void MainWindow::on_actionZoom_3x_triggered()
{
    scaleFactor = 3.0;
    updateFrameViewer();
}

// Show closed captions
void MainWindow::on_actionClosed_Captions_triggered()
{
    closedCaptionDialog->show();
}

// Show PAL Chroma-Decoder configuration
void MainWindow::on_actionPAL_Chroma_decoder_triggered()
{
    palChromaDecoderConfigDialog->show();
}

// Media control frame signal handlers --------------------------------------------------------------------------------

// Previous frame button has been clicked
void MainWindow::on_previousPushButton_clicked()
{
    currentFrameNumber--;
    if (currentFrameNumber < 1) {
        currentFrameNumber = 1;
    } else {
        ui->frameNumberSpinBox->setValue(currentFrameNumber);
        ui->frameHorizontalSlider->setValue(currentFrameNumber);
    }
}

// Next frame button has been clicked
void MainWindow::on_nextPushButton_clicked()
{
    currentFrameNumber++;
    if (currentFrameNumber > tbcSource.getNumberOfFrames()) {
        currentFrameNumber = tbcSource.getNumberOfFrames();
    } else {
        ui->frameNumberSpinBox->setValue(currentFrameNumber);
        ui->frameHorizontalSlider->setValue(currentFrameNumber);
    }
}

// Skip to end frame button has been clicked
void MainWindow::on_endFramePushButton_clicked()
{
    currentFrameNumber = tbcSource.getNumberOfFrames();
    ui->frameNumberSpinBox->setValue(currentFrameNumber);
    ui->frameHorizontalSlider->setValue(currentFrameNumber);
}

// Skip to start frame button has been clicked
void MainWindow::on_startFramePushButton_clicked()
{
    currentFrameNumber = 1;
    ui->frameNumberSpinBox->setValue(currentFrameNumber);
    ui->frameHorizontalSlider->setValue(currentFrameNumber);
}

// Frame number spin box editing has finished
void MainWindow::on_frameNumberSpinBox_editingFinished()
{
    if (ui->frameNumberSpinBox->value() != currentFrameNumber) {
        if (ui->frameNumberSpinBox->value() < 1) ui->frameNumberSpinBox->setValue(1);
        if (ui->frameNumberSpinBox->value() > tbcSource.getNumberOfFrames()) ui->frameNumberSpinBox->setValue(tbcSource.getNumberOfFrames());
        currentFrameNumber = ui->frameNumberSpinBox->value();
        ui->frameHorizontalSlider->setValue(currentFrameNumber);
        showFrame();
    }
}

// Frame slider value has changed
void MainWindow::on_frameHorizontalSlider_valueChanged(int value)
{
    (void)value;
    if (!tbcSource.getIsSourceLoaded()) return;

    currentFrameNumber = ui->frameHorizontalSlider->value();

    // If the spinbox is enabled, we can update the current frame number
    // otherwisew we just ignore this
    if (ui->frameNumberSpinBox->isEnabled()) {
        ui->frameNumberSpinBox->setValue(currentFrameNumber);
        showFrame();
    }
}

// Source/Chroma select button clicked
void MainWindow::on_videoPushButton_clicked()
{
    if (tbcSource.getChromaDecoder()) {
        tbcSource.setChromaDecoder(false);
        ui->videoPushButton->setText(tr("Chroma"));
    } else {
        tbcSource.setChromaDecoder(true);
        ui->videoPushButton->setText(tr("Source"));
    }

    // Show the current frame
    showFrame();
}

// Show/hide dropouts button clicked
void MainWindow::on_dropoutsPushButton_clicked()
{
    if (tbcSource.getHighlightDropouts()) {
        tbcSource.setHighlightDropouts(false);
        ui->dropoutsPushButton->setText(tr("Show dropouts"));
    } else {
        tbcSource.setHighlightDropouts(true);
        ui->dropoutsPushButton->setText(tr("Hide dropouts"));
    }

    // Show the current frame (why isn't this option passed?)
    showFrame();
}

// Normal/Reverse field order button clicked
void MainWindow::on_fieldOrderPushButton_clicked()
{
    if (tbcSource.getFieldOrder()) {
        tbcSource.setFieldOrder(false);

        // If the TBC field order is changed, the number of available frames can change, so we need to update the GUI
        updateGuiLoaded();
        ui->fieldOrderPushButton->setText(tr("Reverse Field-order"));
    } else {
        tbcSource.setFieldOrder(true);

        // If the TBC field order is changed, the number of available frames can change, so we need to update the GUI
        updateGuiLoaded();
        ui->fieldOrderPushButton->setText(tr("Normal Field-order"));
    }

    // Show the current frame
    showFrame();
}

// Zoom in
void MainWindow::on_zoomInPushButton_clicked()
{
    qreal factor = 1.1;
    if (((scaleFactor * factor) > 0.333) && ((scaleFactor * factor) < 3.0)) {
        scaleFactor *= factor;
    }

    updateFrameViewer();
}

// Zoom out
void MainWindow::on_zoomOutPushButton_clicked()
{
    qreal factor = 0.9;
    if (((scaleFactor * factor) > 0.333) && ((scaleFactor * factor) < 3.0)) {
        scaleFactor *= factor;
    }

    updateFrameViewer();
}

// Original size 1:1 zoom
void MainWindow::on_originalSizePushButton_clicked()
{
    scaleFactor = 1.0;
    updateFrameViewer();
}

// Mouse mode button clicked
void MainWindow::on_mouseModePushButton_clicked()
{
    if (ui->mouseModePushButton->isChecked()) {
        // Show the oscilloscope view if currently hidden
        if (!oscilloscopeDialog->isVisible()) {
            updateOscilloscopeDialogue(lastScopeLine, lastScopeDot);
            oscilloscopeDialog->show();
        }
    }

    // Update the frame viewer to display/hide the indicator line
    updateFrameViewer();
}

// Miscellaneous handler methods --------------------------------------------------------------------------------------

// Handler called when another class changes the currenly selected scan line
void MainWindow::scanLineChangedSignalHandler(qint32 scanLine, qint32 pictureDot)
{
    qDebug() << "MainWindow::scanLineChangedSignalHandler(): Called with scanLine =" << scanLine << "and picture dot" << pictureDot;

    if (tbcSource.getIsSourceLoaded()) {
        // Show the oscilloscope dialogue for the selected scan-line
        lastScopeDot = pictureDot;
        lastScopeLine = scanLine;
        updateOscilloscopeDialogue(lastScopeLine, lastScopeDot);
        oscilloscopeDialog->show();

        // Update the frame viewer
        updateFrameViewer();
    }
}

// Mouse press event handler
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (!tbcSource.getIsSourceLoaded()) return;

    // Get the mouse position relative to our scene
    QPoint origin = ui->frameViewerLabel->mapFromGlobal(QCursor::pos());

    // Check that the mouse click is within bounds of the current picture
    qint32 oX = origin.x();
    qint32 oY = origin.y();

    if (oX + 1 >= 0 &&
            oY >= 0 &&
            oX + 1 <= ui->frameViewerLabel->width() &&
            oY <= ui->frameViewerLabel->height()) {

        mouseScanLineSelect(oX, oY);
        event->accept();
    }
}

// Mouse move event
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!tbcSource.getIsSourceLoaded()) return;

    // Get the mouse position relative to our scene
    QPoint origin = ui->frameViewerLabel->mapFromGlobal(QCursor::pos());

    // Check that the mouse click is within bounds of the current picture
    qint32 oX = origin.x();
    qint32 oY = origin.y();

    if (oX + 1 >= 0 &&
            oY >= 0 &&
            oX + 1 <= ui->frameViewerLabel->width() &&
            oY <= ui->frameViewerLabel->height()) {

        mouseScanLineSelect(oX, oY);
        event->accept();
    }
}

// Perform mouse based scan line selection
void MainWindow::mouseScanLineSelect(qint32 oX, qint32 oY)
{
    // X calc
    qreal offsetX = ((static_cast<qreal>(ui->frameViewerLabel->width()) -
                     static_cast<qreal>(ui->frameViewerLabel->pixmap()->width())) / 2.0);

    qreal unscaledXR = (static_cast<qreal>(tbcSource.getFrameWidth()) /
                        static_cast<qreal>(ui->frameViewerLabel->pixmap()->width())) * static_cast<qreal>(oX - offsetX);
    qint32 unscaledX = static_cast<qint32>(unscaledXR);
    if (unscaledX > tbcSource.getFrameWidth() - 1) unscaledX = tbcSource.getFrameWidth() - 1;
    if (unscaledX < 0) unscaledX = 0;

    // Y Calc
    qreal offsetY = ((static_cast<qreal>(ui->frameViewerLabel->height()) -
                     static_cast<qreal>(ui->frameViewerLabel->pixmap()->height())) / 2.0);

    qreal unscaledYR = (static_cast<qreal>(tbcSource.getFrameHeight()) /
                        static_cast<qreal>(ui->frameViewerLabel->pixmap()->height())) * static_cast<qreal>(oY - offsetY);
    qint32 unscaledY = static_cast<qint32>(unscaledYR);
    if (unscaledY > tbcSource.getFrameHeight()) unscaledY = tbcSource.getFrameHeight();
    if (unscaledY < 1) unscaledY = 1;

    // Show the oscilloscope dialogue for the selected scan-line (if the right mouse mode is selected)
    if (ui->mouseModePushButton->isChecked()) {
        // Remember the last line rendered
        lastScopeLine = unscaledY;
        lastScopeDot = unscaledX;

        updateOscilloscopeDialogue(lastScopeLine, lastScopeDot);
        oscilloscopeDialog->show();

        // Update the frame viewer
        updateFrameViewer();
    }
}

// Handle PAL configuration changed signal from the PAL configuration dialogue
void MainWindow::palConfigurationChangedSignalHandler()
{
    // Set the new configuration
    tbcSource.setPalColourConfiguration(palChromaDecoderConfigDialog->getConfiguration());

    // Update the frame viewer;
    updateFrameViewer();
}

// TbcSource class signal handlers ------------------------------------------------------------------------------------

// Signal handler for busyLoading signal from TbcSource class
void MainWindow::on_busyLoading(QString infoMessage)
{
    qDebug() << "MainWindow::on_busyLoading(): Got signal with message" << infoMessage;
    // Set the busy message and centre the dialog in the parent window
    busyDialog->setMessage(infoMessage);
    busyDialog->move(this->geometry().center() - busyDialog->rect().center());

    if (!busyDialog->isVisible()) {
        // Disable the main window during loading
        this->setEnabled(false);
        busyDialog->setEnabled(true);

        busyDialog->show();
    }
}

// Signal handler for finishedLoading signal from TbcSource class
void MainWindow::on_finishedLoading()
{
    qDebug() << "MainWindow::on_finishedLoading(): Called";

    // Ensure source loaded ok
    if (tbcSource.getIsSourceLoaded()) {
        // Generate the graph data
        dropoutAnalysisDialog->startUpdate();
        snrAnalysisDialog->startUpdate();

        qint32 fieldNumber = 1;
        for (qint32 i = 0; i < tbcSource.getGraphDataSize(); i++) {
            dropoutAnalysisDialog->addDataPoint(fieldNumber, tbcSource.getDropOutGraphData()[i]);
            snrAnalysisDialog->addDataPoint(fieldNumber, tbcSource.getBlackSnrGraphData()[i], tbcSource.getWhiteSnrGraphData()[i]);
            fieldNumber += tbcSource.getFieldsPerGraphDataPoint();
        }

        dropoutAnalysisDialog->finishUpdate(tbcSource.getNumberOfFields(), tbcSource.getFieldsPerGraphDataPoint());
        snrAnalysisDialog->finishUpdate(tbcSource.getNumberOfFields(), tbcSource.getFieldsPerGraphDataPoint());

        // Update the GUI
        updateGuiLoaded();

        // Set the main window title
        this->setWindowTitle(tr("ld-analyse - ") + tbcSource.getCurrentSourceFilename());

        // Update the configuration for the source directory
        QFileInfo inFileInfo(tbcSource.getCurrentSourceFilename());
        configuration.setSourceDirectory(inFileInfo.absolutePath());
        qDebug() << "MainWindow::loadTbcFile(): Setting source directory to:" << inFileInfo.absolutePath();
        configuration.writeConfiguration();
    } else {
        // Load failed
        updateGuiUnloaded();

        // Show an error to the user
        QMessageBox messageBox;
        messageBox.warning(this, "Error", "Could not load source TBC file");
        messageBox.setFixedSize(500, 200);
    }

    // Hide the busy dialogue and enable the main window
    busyDialog->hide();
    this->setEnabled(true);
}



