/************************************************************************

    sourcevideo.h

    ld-decode-tools TBC library
    Copyright (C) 2018-2019 Simon Inns

    This file is part of ld-decode-tools.

    ld-decode-tools is free software: you can redistribute it and/or
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

#ifndef SOURCEVIDEO_H
#define SOURCEVIDEO_H

#include <QObject>
#include <QFile>
#include <QCache>
#include <QDebug>

class SourceVideo : public QObject
{
    Q_OBJECT

public:
    explicit SourceVideo(QObject *parent = nullptr);
    ~SourceVideo() override;

    // File handling methods
    bool open(QString filename, qint32 _fieldLength, qint32 _fieldLineLength = -1);
    void close(void);

    // Field handling methods
    QByteArray getVideoField(qint32 fieldNumber);
    QByteArray getVideoField(qint32 fieldNumber, qint32 startFieldLine, qint32 endFieldLine);

    // Get and set methods
    bool isSourceValid();
    qint32 getNumberOfAvailableFields();
    qint32 getFieldByteLength();

private:
    // File handling globals
    QFile inputFile;
    bool isSourceVideoOpen;
    qint32 availableFields;
    qint32 fieldByteLength;
    qint32 fieldLineLength;

    QByteArray outputFieldLineData;

    // Field caching
    QCache<qint32, QByteArray> fieldCache;
};

#endif // SOURCEVIDEO_H
