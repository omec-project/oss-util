#ifndef __CLOGGER_H
#define __CLOGGER_H

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
        eCLSeverityStartup,
        eCLSeverityWarn,
        eCLSeverityError
};

extern int clSystemLog;

void clSetOption(enum CLoggerOptions opt, const char *val);

void clInit(void);
void clStart(void);
void clStop(void);

int clAddLogger(const char *logname);
char *clGetLoggers(void);
int clUpdateLogger(const char *json, char **response);

void clLog(const int log, enum CLoggerSeverity sev, const char *fmt, ...);

void *clGetAuditLogger(void);
void *clGetStatsLogger(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLOGGER_H */
