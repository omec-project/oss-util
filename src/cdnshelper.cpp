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

extern "C" void NodeSelector_callback(EPC::NodeSelector &ns, void *user_data)
{
	//ns.dump();
	dns_cb_userdata_t *cb_user_data = (dns_cb_userdata_t *) user_data;
	dns_query_callback callback = cb_user_data->cb;
	void *data = cb_user_data->data;
	void *arg = &ns;
	callback(arg, data, user_data);
}

void set_dnscache_refresh_params(unsigned int concurrent, int percent,
		long interval)
{
	CachedDNS::Cache::setRefreshConcurrent(concurrent);
	CachedDNS::Cache::setRefreshPercent(percent);
	CachedDNS::Cache::setRefreshInterval(interval);
}

void set_dns_retry_params(long timeout, unsigned int retries)
{
	CachedDNS::Cache::setQueryTimeoutMS(timeout);
	CachedDNS::Cache::setQueryTries(retries);
}

void set_nameserver_config(const char *address, int udp_port, int tcp_port,
		nameserver_type_id ns_type)
{
	CachedDNS::Cache::getInstance(ns_type).addNamedServer(address, udp_port, tcp_port);
}

void apply_nameserver_config(nameserver_type_id ns_type)
{
	CachedDNS::Cache::getInstance(ns_type).applyNamedServers();
}

void init_save_dns_queries(nameserver_type_id ns_type,
		const char *qfn, int qsf)
{
	CachedDNS::Cache::getInstance(ns_type).initSaveQueries(qfn, qsf * 1000);
}

int load_dns_queries(nameserver_type_id ns_type, const char *qfn)
{
	try
	{
	  CachedDNS::Cache::getInstance(ns_type).loadQueries(qfn);
	}
	catch(const std::exception& e)
	{
	  return 1;
	}

	return 0;
}

void *init_pgwupf_node_selector(const char *apnoi, const char *mnc, const char *mcc)
{
	EPC::PGWUPFNodeSelector *sel =
			new EPC::PGWUPFNodeSelector(apnoi, mnc, mcc);

	sel->setNamedServerID(NS_OPS);

	sel->setNodeSelType(PGWUPFNODESELECTOR);

	return sel;
}

void *init_sgwupf_node_selector(char *lb, char *hb,
		const char *mnc, const char *mcc)
{
	EPC::SGWUPFNodeSelector *sel =
				new EPC::SGWUPFNodeSelector(lb, hb, mnc, mcc);

	sel->setNamedServerID(NS_OPS);

	sel->setNodeSelType(SGWUPFNODESELECTOR);

	return sel;
}

void *init_enbupf_node_selector(const char *enb, const char *mnc,
		const char *mcc)
{
	EPC::ENodeBUPFNodeSelector *sel =
				new EPC::ENodeBUPFNodeSelector(enb, mnc, mcc);

	sel->setNamedServerID(NS_APP);

	sel->setNodeSelType(ENBUPFNODESELECTOR);

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
}

void process_dnsreq_async(void *node_obj, dns_cb_userdata_t *user_data)
{

	EPC::NodeSelector *sel = static_cast<EPC::NodeSelector *>(node_obj);
	sel->process((void *)user_data, NodeSelector_callback);
}

void get_dns_query_res(void *node_obj, dns_query_result_t *result,
		uint16_t *res_count)
{
	EPC::NodeSelector *sel = static_cast<EPC::NodeSelector *>(node_obj);
	//sel->dump();
	sel->get_result(result, res_count);
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

uint8_t get_node_selector_type(void *node_obj)
{
	EPC::NodeSelector *sel  = static_cast<EPC::NodeSelector *>(node_obj);
	return sel->getNodeSelType();
}
