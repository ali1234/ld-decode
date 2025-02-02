/************************************************************************

    ntscdecoder.cpp

    ld-chroma-decoder - Colourisation filter for ld-decode
    Copyright (C) 2018 Chad Page
    Copyright (C) 2018-2019 Simon Inns
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

#include "ntscdecoder.h"

#include "decoderpool.h"

NtscDecoder::NtscDecoder(const Comb::Configuration &combConfig)
{
    config.combConfig = combConfig;
}

bool NtscDecoder::configure(const LdDecodeMetaData::VideoParameters &videoParameters) {
    // Ensure the source video is NTSC
    if (videoParameters.isSourcePal) {
        qCritical() << "This decoder is for NTSC video sources only";
        return false;
    }

    // Compute cropping parameters
    setVideoParameters(config, videoParameters, config.combConfig.firstActiveLine, config.combConfig.lastActiveLine);

    return true;
}

qint32 NtscDecoder::getLookBehind() const
{
    if (config.combConfig.use3D) {
        // In 3D mode, we need to see the previous frame
        return 1;
    }

    return 0;
}

QThread *NtscDecoder::makeThread(QAtomicInt& abort, DecoderPool& decoderPool)
{
    return new NtscThread(abort, decoderPool, config);
}

NtscThread::NtscThread(QAtomicInt& _abort, DecoderPool &_decoderPool,
                       const NtscDecoder::Configuration &_config, QObject *parent)
    : DecoderThread(_abort, _decoderPool, parent), config(_config)
{
    // Configure NTSC decoder
    comb.updateConfiguration(config.videoParameters, config.combConfig);
}

void NtscThread::decodeFrames(const QVector<SourceField> &inputFields, qint32 startIndex, qint32 endIndex,
                              QVector<QByteArray> &outputFrames)
{
    // Decode lookahead fields, discarding the result
    for (qint32 i = 0; i < startIndex; i += 2) {
        comb.decodeFrame(inputFields[i], inputFields[i + 1]);
    }

    // Decode real fields to frames
    for (qint32 i = startIndex, j = 0; i < endIndex; i += 2, j++) {
        // Filter the frame
        QByteArray outputData = comb.decodeFrame(inputFields[i], inputFields[i + 1]);

        // The NTSC filter outputs the whole frame, so here we crop it to the required dimensions
        outputFrames[j] = NtscDecoder::cropOutputFrame(config, outputData);
    }
}
