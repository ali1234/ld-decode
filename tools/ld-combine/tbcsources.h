/************************************************************************

    tbcsources.h

    ld-combine - TBC combination and enhancement tool
    Copyright (C) 2019 Simon Inns

    This file is part of ld-decode-tools.

    ld-combine is free software: you can redistribute it and/or
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

#ifndef TBCSOURCES_H
#define TBCSOURCES_H

#include <QObject>
#include <QList>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>

// TBC library includes
#include "sourcevideo.h"
#include "lddecodemetadata.h"
#include "vbidecoder.h"

class TbcSources : public QObject
{
    Q_OBJECT
public:
    explicit TbcSources(QObject *parent = nullptr);

    bool loadSource(QString filename, bool reverse);
    bool unloadSource();
    bool saveSource(QString outputFilename, qint32 vbiStartFrame, qint32 length, qint32 dodThreshold);
    qint32 getNumberOfAvailableSources();
    qint32 getMinimumVbiFrameNumber();
    qint32 getMaximumVbiFrameNumber();

private:
    struct Source {
        SourceVideo sourceVideo;
        LdDecodeMetaData ldDecodeMetaData;
        QString filename;
        qint32 minimumVbiFrameNumber;
        qint32 maximumVbiFrameNumber;
        bool isSourceCav;
    };

    struct CombinedFrame {
        QByteArray firstFieldData;
        QByteArray secondFieldData;

        LdDecodeMetaData::Field firstFieldMetadata;
        LdDecodeMetaData::Field secondFieldMetadata;
    };

    // The frame number is common between sources
    qint32 currentVbiFrameNumber;

    QVector<Source*> sourceVideos;
    qint32 currentSource;

    CombinedFrame combineFrame(qint32 targetVbiFrame, qint32 threshold);
    bool setDiscTypeAndMaxMinFrameVbi(qint32 sourceNumber);
    qint32 convertVbiFrameNumberToSequential(qint32 vbiFrameNumber, qint32 sourceNumber);
};

#endif // TBCSOURCES_H
