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

#include "../include/cdnscache.h"

#include <stdarg.h>
#include <stdio.h>
#include <memory.h>
#include <poll.h>

#include <iostream>
#include <list>

#include "ssync.h"
#include "../include/serror.h"
#include "../include/sthread.h"
#include "cdnsparser.h"

using namespace CachedDNS;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template<class T>
class Buffer
{
public:
   Buffer(size_t size) { msize = size; mbuf = new T[msize]; }
   ~Buffer() { if (mbuf) delete [] mbuf; }
   T *get() { return mbuf; }
private:
   Buffer();
   size_t msize;
   T *mbuf;
};

static std::string string_format( const char *format, ... )
{
   va_list args;

   va_start( args, format );
   size_t size = vsnprintf( NULL, 0, format, args ) + 1; // Extra space for '\0'
   va_end( args );

   Buffer<char> buf( size );

   va_start( args, format );
   vsnprintf( buf.get(), size, format, args  );
   va_end( args );

   return std::string( buf.get(), size - 1 ); // We don't want the '\0' inside
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace CachedDNS
{
   QueryProcessorThread::QueryProcessorThread(QueryProcessor &qp)
      : m_shutdown( false ),
        m_qp(qp),
        m_activequeries(0)
   {
   }

   unsigned long QueryProcessorThread::threadProc(void *arg)
   {
      while ( true )
      {
         int qcnt = m_activequeries.getCurrentCount();

         if ( qcnt == 0 )
         {
            // wait for a new query to be submitted or for shutdown
            decActiveQueries();

            if ( m_shutdown && getActiveQueries() == 0 )
               break;

            incActiveQueries();
         }
         else if ( m_shutdown && qcnt == 1 )
         {
            break;
         }

         wait_for_completion();
      }

      return 0;
   }

   void QueryProcessorThread::wait_for_completion()
   {
      struct timeval tv;
      int timeout;
      ares_socket_t sockets[ ARES_GETSOCK_MAXNUM ];
      struct pollfd fds[ ARES_GETSOCK_MAXNUM ];

      while( true )
      {
         int rwbits = ares_getsock( m_qp.getChannel(), sockets, ARES_GETSOCK_MAXNUM );
         int fdcnt = 0;

         memset( fds, 0, sizeof(fds) );

         for ( int i = 0; i < ARES_GETSOCK_MAXNUM; i++ )
         {
            fds[fdcnt].fd = sockets[i];
            fds[fdcnt].events |= ARES_GETSOCK_READABLE(rwbits,i) ? (POLLIN | POLLRDNORM) : 0;
            fds[fdcnt].events |= ARES_GETSOCK_WRITABLE(rwbits,i) ? (POLLOUT | POLLWRNORM) : 0;
            if ( fds[fdcnt].events != 0 )
               fdcnt++;
         }

         if ( !fdcnt )
            break;

         memset( &tv, 0, sizeof(tv) );
         ares_timeout( m_qp.getChannel(), NULL, &tv );
         timeout = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

         if ( poll(fds,fdcnt,timeout) != 0 )
         {
            for ( int i = 0; i < fdcnt; i++ )
            {
               if ( fds[i].revents != 0 )
               {
                  ares_process_fd( m_qp.getChannel(),
                     fds[i].revents & (POLLIN | POLLRDNORM) ? fds[i].fd : ARES_SOCKET_BAD,
                     fds[i].revents & (POLLOUT | POLLWRNORM) ? fds[i].fd : ARES_SOCKET_BAD);
               }
            }
         }
         else
         {
            // timeout
            ares_process_fd( m_qp.getChannel(), ARES_SOCKET_BAD, ARES_SOCKET_BAD );
         }
      }
   }

   void QueryProcessorThread::shutdown()
   {
      m_shutdown = true;
      incActiveQueries();
      join();
   }

   void QueryProcessorThread::ares_callback( void *arg, int status, int timeouts, unsigned char *abuf, int alen )
   {
      QueryPtr *qq = reinterpret_cast<QueryPtr*>(arg);

      QueryProcessor *qp = (*qq)->getQueryProcessor();

      if (qp)
      {
         qp->endQuery();

         try
         {
            Parser p( *qq, abuf, alen );
            p.parse();
         }
         catch (std::exception &ex)
         {
            (*qq)->setError( true );
            (*qq)->setErrorMsg( ex.what() );
         }

         if ( !(*qq)->getError() )
         {
            qp->getCache().updateCache( *qq );
         }

         if ( (*qq)->getCompletionEvent() )
            (*qq)->getCompletionEvent()->set();

         if ( (*qq)->getCallback() )
         {
            const void *data = (*qq)->getData();
            (*qq)->setData(NULL);
            (*qq)->getCallback()( *qq, false, data );
         }
      }

      delete qq;
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////

   QueryProcessor::QueryProcessor( Cache &cache )
      : m_cache( cache ),
        m_qpt( *this )
   {
      m_channel = NULL;

      init();

      m_qpt.init( NULL );
   }

   QueryProcessor::~QueryProcessor()
   {
      ares_destroy( m_channel );
   }

   void QueryProcessor::init()
   {
      int status;
      if ( (status = ares_init(&m_channel)) == ARES_SUCCESS )
      {
         struct ares_options opt;
         opt.timeout = 1000;
         opt.ndots = 0;
         opt.flags = ARES_FLAG_EDNS;
         opt.ednspsz = 8192;
         ares_init_options( &m_channel, &opt, ARES_OPT_TIMEOUTMS | ARES_OPT_NDOTS | ARES_OPT_EDNSPSZ | ARES_OPT_FLAGS );
      }
      else
      {
         std::string msg( string_format("QueryProcessor::init() - ares_init() failed status = %d", status) );
         SError::throwRuntimeException( msg );
      }
   }

   void QueryProcessor::shutdown()
   {
      m_qpt.shutdown();
   }

   void QueryProcessor::addNamedServer(const char *address, int udp_port, int tcp_port)
   {
      union {
        struct in_addr       addr4;
        struct ares_in6_addr addr6;
      } addr;
      struct NamedServer ns;

      if (ares_inet_pton(AF_INET, address, &addr.addr4) == 1)
         ns.family = AF_INET;
      else if (ares_inet_pton(AF_INET6, address, &addr.addr6) == 1)
         ns.family = AF_INET6;
      else
      {
         std::string msg( string_format("QueryProcessor::addNamedServer() - unrecognized address [%s]", address) );
         SError::throwRuntimeException( msg );
      }

      ares_inet_ntop( ns.family, &addr, ns.address, sizeof(ns.address) );
      ns.udp_port = udp_port;
      ns.tcp_port = tcp_port;

      m_servers[ns.address] = ns;
   }

   void QueryProcessor::removeNamedServer(const char *address)
   {
      m_servers.erase( address );
   }

   void QueryProcessor::applyNamedServers()
   {
      ares_addr_port_node *head = NULL;

      // create linked list of named servers
      for (const auto &kv : m_servers)
      {
         ares_addr_port_node *p = new ares_addr_port_node();

         p->next = head;
         p->family = kv.second.family;
         ares_inet_pton( p->family, kv.second.address, &p->addr );
         p->udp_port = kv.second.udp_port;
         p->tcp_port = kv.second.tcp_port;

         head = p;
      }

      // apply the named server list
      int status = ares_set_servers_ports( m_channel, head );

      // delete the list of named servers
      while (head)
      {
         ares_addr_port_node *p = head;
         head = head->next;
         delete p;
      }
      // throw exception if needed
      if ( status != ARES_SUCCESS )
      {
         std::string msg( string_format("QueryProcessor::updateNamedServers() - ares_set_servers_ports() failed status = %d", status) );
         SError::throwRuntimeException( msg );
      }
   }

   void QueryProcessor::beginQuery( QueryPtr &q )
   {
      m_qpt.incActiveQueries();
      q->setQueryProcessor( this );
      q->setError( false );

      QueryPtr *qq = new QueryPtr(q);

      if ( q->getCallback() || q->getCompletionEvent() )
      {
         ares_query( m_channel, q->getDomain().c_str(), ns_c_in, q->getType(), QueryProcessorThread::ares_callback, qq );
      }
      else
      {
         SEvent event;
         q->setCompletionEvent( &event );
         ares_query( m_channel, q->getDomain().c_str(), ns_c_in, q->getType(), QueryProcessorThread::ares_callback, qq );
         event.wait();
         q->setCompletionEvent( NULL );
      }
   }

   void QueryProcessor::endQuery()
   {
      m_qpt.decActiveQueries();
   }

   /////////////////////////////////////////////////////////////////////////////
   /////////////////////////////////////////////////////////////////////////////

   int Cache::m_ref = 0;
   unsigned int Cache::m_concur = 10;
   int Cache::m_percent = 80;
   long Cache::m_interval = 60;

   Cache::Cache()
      : m_qp( *this ),
        m_refresher( *this, m_concur, m_percent, m_interval )
   {
      if (m_ref == 0)
      {
         int status = ares_library_init( ARES_LIB_INIT_ALL );
         if ( status != ARES_SUCCESS )
         {
            std::string msg( string_format("Cache::Cache() - ares_library_init() failed status = %d", status) );
            SError::throwRuntimeException( msg );
         }
      }

      m_ref++;
      m_nsid = NS_DEFAULT;

      // start the refresh thread
      m_refresher.init(NULL);
   }

   Cache::~Cache()
   {
      // stop the refresh thread
      m_refresher.quit();
      m_refresher.join();

      // stop the query processor
      m_qp.shutdown();

      m_ref--;

      if (m_ref == 0)
         ares_library_cleanup();
   }

   class CacheMap
   {
   public:
      ~CacheMap()
      {
         std::map<namedserverid_t, Cache*>::iterator it;

         while ((it = m_map.begin()) != m_map.end())
         {
            Cache *c = it->second;
            m_map.erase( it );
            delete c;
         }
      }

      std::map<namedserverid_t, Cache*> &getMap() { return m_map; }

   private:
      std::map<namedserverid_t, Cache*> m_map;
   };

   Cache& Cache::getInstance(namedserverid_t nsid)
   {
      static CacheMap cm;
      Cache *c;

      auto search = cm.getMap().find(nsid);
      if (search == cm.getMap().end())
      {
         c = new Cache();
         c->m_nsid = nsid;
         cm.getMap()[nsid] = c;
      }
      else
      {
         c = search->second;
      }

      return *c;
   }

   void Cache::addNamedServer(const char *address, int udp_port, int tcp_port)
   {
      m_qp.addNamedServer(address, udp_port, tcp_port);
   }

   void Cache::removeNamedServer(const char *address)
   {
      m_qp.removeNamedServer(address);
   }

   void Cache::applyNamedServers()
   {
      m_qp.applyNamedServers();
   }

   QueryPtr Cache::query( ns_type rtype, const std::string & domain, bool &cacheHit, bool ignorecache )
   {
      QueryPtr q = lookupQuery( rtype, domain );

      cacheHit = !( !q || q->isExpired() );

      if ( !cacheHit || ignorecache ) // query not found or expired
      {
         q.reset( new Query( rtype, domain ) );
         m_qp.beginQuery( q );
         if (ignorecache)
            cacheHit = false;
      }

      return q;
   }

   void Cache::query( ns_type rtype, const std::string &domain, CachedDNSQueryCallback cb, const void *data, bool ignorecache )
   {
      QueryPtr q = lookupQuery( rtype, domain );

      bool cacheHit = !( !q || q->isExpired() );

      if ( cacheHit && !ignorecache )
      {
         if ( cb )
            cb( q, cacheHit, data );
      }
      else
      {
         q.reset( new Query( rtype, domain ) );
         q->setCallback( cb );
         q->setData( data );
         m_qp.beginQuery( q );
      }
   }

   QueryPtr Cache::lookupQuery( ns_type rtype, const std::string &domain )
   {
      QueryCacheKey qck( rtype, domain );
      return lookupQuery( qck );
   }

   QueryPtr Cache::lookupQuery( QueryCacheKey &qck )
   {
      SRdLock l( m_cacherwlock );
      QueryCache::const_iterator it = m_cache.find( qck );
      return it != m_cache.end() ? it->second : QueryPtr();
   }

   void Cache::updateCache( QueryPtr q )
   {
      if ( !q )
         return;

      if ( !q->getError() )
      {
         QueryCacheKey qck( q->getType(), q->getDomain() );
         SWrLock l( m_cacherwlock );
         m_cache[qck] = q;
      }
   }

   void Cache::identifyExpired( std::list<QueryCacheKey> &keys, int percent )
   {
      SRdLock l( m_cacherwlock );

      for (auto val : m_cache)
      {
         QueryPtr q = val.second;
         if ( q )
         {
            if ( !q->isExpired() )
            {
               time_t diff = (q->getTTL() - (q->getExpires() - time(NULL))) * 100;
               int pcnt = diff / q->getTTL();
               if ( pcnt < percent )
                  continue;
            }
            keys.push_back( val.first );
         }
      }
   }

   ////////////////////////////////////////////////////////////////////////////////
   ////////////////////////////////////////////////////////////////////////////////

   CacheRefresher::CacheRefresher(Cache &cache, unsigned int maxconcur, int percent, long interval)
      : m_cache( cache ),
        m_sem( maxconcur ),
        m_percent( percent ),
        m_interval( interval ),
        m_running( false )
   {
   }

   void CacheRefresher::onInit()
   {
      m_timer.setInterval( m_interval );
      m_timer.setOneShot( false );
      initTimer( m_timer );
      m_timer.start();
   }

   void CacheRefresher::onQuit()
   {
      m_timer.stop();
   }

   void CacheRefresher::onTimer( SEventThread::Timer &timer)
   {
      if (m_running)
         return;
      m_running = true;

      std::list<QueryCacheKey> keys;

      m_cache.identifyExpired( keys, m_percent );

      for (auto qck : keys)
      {
         m_sem.decrement();

         m_cache.query( qck.getType(), qck.getDomain(), callback, this, true );
      }

      m_running = false;
   }

   void CacheRefresher::dispatch( SEventThreadMessage &msg )
   {
   }

   void CacheRefresher::callback( QueryPtr q, bool cacheHit, const void *data )
   {
      CacheRefresher *ths = (CacheRefresher*)data;
      ths->m_sem.increment();
   }
} // namespace CachedDNS




