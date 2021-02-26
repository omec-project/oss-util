/*
 * Copyright (c) 2019 Sprint
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __CLOGGER_H
#define __CLOGGER_H

#define __file__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG_FORMAT "%s:%s:%d:"
#define LOG_VALUE __file__, __func__, __LINE__


#ifdef __cplusplus
extern "C" {
#endif

enum CLoggerOptions {
        eCLOptLogFileName,
        eCLOptLogMaxSize,
        eCLOptLogNumberFiles,
        eCLOptStatFileName,
        eCLOptStatMaxSize,
        eCLOptStatNumberFiles,
        eCLOptAuditFileName,
        eCLOptAuditMaxSize,
        eCLOptAuditNumberFiles,
        eCLOptLogQueueSize
};

enum CLoggerSeverity {
        eCLSeverityTrace,
        eCLSeverityDebug,
        eCLSeverityInfo,
        eCLSeverityMinor,
        eCLSeverityMajor,
        eCLSeverityCritical
};

enum log_level
{
	activate_log_level = 1,
};
extern int clSystemLog;
extern int optStatMaxSize;


void clSetOption(enum CLoggerOptions opt, const char *val);

void clInit(const char *app, uint8_t cp_logger);
void clStart(void);
void clStop(void);

int clAddLogger(const char *logname, uint8_t cp_logger);
char *clGetLoggers(void);
int clUpdateLogger(const char *json, char **response);

void clLog(const int log, enum CLoggerSeverity sev, const char *fmt, ...);
int clLogger(char **response);

void *clGetAuditLogger(void);
void *clGetStatsLogger(void);

void clAddRecentLogger(const char *name,const char *app_name, int max);
void clAddobject(const char *category, char *log_level, const char *message);
int clRecentLogger(const char *request,char **response);
int clRecentLogMaxsize(const char *request, char **response);
int clRecentSetMaxsize(const char *json, char **response);
int clchange_file_size(const char *json, char **response);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLOGGER_H */
