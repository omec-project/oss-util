#ifndef __CSTATS_H
#define __CSTATS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t (*CStatsGetter)(int categoryid, int valueid, int peerid);
typedef const char * (*CStatsGetter_time)(int categoryid, int valueid, int peerid);
typedef int64_t(*CStatsGetter_common)(int valueid);
typedef int64_t(*CStatsGetter_health)(int categoryid, int valueid, int peerid);

void csSetName(const char *name);
//void csInit(void *logger, CStatsGetter getter, CStatsGetter_time time_getter, int maxvalues, long interval);
void csInit(void *logger, CStatsGetter getter, CStatsGetter_common common_getter,  CStatsGetter_health health_getter, int maxvalues, long interval);
void csStart(void);
void csStop(void);

int csAddUpsecs(long upsecs,long resetsecs);
int csAddInterface(const char *name,const char *pname);  //Interface
int csAddPeer(int categoryid,const char *status,const char *ipaddress); //Peer

void csAddLastactivity(int categoryid, int peerid, const char *lastactivity);

int csAddHealth(int categoryid,int peerid,int64_t resptimeout,int64_t maxtimeouts,int64_t timeouts); //Health
int csAddMessage(int categoryid,int peerid, const char *name,const char *dir); //Messages
//int csAddMessage(int categoryid,int peerid, const char *name,const char *dir, const char *time); //Messages
int csAddActive(long v);
int csGetInterval(char **response);
int csUpdateInterval(const char *json, char **response);
int csGetLive(char **response);

int parse_config_param(const char *param,const char *json,char **response,char *buff);

void construct_json(const char *param,const char *value,const char *effect, char *buf);

void get_current_file_size(size_t len);

#ifdef __cplusplus
}
#endif

#endif /* __CSTATS_H */
