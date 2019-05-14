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

#include "cdnscache.h"
#include "stime.h"
#include "epc.h"
#include "cdnshelper.h"

const CachedDNS::namedserverid_t NS_OPS = 1;
const CachedDNS::namedserverid_t NS_APP = 2;

int rqst = 0;
int resp = 0;
SEvent event;

const ns_type RQST_TYPE = ns_t_naptr;
const char *RQST_DOMAIN = "imsTV2.apn.epc.mnc990.mcc311.3gppnetwork.org";

#define DNS_QUERY_SYNC(__nsid__,__cachehit__,__ignorecache__)              \
{                                                                          \
   CachedDNS::QueryPtr q = CachedDNS::Cache::getInstance(__nsid__)           \
      .query(RQST_TYPE,RQST_DOMAIN,__cachehit__,__ignorecache__);          \
   std::cout <<                                                            \
      (__nsid__==NS_OPS?"NS_OPS":__nsid__==NS_APP?"NS_APP":"UNKNOWN") <<   \
      " " <<                                                               \
      (q->getError() ? "FAILED":"SUCCEEDED") <<                            \
      " cachehit=" << (cachehit?"true":"false") <<                         \
      " ignorecache=" << (__ignorecache__?"true":"false") <<               \
      std::endl;                                                           \
   /* q->dump(); */                                                        \
}

#define DNS_QUERY_ASYNC(__nsid__,__cb__,__ignorecache__,__seq__)           \
{                                                                          \
   char *data = new char[128];                                             \
   sprintf(data, "%s %d",                                                  \
         __nsid__ == NS_OPS ? "NS_OPS" :                                   \
         __nsid__ == NS_APP ? "NS_APP" : "UNKNOWN", __seq__);              \
   CachedDNS::Cache::getInstance(__nsid__)                                 \
         .query(RQST_TYPE,RQST_DOMAIN,__cb__,data,__ignorecache__);        \
}

void dnscb(CachedDNS::QueryPtr q, bool cacheHit, const void *data)
{
   char *msg = (char*)data;
   resp++;
   std::cout
      << msg
      << (q->getError() ? " FAILED":" SUCCEEDED")
      << " cachehit="
      << (cacheHit?"true":"false")
      << " using callback"
      << std::endl;
   if (rqst == resp)
      event.set();

   delete [] msg;
}

class RWLockThread : public SThread
{
public:
   RWLockThread()
      : SThread( false )
   {
   }

   virtual unsigned long threadProc(void *arg)
   {
      std::string str;
      SRwLock *rwlock = (SRwLock*)arg;
      SRdLock rdlck1( *rwlock, false );

      SThread::sleep(1000);

      rdlck1.acquire( false );
      STime::Now().Format( str, "%Y-%m-%d %H:%M:%S.%0", true );
      std::cout << str << " RWLockThread::rdlck1 isLocked=" << (rdlck1.isLocked()?"true":"false") << std::endl;

      SRdLock rdlck2( *rwlock );
      STime::Now().Format( str, "%Y-%m-%d %H:%M:%S.%0", true );
      std::cout << str << " RWLockThread::rdlck2 isLocked=" << (rdlck2.isLocked()?"true":"false") << std::endl;

      return 0;
   }
};

void rwlock()
{
   std::string str;
   RWLockThread t;
   SRwLock rwlock;

   t.init( &rwlock );

   {
      SWrLock wrlck1( rwlock );
      STime::Now().Format( str, "%Y-%m-%d %H:%M:%S.%0", true );
      std::cout << str << " rwlock() wrlck1 isLocked=" << (wrlck1.isLocked()?"true":"false") << std::endl;
      SThread::sleep(3000);
   }

   t.join();
}

void set_dnscache_refresh_params(unsigned int concurrent, int percent,
		long interval)
{
	CachedDNS::Cache::setRefreshConcurrent(concurrent);
	CachedDNS::Cache::setRefreshPercent(percent);
	CachedDNS::Cache::setRefreshInterval(interval);
}

