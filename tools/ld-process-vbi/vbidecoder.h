/************************************************************************

    vbidecoder.h

    ld-process-vbi - VBI and IEC NTSC specific processor for ld-decode
    Copyright (C) 2018-2019 Simon Inns

    This file is part of ld-decode-tools.

    ld-process-vbi is free software: you can redistribute it and/or
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

#ifndef VBIDECODER_H
#define VBIDECODER_H

#include <QObject>
#include <QAtomicInt>
#include <QThread>
#include <QDebug>

#include "lddecodemetadata.h"
#include "fmcode.h"
#include "whiteflag.h"
#include "closedcaption.h"

class DecoderPool;

class VbiDecoder : public QThread {
    Q_OBJECT

public:
    explicit VbiDecoder(QAtomicInt& _abort, DecoderPool& _decoderPool, QObject *parent = nullptr);

protected:
    void run() override;

private:
    // Decoder pool
    QAtomicInt& abort;
    DecoderPool& decoderPool;

    // Temporary output buffer
    LdDecodeMetaData::Field outputData;

    QByteArray getActiveVideoLine(QByteArray *sourceFrame, qint32 scanLine, LdDecodeMetaData::VideoParameters videoParameters);
    qint32 manchesterDecoder(QByteArray lineData, qint32 zcPoint, LdDecodeMetaData::VideoParameters videoParameters);
    QVector<bool> getTransitionMap(QByteArray lineData, qint32 zcPoint);
};

#endif // VBIDECODER_H
