#ifndef __CSTATS_H
#define __CSTATS_H

#ifdef __cplusplus
extern "C" {
#endif

void csInit(void *logger, long interval);
void csStart(void);
void csStop(void);
int csGetInterval(char **response);
int csGetStatLogging(char **response);
int csUpdateStatLogging(const char *json, char **response);
int csUpdateInterval(const char *json, char **response);
int csGetLive(char **response);
int csGetLiveAll(char **response);
void get_current_file_size(size_t len);
void cli_init(cli_node_t *cli_node, int *cnt_peer);

#ifdef __cplusplus
}
#endif

#endif /* __CSTATS_H */