void set_named_server(const char *address, int udp_port, int tcp_port)
{
	CachedDNS::Cache::getInstance(NS_OPS).addNamedServer(address, udp_port, tcp_port);
	CachedDNS::Cache::getInstance(NS_OPS).applyNamedServers();
}

void *init_pgwupf_node_selector(const char *apnoi, const char *mnc, const char *mcc)
{
	EPC::PGWUPFNodeSelector *sel =
			new EPC::PGWUPFNodeSelector(apnoi, mnc, mcc);

	sel->setNamedServerID(NS_APP);

	return sel;
}

void *init_sgwupf_node_selector(const unsigned char lb, const unsigned char hb,
		const char *mnc, const char *mcc)
{
	EPC::SGWUPFNodeSelector *sel =
				new EPC::SGWUPFNodeSelector(lb, hb, mnc, mcc);

	sel->setNamedServerID(NS_APP);

	return sel;
}

void *init_enbupf_node_selector(const char *enb, const char *mnc,
		const char *mcc)
{
	EPC::ENodeBUPFNodeSelector *sel =
				new EPC::ENodeBUPFNodeSelector(enb, mnc, mcc);

	sel->setNamedServerID(NS_APP);

	return sel;
}

void deinit_node_selector(void *node_obj)
{
	delete static_cast<EPC::NodeSelector *> (node_obj);
}

void set_desired_proto(void *node_obj, node_selector_type obj_type,
		enum upf_app_proto_t protocol)
{
	switch (obj_type) {

		case MMENODESELECTOR: {
			break;
		}

		case PGWNODESELECTOR: {
			break;
		}

		case PGWUPFNODESELECTOR: {
			EPC::PGWUPFNodeSelector *sel = static_cast<EPC::PGWUPFNodeSelector *> (node_obj);
			sel->addDesiredProtocol((EPC::UPFAppProtocolEnum)protocol);
			break;
		}

		case SGWUPFNODESELECTOR: {
			EPC::SGWUPFNodeSelector *sel = static_cast<EPC::SGWUPFNodeSelector *> (node_obj);
			sel->addDesiredProtocol((EPC::UPFAppProtocolEnum)protocol);
			break;
		}

		case ENBUPFNODESELECTOR: {
			EPC::ENodeBUPFNodeSelector *sel = static_cast<EPC::ENodeBUPFNodeSelector *> (node_obj);
			sel->addDesiredProtocol((EPC::UPFAppProtocolEnum)protocol);
			break;
		}

		default:
			break;
	}
}

void set_ueusage_type(void *node_obj, int usage_type)
{
	EPC::NodeSelector *sel = static_cast<EPC::NodeSelector *>(node_obj);
	sel->addDesiredUsageType(usage_type);
}

void set_nwcapability(void *node_obj, const char *nc)
{
	EPC::NodeSelector *sel = static_cast<EPC::NodeSelector *>(node_obj);
	sel->addDesiredNetworkCapability(nc);
}

void process_dnsreq(void *node_obj, dns_query_result_t *result,
		uint16_t *res_count)
{
	EPC::NodeSelector *sel = static_cast<EPC::NodeSelector *>(node_obj);
	sel->process();
	//sel->dump();
	sel->get_result(result, res_count);
	//rwlock();
}

int get_colocated_candlist(void *node_obj1, void *node_obj2,
		canonical_result_t *result)
{
	EPC::NodeSelector *sel1  = static_cast<EPC::NodeSelector *>(node_obj1);
	EPC::NodeSelector *sel2  = static_cast<EPC::NodeSelector *>(node_obj2);

	EPC::ColocatedCandidateList ccl(sel1->getResults(), sel2->getResults());
	//ccl.dump();
	return ccl.get_result(result);
}

int get_colocated_candlist_fqdn(char *sgwu_fqdn, void *node_obj2,
		canonical_result_t *result)
{
	EPC::NodeSelector *sel2  = static_cast<EPC::NodeSelector *>(node_obj2);
	std::string fqdn(sgwu_fqdn);
	EPC::ColocatedCandidateList ccl(fqdn, sel2->getResults());
	return ccl.get_result_fqdn(result);
}
