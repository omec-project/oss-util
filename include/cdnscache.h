/*
* Copyright (c) 2017 Sprint
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef __CDNSCACHE_H
#define __CDNSCACHE_H

#include <list>
#include <map>
#include <ares.h>

#include "cdnsquery.h"
#include "satomic.h"
#include "ssync.h"
#include "sthread.h"

namespace CachedDNS
{
   typedef int namedserverid_t;

   class Cache;
   class QueryProcessor;

   const namedserverid_t NS_DEFAULT = 0;

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////
   
   class QueryProcessorThread : public SThread
   {
      friend QueryProcessor;

   public:
      QueryProcessorThread(QueryProcessor &qp);

      void incActiveQueries() { m_activequeries.increment(); }
      void decActiveQueries() { m_activequeries.decrement(); }
      int getActiveQueries()  { return m_activequeries.getCurrentCount(); }

      virtual unsigned long threadProc(void *arg);

      void shutdown();

   protected:
      static void ares_callback( void *arg, int status, int timeouts, unsigned char *abuf, int alen );

   private:
      QueryProcessorThread();
      void wait_for_completion();

      bool m_shutdown;
      QueryProcessor &m_qp;
      SSemaphore m_activequeries;
   };

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////

   struct NamedServer
   {
      char address[128];
      int family;
      int udp_port;
      int tcp_port;
   };

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////

   class QueryProcessor
   {
      friend Cache;
      friend QueryProcessorThread;
   public:

      QueryProcessor( Cache &cache );
      ~QueryProcessor();

      Cache &getCache() { return m_cache; }

      void shutdown();

      QueryProcessorThread *getQueryProcessorThread() { return &m_qpt; }

      void addNamedServer(const char *address, int udp_port, int tcp_port);
      void removeNamedServer(const char *address);
      void applyNamedServers();

      SMutex &getChannelMutex() { return m_mutex; }

   protected:
      ares_channel getChannel() { return m_channel; }

      void beginQuery( QueryPtr &q );
      void endQuery();

   private:
      QueryProcessor();
      void init();

      Cache &m_cache;
      QueryProcessorThread m_qpt;
      ares_channel m_channel;
      std::map<const char *,NamedServer> m_servers;
      SMutex m_mutex;
   };

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////

   #define SAVED_QUERY_TYPE "type"
   #define SAVED_QUERY_DOMAIN "domain"

   const uint16_t CR_SAVEQUERIES = ETM_USER + 1;
   const uint16_t CR_FORCEREFRESH = ETM_USER + 2;

   class CacheRefresher : SEventThread
   {
      friend Cache;

   protected:
      CacheRefresher(Cache &cache, unsigned int maxconcur, int percent, long interval);

      virtual void onInit();
      virtual void onQuit();
      virtual void onTimer( SEventThread::Timer &timer );
      virtual void dispatch( SEventThreadMessage &msg );

      const std::string &queryFileName() { return m_qfn; }
      long querySaveFrequency() { return m_qsf; }

      void loadQueries(const char *qfn);
      void loadQueries(const std::string &qfn) { loadQueries(qfn.c_str()); }
      void initSaveQueries(const char *qfn, long qsf);
      void saveQueries() { postMessage(CR_SAVEQUERIES); }
      void forceRefresh() { postMessage(CR_FORCEREFRESH); }

   private:
      CacheRefresher();
      static void callback( QueryPtr q, bool cacheHit, const void *data );
      void _submitQueries( std::list<QueryCacheKey> &keys );
      void _refreshQueries();
      void _saveQueries();
      void _forceRefresh();

      Cache &m_cache;
      SSemaphore m_sem;
      int m_percent;
      SEventThread::Timer m_timer;
      long m_interval;
      bool m_running;
      std::string m_qfn;
      long m_qsf;
      SEventThread::Timer m_qst;
   };

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////

   class Cache
   {
      friend QueryProcessor;
      friend QueryProcessorThread;

      friend CacheRefresher;

   public:
      Cache();
      ~Cache();

      static Cache& getInstance(namedserverid_t nsid);
      static Cache& getInstance() { return getInstance(NS_DEFAULT); }

      static unsigned int getRefreshConcurrent() { return m_concur; }
      static unsigned int setRefreshConcurrent(unsigned int concur) { return m_concur = concur; }

      static int getRefreshPercent() { return m_percent; }
      static int setRefreshPercent(int percent) { return m_percent = percent; }

      static long getRefeshInterval() { return m_interval; }
      static long setRefreshInterval(long interval) { return m_interval = interval; }

      void addNamedServer(const char *address, int udp_port=53, int tcp_port=53);
      void removeNamedServer(const char *address);
      void applyNamedServers();

      QueryPtr query( ns_type rtype, const std::string &domain, bool &cacheHit, bool ignorecache=false );
      void query( ns_type rtype, const std::string &domain, CachedDNSQueryCallback cb, const void *data=NULL, bool ignorecache=false );

      void loadQueries(const char *qfn);
      void loadQueries(const std::string &qfn) { loadQueries(qfn.c_str()); }
      void initSaveQueries(const char *qfn, long qsf);
      void saveQueries();
      void forceRefresh();

      namedserverid_t getNamedServerId() { return m_nsid; }

      long resetNewQueryCount() { return atomic_swap(m_newquerycnt, 0); }

   protected:
      void updateCache( QueryPtr q );
      QueryPtr lookupQuery( ns_type rtype, const std::string &domain );
      QueryPtr lookupQuery( QueryCacheKey &qck );

      void identifyExpired( std::list<QueryCacheKey> &keys, int percent );
      void getCacheKeys( std::list<QueryCacheKey> &keys );


   private:

      static int m_ref;
      static unsigned int m_concur;
      static int m_percent;
      static long m_interval;

      QueryProcessor m_qp;
      CacheRefresher m_refresher;
      QueryCache m_cache;
      namedserverid_t m_nsid;
      SRwLock m_cacherwlock;
      long m_newquerycnt;
   };
}

#endif // #ifndef __CDNSCACHE_H
