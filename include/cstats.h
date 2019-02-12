#ifndef __CSTATS_H
#define __CSTATS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t (*CStatsGetter)(int categoryid, int valueid);

void csInit(void *logger, CStatsGetter getter, int maxvalues, long interval);
void csStart(void);
void csStop(void);
int csAddCategory(const char *name);
int csAddValue(int categoryid, const char *name);
int csGetInterval(char **response);
int csUpdateInterval(const char *json, char **response);
int csGetLive(char **response);

#ifdef __cplusplus
}
#endif

#endif /* __CSTATS_H */
