/************************************************************************

    decoder.h

    ld-chroma-decoder - Colourisation filter for ld-decode
    Copyright (C) 2019 Adam Sampson

    This file is part of ld-decode-tools.

    ld-chroma-decoder is free software: you can redistribute it and/or
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

#ifndef DECODER_H
#define DECODER_H

#include <QAtomicInt>
#include <QByteArray>
#include <QDebug>
#include <QThread>
#include <cassert>

#include "lddecodemetadata.h"

#include "sourcefield.h"

class DecoderPool;

// Abstract base class for chroma decoders.
//
// For each chroma decoder in ld-chroma-decoder, there is a subclass of this
// class, and a corresponding subclass of DecoderThread -- let's say
// SecamDecoder and SecamThread.
//
// main() creates an instance of SecamDecoder and passes it to DecoderPool.
// DecoderPool calls SecamDecoder::configure with the input video parameters,
// then calls SecamDecoder::makeThread repeatedly to populate its thread pool.
//
// SecamThread::run fetches input frames from DecoderPool and writes completed
// output frames back to DecoderPool; it keeps going until there are no input
// frames left, or until abort becomes true. If it detects that something's
// gone wrong, it sets abort to true and returns.
//
// This means that you can have state shared between all the decoder threads,
// in SecamDecoder, or specific to each thread, in SecamThread -- and
// DecoderPool doesn't need to know anything specific about the decoder.
class Decoder {
public:
    virtual ~Decoder() = default;

    // Configure the decoder given input video parameters.
    // If the video is not compatible, print an error message and return false.
    virtual bool configure(const LdDecodeMetaData::VideoParameters &videoParameters) = 0;

    // After configuration, return the number of frames that the decoder needs
    // to be able to see into the past (each frame being two SourceFields).
    // The default implementation returns 0, which is appropriate for 1D/2D decoders.
    virtual qint32 getLookBehind() const;

    // After configuration, return the number of frames that the decoder needs
    // to be able to see into the future (each frame being two SourceFields).
    // The default implementation returns 0, which is appropriate for 1D/2D decoders.
    virtual qint32 getLookAhead() const;

    // Construct a new worker thread
    virtual QThread *makeThread(QAtomicInt& abort, DecoderPool& decoderPool) = 0;

    // Parameters used by the decoder and its threads.
    // This may be subclassed by decoders to add extra parameters.
    struct Configuration {
        // Parameters computed from the video metadata
        LdDecodeMetaData::VideoParameters videoParameters;
        qint32 firstActiveLine;
        qint32 lastActiveLine;
        qint32 topPadLines;
        qint32 bottomPadLines;
    };

    // Compute the output frame size in Configuration, adjusting the active
    // video region as required
    static void setVideoParameters(Configuration &config, const LdDecodeMetaData::VideoParameters &videoParameters,
                                   qint32 firstActiveLine, qint32 lastActiveLine);

    // Crop a full decoded frame to the output frame size
    static QByteArray cropOutputFrame(const Configuration &config, QByteArray outputData);
};

// Abstract base class for chroma decoder worker threads.
class DecoderThread : public QThread {
    Q_OBJECT
public:
    explicit DecoderThread(QAtomicInt &abort, DecoderPool &decoderPool, QObject *parent = nullptr);

protected:
    void run() override;

    // Decode a sequence of fields into a sequence of frames
    virtual void decodeFrames(const QVector<SourceField> &inputFields, qint32 startIndex, qint32 endIndex,
                              QVector<QByteArray> &outputFrames) = 0;

    // Decoder pool
    QAtomicInt& abort;
    DecoderPool& decoderPool;
};

#endif
