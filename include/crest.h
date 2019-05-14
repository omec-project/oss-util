#ifndef __CREST_H
#define __CREST_H

#ifdef __cplusplus
extern "C" {
#endif

enum CRCommand
{
	eCRCommandGet,
	eCRCommandPost,
	eCRCommandPut,
	eCRCommandDelete
};

typedef int (*CRestStaticHandler)(const char *requestBody, char **responseBody);
typedef int (*CRestDynamicHandler)(const char *param, const char *requestBody, char **responseBody);

void crInit(void *auditLogger, int port, size_t thread_count);
void crStart(void);
void crStop(void);
void crRegisterStaticHandler(enum CRCommand cmd, const char *route, CRestStaticHandler handler);
void crRegisterDynamicHandler(enum CRCommand cmd, const char *baseroute, const char *param, const char *route, CRestDynamicHandler handler);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CREST_H */
