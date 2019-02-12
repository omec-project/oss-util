Introduction:
=============
OSS-UTIL is a interface and APIs to implement CLI and logging support for
various applications. Using OSS-UTIL will make CLI and logging support common
across all the applications. OSS-UTIL comes as library which can be linked to
the application. Hence it helps keeping application core implementation
completely separate from the CLI and logging interface.

Install OSS UTIL:
=================
1. Install and Build the OSS UTIL library.

        $ cd {installation_root}/oss-util
        $ ./install.sh

2. Install.sh script will download all the packages, modules and also
   build the oss util library.

3. In case you want to build the util library, you can build it

        $ cd {installation_root}/oss-util
        $ make


Usage:
================
Static library build using instructions given can be directly linked to the
application to which CLI, logging or statistics supports needs to be provided.

For interface APIs of CLI refer to following class and header file:
Class:- _RestStaticHandler, _RestDynamicHandler, RestHandler, RestEndpoint,
            Logger, CStatValue, CStatCategory, CStatsUpdateInterval,
            CStatsGenerateCSV, CStats
Header File:- creat.h, cstats.h, clogger.h, sthread.h

For Interface APIs of logging please refer to following class and header file:
Class:- Loggger, LoggerException, SLogger
Header File:- clogger.h, slogger.h


