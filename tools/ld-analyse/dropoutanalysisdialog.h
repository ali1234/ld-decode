/************************************************************************

    dropoutanalysisdialog.h

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

#ifndef DROPOUTANALYSISDIALOG_H
#define DROPOUTANALYSISDIALOG_H

#include <QDialog>
#include <QtCharts>

#include "lddecodemetadata.h"

using namespace QtCharts;

namespace Ui {
class DropoutAnalysisDialog;
}

class DropoutAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DropoutAnalysisDialog(QWidget *parent = nullptr);
    ~DropoutAnalysisDialog();

    void startUpdate();
    void addDataPoint(qint32 fieldNumber, qreal doLength);
    void finishUpdate(qint32 numberOfFields, qint32 fieldsPerDataPoint);

private slots:
    void on_reset_pushButton_clicked();

private:
    void removeChartContents();

    Ui::DropoutAnalysisDialog *ui;
    QChartView chartView;
    QLineSeries qLineSeries;
    QChart chart;
    QValueAxis axisX;
    QValueAxis axisY;
    qreal maxY;

    bool chartOwnsContents;
};

#endif // DROPOUTANALYSISDIALOG_H
