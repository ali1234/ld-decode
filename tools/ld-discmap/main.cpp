/************************************************************************

    main.cpp

    ld-discmap - TBC and VBI alignment and correction
    Copyright (C) 2019 Simon Inns

    This file is part of ld-decode-tools.

    ld-discmap is free software: you can redistribute it and/or
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

#include <QCoreApplication>
#include <QDebug>
#include <QtGlobal>
#include <QCommandLineParser>

#include "logging.h"
#include "discmap.h"

int main(int argc, char *argv[])
{
    // Install the local debug message handler
    setDebug(true);
    qInstallMessageHandler(debugOutputHandler);

    QCoreApplication a(argc, argv);

    // Set application name and version
    QCoreApplication::setApplicationName("ld-discmap");
    QCoreApplication::setApplicationVersion("1.0");
    QCoreApplication::setOrganizationDomain("domesday86.com");

    // Set up the command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
                "ld-discmap - TBC and VBI alignment and correction\n"
                "\n"
                "(c)2019 Simon Inns\n"
                "GPLv3 Open-Source - github: https://github.com/happycube/ld-decode");
    parser.addHelpOption();
    parser.addVersionOption();

    // Option to show debug (-d / --debug)
    QCommandLineOption showDebugOption(QStringList() << "d" << "debug",
                                       QCoreApplication::translate("main", "Show debug"));
    parser.addOption(showDebugOption);

    // Option to reverse the field order (-r / --reverse)
    QCommandLineOption setReverseOption(QStringList() << "r" << "reverse",
                                       QCoreApplication::translate("main", "Reverse the field order to second/first (default first/second)"));
    parser.addOption(setReverseOption);

    // Option to only perform mapping (without saving) (-m / --maponly)
    QCommandLineOption setMapOnlyOption(QStringList() << "m" << "maponly",
                                       QCoreApplication::translate("main", "Only perform mapping, but do not save to target (for testing purposes)"));
    parser.addOption(setMapOnlyOption);

    // Positional argument to specify input TBC file
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "Specify input TBC file"));

    // Positional argument to specify output TBC file
    parser.addPositionalArgument("output", QCoreApplication::translate("main", "Specify output TBC file"));

    // Process the command line options and arguments given by the user
    parser.process(a);

    // Get the options from the parser
    bool isDebugOn = parser.isSet(showDebugOption);
    bool reverse = parser.isSet(setReverseOption);
    bool mapOnly = parser.isSet(setMapOnlyOption);

    // Process the command line options
    QString inputFilename;
    QString outputFilename;
    QStringList positionalArguments = parser.positionalArguments();
    if (!mapOnly) {
        // Require source and target filenames
        if (positionalArguments.count() == 2) {
            inputFilename = positionalArguments.at(0);
            outputFilename = positionalArguments.at(1);
        } else {
            // Quit with error
            qCritical("You must specify input and output TBC files");
            return -1;
        }

        if (inputFilename == outputFilename) {
            // Quit with error
            qCritical("Input and output TBC files cannot have the same file names");
            return -1;
        }
    } else {
        // Require only source filename
        if (positionalArguments.count() > 0) {
            inputFilename = positionalArguments.at(0);
            outputFilename = "";
        } else {
            // Quit with error
            qCritical("You must specify the input TBC file");
            return -1;
        }
    }

    if (isDebugOn) setDebug(true); else setDebug(false);

    // Process the TBC file
    DiscMap discMap;
    if (!discMap.process(inputFilename, outputFilename, reverse, mapOnly)) {
        return 1;
    }

    // Quit with success
    return 0;
}
