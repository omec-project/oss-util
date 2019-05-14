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
        eCLSeverityMinor,
        eCLSeverityMajor,
        eCLSeverityCritical
};

extern int clSystemLog;
//extern int optStatMaxSize; 


void clSetOption(enum CLoggerOptions opt, const char *val);

void clInit(const char *app);
void clStart(void);
void clStop(void);

int clAddLogger(const char *logname);
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

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLOGGER_H */
