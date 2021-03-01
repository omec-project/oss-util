/*
 * Copyright (c) 2019 Sprint
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <time.h>
#include <netinet/ip.h>

#include "sthread.h"
#include "slogger.h"
#include "stime.h"
#include "gw_adapter.h"
#include "cstats.h"
#include "clogger.h"

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define RAPIDJSON_NAMESPACE statsrapidjson
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "cstats_dev.hpp"

long oss_resetsec;
cp_configuration_t *cp_config_ptr;
dp_configuration_t *dp_config_ptr;

const uint16_t CSTAT_UPDATE_INTERVAL = ETM_USER + 1;
const uint16_t CSTAT_GENERATE_CSV = ETM_USER + 2;

class CStatsUpdateInterval : public SEventThreadMessage
{
	public:
		CStatsUpdateInterval(long interval)
			: SEventThreadMessage(CSTAT_UPDATE_INTERVAL), m_interval(interval)
		{
		}

	long getInterval() { return m_interval; }

private:
	long m_interval;
};

class CStatsGenerateCSV : public SEventThreadMessage
{
public:
	CStatsGenerateCSV()
		: SEventThreadMessage(CSTAT_GENERATE_CSV)
	{
	}
};

class CStats : public SEventThread
{
public:
	enum CStatsSerializationEngine
	{
		csseUndefined,
		csseJson,
		csseCSV
	};

	CStats();
	~CStats();

	static CStats &singleton() { if (!m_singleton) m_singleton = new CStats(); return *m_singleton; }
	static void init(SLogger* logger) { singleton().setLogger(logger); }

	void onInit();
	void onQuit();
	void onTimer( SEventThread::Timer &t );
	void dispatch( SEventThreadMessage &msg );

	void updateInterval(long interval);
	void setInterval(long interval)			{ m_interval = interval; }
	long getInterval()				{ return m_interval; }

	void setLogger(SLogger *logger)			{ m_logger = logger; }

    	void setStatLoggingSuppress(bool suppress)      { statLoggingSuppress = suppress;}
    	bool getStatLoggingSuppress()                   { return statLoggingSuppress;}
	void serializeJSON(std::string &json,bool suppressed);

private:
	static CStats *m_singleton;

	long m_interval;
	SLogger *m_logger;
	SEventThread::Timer m_timer;
    	bool statLoggingSuppress;
};

int len = 0;
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void csInit(void *logger, long interval)
{
	CStats::singleton().setLogger((SLogger*)logger);
	CStats::singleton().setInterval(interval);
    	CStats::singleton().setStatLoggingSuppress(true);
}

void csStart(void)
{
	((SEventThread&)CStats::singleton()).init(NULL);
}

void csStop(void)
{
	CStats::singleton().quit();
	CStats::singleton().join();
}

int csGetInterval(char **response)
{
	std::string res = "{\"statfreq\": " + std::to_string(CStats::singleton().getInterval()) + "}";
	*response = strdup(res.c_str());
	return REST_SUCESSS;
}

int get_number_of_request_tries(char **response, int request_tries)
{
	std::string res = "{\"request_tries\": " + std::to_string(request_tries) + "}";
	*response = strdup(res.c_str());
	return REST_SUCESSS;
}

int get_pcap_generation_status(char **response, uint8_t pcap_gen_status)
{
	std::string res;
	if ((pcap_gen_status == START_PCAP_GEN) || (pcap_gen_status == RESTART_PCAP_GEN))
		res = "{\"PCAP_GENERATIN\": \"START\"}";
	else if (pcap_gen_status == STOP_PCAP_GEN)
                res = "{\"PCAP_GENERATIN\": \"STOP\"}";

	*response = strdup(res.c_str());
	return REST_SUCESSS;
}

int get_number_of_transmit_count(char **response, int transmit_count)
{
	std::string res = "{\"transmit_count\": " + std::to_string(transmit_count) + "}";
	*response = strdup(res.c_str());
	return REST_SUCESSS;
}

int get_transmit_timer_value(char **response, int transmit_timer_value)
{
	std::string res = "{\"transmit_timer\": " + std::to_string(transmit_timer_value) + "}";
	*response = strdup(res.c_str());
	return REST_SUCESSS;
}

int get_periodic_timer_value(char **response, int periodic_timer_value)
{
	std::string res = "{\"periodic_timer\": " + std::to_string(periodic_timer_value) + "}";
	*response = strdup(res.c_str());
	return REST_SUCESSS;
}

int get_request_timeout_value(char **response, int request_timeout_value)
{
	std::string res = "{\"request_timeout\": " + std::to_string(request_timeout_value) + "}";
	*response = strdup(res.c_str());
	return REST_SUCESSS;
}

int csGetStatLogging(char **response)
{
    std::string res;
    if(CStats::singleton().getStatLoggingSuppress())
        res = "{\"statlog\": \"suppress\"}";
    else
        res = "{\"statlog\": \"all\"}";
	*response = strdup(res.c_str());
	return REST_SUCESSS;
}

int csUpdateStatLogging(const char *json, char **response)
{
	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}
	if(!doc.HasMember("statlog") || !doc["statlog"].IsString())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}

    string StatLoggingStr = doc["statlog"].GetString();

    if(StatLoggingStr == "suppress")
        CStats::singleton().setStatLoggingSuppress(true);
    else if(StatLoggingStr == "all")
        CStats::singleton().setStatLoggingSuppress(false);
    else{
            if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
            return REST_FAIL;
    }

	if (response)
		*response = strdup("{\"result\": \"OK\"}");

	return REST_SUCESSS;
}


int csUpdateInterval(const char *json, char **response)
{
	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}
	if(!doc.HasMember("statfreq") || !doc["statfreq"].IsUint64())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}

	unsigned long statfreq = doc["statfreq"].GetUint64();
	CStats::singleton().updateInterval(statfreq);

	if (response)
		*response = strdup("{\"result\": \"OK\"}");

	return REST_SUCESSS;
}

int get_request_tries_value(const char *json, char **response)
{

	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}
	if(!doc.HasMember("request_tries") || !doc["request_tries"].IsUint64())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}
	unsigned long request_tries = doc["request_tries"].GetUint64();
	return request_tries;
}

int get_transmit_count_value(const char *json, char **response)
{

	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}
	if(!doc.HasMember("transmit_count") || !doc["transmit_count"].IsUint64())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}

	unsigned long transmit_count = doc["transmit_count"].GetUint64();
	return transmit_count;
}

int get_request_timeout_value_in_milliseconds(const char *json, char **response)
{

	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}
	if(!doc.HasMember("request_timeout") || !doc["request_timeout"].IsUint64())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}

	unsigned long request_timeout_value = doc["request_timeout"].GetUint64();
	return request_timeout_value;
}

int get_periodic_timer_value_in_seconds(const char *json, char **response)
{

	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}
	if(!doc.HasMember("periodic_timer") || !doc["periodic_timer"].IsUint64())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}

	unsigned long periodic_timer_value = doc["periodic_timer"].GetUint64();
	return periodic_timer_value;
}

int get_transmit_timer_value_in_seconds(const char *json, char **response)
{

	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}
	if(!doc.HasMember("transmit_timer") || !doc["transmit_timer"].IsUint64())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}

	unsigned long transmit_timer_value = doc["transmit_timer"].GetUint64();
	return transmit_timer_value;
}

int get_pcap_generation_cmd_value(const char *json, char **response)
{
	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}
	if(!doc.HasMember("generate_pcap") || !doc["generate_pcap"].IsString())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}

	std::string pcap_generation_cmd  = doc["generate_pcap"].GetString();

	if ((pcap_generation_cmd == "start") || (pcap_generation_cmd == "START"))
		return START_PCAP_GEN;
	else if ((pcap_generation_cmd == "stop") || (pcap_generation_cmd == "STOP"))
		return STOP_PCAP_GEN;
	else if ((pcap_generation_cmd == "restart") || (pcap_generation_cmd == "RESTART"))
		return RESTART_PCAP_GEN;

	return REST_FAIL;
}

void  construct_json(const char *param,const char *value, char *buf)
{
	const char *ret;
	statsrapidjson::Document document;
	document.SetObject();
	statsrapidjson::Document::AllocatorType &allocator = document.GetAllocator();

	document.AddMember("parameter",statsrapidjson::StringRef(param),allocator);
	document.AddMember("set_value",statsrapidjson::StringRef(value),allocator);

	statsrapidjson::StringBuffer strbuf;
	statsrapidjson::Writer<statsrapidjson::StringBuffer> writer(strbuf);
	document.Accept(writer);

	ret=strbuf.GetString();
	strcpy(buf,ret);

}

int  resp_cmd_not_supported(uint8_t gw_type, char **response)
{
	char temp[JSON_RESP_SIZE];
	strncpy(temp, "{ \"ERROR\": \"NOT SUPPORTED COMMAND ON ", ENTRY_NAME_SIZE);
	strncat(temp, ossGatewayStr[gw_type], ENTRY_VALUE_SIZE);
	strncat(temp, "\"}", 3);
	*response = strdup(temp);
	return REST_FAIL;
}

int csGetLive(char **response)
{
	std::string live;
	CStats::singleton().serializeJSON(live,true);
	*response = strdup(live.c_str());
	return REST_SUCESSS;
}

int get_cp_configuration(char **response, cp_configuration_t *cp_config_ptr)
{

	if(cp_config_ptr == NULL)
	{
		if (response)
			*response = strdup("{\"result\": \"system failure\"}");
		return REST_FAIL;
	}

	std::string json;
	statsrapidjson::Document document;
	document.SetObject();
	statsrapidjson::Document::AllocatorType& allocator = document.GetAllocator();
	statsrapidjson::Value valArray(statsrapidjson::kArrayType);

	document.AddMember("Gateway Type", statsrapidjson::StringRef(ossGatewayStr[cp_config_ptr->cp_type]), allocator);

	statsrapidjson::Value s11(statsrapidjson::kObjectType);
	s11.AddMember("S11 MME IP", statsrapidjson::Value(inet_ntoa(cp_config_ptr->s11_mme_ip), allocator).Move(), allocator);
	s11.AddMember("S11 MME Port", cp_config_ptr->s11_mme_port, allocator);
	s11.AddMember("S11 SGW IP", statsrapidjson::Value(inet_ntoa(cp_config_ptr->s11_ip), allocator).Move(), allocator);
	s11.AddMember("S11 SGW Port", cp_config_ptr->s11_port, allocator);
	document.AddMember("S11 Interface", s11, allocator);

	statsrapidjson::Value s5s8(statsrapidjson::kObjectType);
	s5s8.AddMember("S5S8 IP", statsrapidjson::Value(inet_ntoa(cp_config_ptr->s5s8_ip), allocator).Move(), allocator);
	s5s8.AddMember("S5S8 Port", cp_config_ptr->s5s8_port, allocator);
	document.AddMember("S5S8 Interface", s5s8, allocator);

	statsrapidjson::Value sx(statsrapidjson::kObjectType);
	sx.AddMember("PFCP IP", statsrapidjson::Value(inet_ntoa(cp_config_ptr->pfcp_ip), allocator).Move(), allocator);
	sx.AddMember("PFCP Port", cp_config_ptr->pfcp_port, allocator);
	sx.AddMember("UPF PFCP IP", statsrapidjson::Value(inet_ntoa(cp_config_ptr->upf_pfcp_ip), allocator).Move(), allocator);
	sx.AddMember("UPF PFCP Port", cp_config_ptr->upf_pfcp_port, allocator);
	document.AddMember("SX Interface", sx, allocator);

	statsrapidjson::Value li(statsrapidjson::kObjectType);
	li.AddMember("DADMF IP", statsrapidjson::Value(inet_ntoa(cp_config_ptr->dadmf_ip), allocator).Move(), allocator);
	li.AddMember("DADMF Port", cp_config_ptr->dadmf_port, allocator);
	li.AddMember("DDF2 IP", statsrapidjson::Value(inet_ntoa(cp_config_ptr->ddf2_ip), allocator).Move(), allocator);
	li.AddMember("DDF2 Port", cp_config_ptr->ddf2_port, allocator);
	li.AddMember(statsrapidjson::StringRef("DDF2 Interface"),
			statsrapidjson::StringRef(cp_config_ptr->ddf2_intfc), allocator);

	document.AddMember("LI Interface", li, allocator);

	statsrapidjson::Value urr_default(statsrapidjson::kObjectType);
	urr_default.AddMember("Trigger Type", cp_config_ptr->trigger_type, allocator);
	urr_default.AddMember("Uplink Volume Thresold", cp_config_ptr->uplink_volume_th, allocator);
	urr_default.AddMember("Downlink Volume Thresold", cp_config_ptr->downlink_volume_th, allocator);
	urr_default.AddMember("Time Thresold", cp_config_ptr->time_th, allocator);
	document.AddMember("URR DEFAULT PARAMETERS", urr_default, allocator);

	document.AddMember("Generate CDR", cp_config_ptr->generate_cdr, allocator);
	if(cp_config_ptr->is_gx_interface) {
		document.AddMember("USE GX", cp_config_ptr->use_gx, allocator);
	} else {
		document.AddMember("USE GX", statsrapidjson::StringRef("Not Applicable On SGWC"), allocator);
	}
	document.AddMember("Generate SGW CDR", cp_config_ptr->generate_sgw_cdr, allocator);
	if(cp_config_ptr->generate_sgw_cdr == SGW_CHARGING_CHARACTERISTICS) {
		document.AddMember("SGW Charging Characteristics", cp_config_ptr->sgw_cc, allocator);
	} else {
		 document.AddMember("SGW Charging Characteristics",
				 statsrapidjson::StringRef("SGW Charging Characteristics Flag Is Disable In cp.cfg"), allocator);
	}
	document.AddMember("UPF S5S8 IP",
					statsrapidjson::Value(inet_ntoa(*((struct in_addr *)&cp_config_ptr->upf_s5s8_ip)), allocator).Move(), allocator);
	document.AddMember("UPF S5S8 Mask",
					statsrapidjson::Value(inet_ntoa(*((struct in_addr *)&cp_config_ptr->upf_s5s8_mask)), allocator).Move(), allocator);

	statsrapidjson::Value redis_server(statsrapidjson::kObjectType);
	redis_server.AddMember("Redis Server IP", statsrapidjson::Value(inet_ntoa(cp_config_ptr->redis_ip),
				allocator).Move(), allocator);
	redis_server.AddMember("Redis Server Port", cp_config_ptr->redis_port, allocator);
	redis_server.AddMember("CP Redis IP", statsrapidjson::Value(inet_ntoa(cp_config_ptr->cp_redis_ip),
				allocator).Move(), allocator);
	redis_server.AddMember(statsrapidjson::StringRef("Redis Cert Path"),
			statsrapidjson::StringRef(cp_config_ptr->redis_cert_path), allocator);
	document.AddMember("Redis Server Info", redis_server, allocator);

	statsrapidjson::Value restoration(statsrapidjson::kObjectType);
	restoration.AddMember("Periodic Timer", cp_config_ptr->restoration_params.periodic_timer, allocator);
	restoration.AddMember("Transmit Timer", cp_config_ptr->restoration_params.transmit_timer, allocator);
	restoration.AddMember("Transmit Count", cp_config_ptr->restoration_params.transmit_cnt, allocator);
	document.AddMember("Restoration Parameters", restoration, allocator);

	document.AddMember("Request Timeout", cp_config_ptr->request_timeout, allocator);
	document.AddMember("Request Tries", cp_config_ptr->request_tries, allocator);
	document.AddMember("Add Default Rule", cp_config_ptr->add_default_rule, allocator);

	statsrapidjson::Value ip_pool(statsrapidjson::kObjectType);
	ip_pool.AddMember("IP Pool IP", statsrapidjson::Value(inet_ntoa(cp_config_ptr->ip_pool_ip), allocator).Move(), allocator);
	ip_pool.AddMember("IP Pool Mask", statsrapidjson::Value(inet_ntoa(cp_config_ptr->ip_pool_mask), allocator).Move(), allocator);
	document.AddMember("IP Pool Config", ip_pool, allocator);

	document.AddMember("Use DNS", cp_config_ptr->use_dns, allocator);
	document.AddMember("CP Logger", cp_config_ptr->cp_logger, allocator);

	statsrapidjson::Value apnList(statsrapidjson::kArrayType);

	for(int itr_apn = 0; itr_apn < cp_config_ptr->num_apn; itr_apn++)
	{
		statsrapidjson::Value value(statsrapidjson::kObjectType);
		value.AddMember(statsrapidjson::StringRef("APN Name Label"),
				statsrapidjson::StringRef(cp_config_ptr->apn_list[itr_apn].apn_name_label), allocator);
		value.AddMember(statsrapidjson::StringRef("APN Usage Type"), cp_config_ptr->apn_list[itr_apn].apn_usage_type, allocator);
		value.AddMember(statsrapidjson::StringRef("APN Network Capability"),
				statsrapidjson::StringRef(cp_config_ptr->apn_list[itr_apn].apn_net_cap), allocator);
		value.AddMember(statsrapidjson::StringRef("Trigger Type"), cp_config_ptr->apn_list[itr_apn].trigger_type, allocator);
		value.AddMember(statsrapidjson::StringRef("Uplink Volume Threshold"),
				cp_config_ptr->apn_list[itr_apn].uplink_volume_th, allocator);
		value.AddMember(statsrapidjson::StringRef("Downlink Volume Threshold"),
				cp_config_ptr->apn_list[itr_apn].downlink_volume_th, allocator);
		value.AddMember(statsrapidjson::StringRef("Time Threshold"),
				cp_config_ptr->apn_list[itr_apn].time_th, allocator);
		apnList.PushBack(value, allocator);

	}

	document.AddMember("APN List", apnList, allocator);

	statsrapidjson::Value dns_cache(statsrapidjson::kObjectType);
	dns_cache.AddMember(statsrapidjson::StringRef("Concurrent"), cp_config_ptr->dns_cache.concurrent, allocator);
	dns_cache.AddMember(statsrapidjson::StringRef("Seconds"), cp_config_ptr->dns_cache.sec, allocator);
	dns_cache.AddMember(statsrapidjson::StringRef("Percentage"), cp_config_ptr->dns_cache.percent, allocator);
	dns_cache.AddMember(statsrapidjson::StringRef("Timeouts MilliSeconds"), cp_config_ptr->dns_cache.timeoutms, allocator);
	dns_cache.AddMember(statsrapidjson::StringRef("Query Tries"), cp_config_ptr->dns_cache.tries, allocator);

	statsrapidjson::Value app_dns(statsrapidjson::kObjectType);
	app_dns.AddMember(statsrapidjson::StringRef("Frequency Seconds"), cp_config_ptr->app_dns.freq_sec, allocator);
	app_dns.AddMember("Filename", statsrapidjson::StringRef(cp_config_ptr->app_dns.filename), allocator);
	app_dns.AddMember("Nameserver",
		statsrapidjson::StringRef(cp_config_ptr->app_dns.nameserver_ip[cp_config_ptr->app_dns.nameserver_cnt-1]), allocator);

	statsrapidjson::Value ops_dns(statsrapidjson::kObjectType);
	ops_dns.AddMember(statsrapidjson::StringRef("Frequency Seconds"), cp_config_ptr->ops_dns.freq_sec, allocator);
	ops_dns.AddMember("Filename", statsrapidjson::StringRef(cp_config_ptr->ops_dns.filename), allocator);
	ops_dns.AddMember("Nameserver",
		statsrapidjson::StringRef(cp_config_ptr->ops_dns.nameserver_ip[cp_config_ptr->ops_dns.nameserver_cnt-1]), allocator);

	document.AddMember("DNS CACHE", dns_cache, allocator);
	document.AddMember("DNS APP", app_dns, allocator);
	document.AddMember("DNS OPS", ops_dns, allocator);

	statsrapidjson::StringBuffer strbuf;
	statsrapidjson::Writer<statsrapidjson::StringBuffer> writer(strbuf);
	document.Accept(writer);
	json = strbuf.GetString();
	*response = strdup(json.c_str());

	return REST_SUCESSS;
}

int get_dp_configuration(char **response, dp_configuration_t *dp_config_ptr)
{
	if(dp_config_ptr == NULL)
	{
		if (response)
			*response = strdup("{\"result\": \"system failure\"}");
		return REST_FAIL;
	}

	std::string json;
	statsrapidjson::Document document;
	document.SetObject();
	statsrapidjson::Document::AllocatorType& allocator = document.GetAllocator();

	document.AddMember("Gateway Type", statsrapidjson::StringRef(ossGatewayStr[dp_config_ptr->dp_type]), allocator);

	statsrapidjson::Value sx(statsrapidjson::kObjectType);
	sx.AddMember("CP Common IP", statsrapidjson::Value(inet_ntoa(dp_config_ptr->cp_comm_ip), allocator).Move(), allocator);
	sx.AddMember("CP Common Port", dp_config_ptr->cp_comm_port, allocator);
	sx.AddMember("DP Common IP", statsrapidjson::Value(inet_ntoa(dp_config_ptr->dp_comm_ip), allocator).Move(), allocator);
	sx.AddMember("DP Common Port", dp_config_ptr->dp_comm_port, allocator);
	document.AddMember("SX Interface", sx, allocator);

	statsrapidjson::Value teid(statsrapidjson::kObjectType);
	teid.AddMember("Teidri Timeout", dp_config_ptr->teidri_timeout, allocator);
	teid.AddMember("Teidri Value", dp_config_ptr->teidri_val, allocator);
	document.AddMember("Teid Parameters", teid, allocator);

	statsrapidjson::Value restoration(statsrapidjson::kObjectType);
	restoration.AddMember("Periodic Timer", dp_config_ptr->restoration_params.periodic_timer, allocator);
	restoration.AddMember("Transmit Timer", dp_config_ptr->restoration_params.transmit_timer, allocator);
	restoration.AddMember("Transmit Count", dp_config_ptr->restoration_params.transmit_cnt, allocator);
	document.AddMember("Restoration Parameters", restoration, allocator);

	document.AddMember("Generate Pcap", dp_config_ptr->generate_pcap, allocator);
	document.AddMember("DP Logger", dp_config_ptr->dp_logger, allocator);
	document.AddMember("Numa", dp_config_ptr->numa_on, allocator);
	document.AddMember("GTPU Sequence Number In", dp_config_ptr->gtpu_seqnb_in, allocator);
	document.AddMember("GTPU Sequence Number Out", dp_config_ptr->gtpu_seqnb_out, allocator);

	statsrapidjson::Value west(statsrapidjson::kObjectType);
	west.AddMember("West Kni Interface", statsrapidjson::StringRef(dp_config_ptr->wb_iface_name), allocator);
	west.AddMember("West Bound IP", statsrapidjson::Value(inet_ntoa(*((struct in_addr *)&dp_config_ptr->wb_ip)),
				allocator).Move(), allocator);
	west.AddMember("West Bound Mask", statsrapidjson::Value(inet_ntoa(*((struct in_addr *)&dp_config_ptr->wb_mask)),
				allocator).Move(), allocator);
	west.AddMember("West Bound Mac", statsrapidjson::StringRef(dp_config_ptr->wb_mac), allocator);
	document.AddMember("West User Plane", west, allocator);

	statsrapidjson::Value east(statsrapidjson::kObjectType);
	east.AddMember("East Kni Interface", statsrapidjson::StringRef(dp_config_ptr->eb_iface_name), allocator);
	east.AddMember("East Bound IP", statsrapidjson::Value(inet_ntoa(*((struct in_addr *)&dp_config_ptr->eb_ip)),
				allocator).Move(), allocator);
	east.AddMember("East Bound Mask", statsrapidjson::Value(inet_ntoa(*((struct in_addr *)&dp_config_ptr->eb_mask)),
				allocator).Move(), allocator);
	east.AddMember("East Bound Mac", statsrapidjson::StringRef(dp_config_ptr->eb_mac), allocator);
	document.AddMember("East User Plane", east, allocator);

	statsrapidjson::Value li(statsrapidjson::kObjectType);
	li.AddMember("DDF2 IP",
		statsrapidjson::Value(inet_ntoa(*((struct in_addr *)&dp_config_ptr->ddf2_ip)), allocator).Move(), allocator);
	li.AddMember("DDF2 Port", dp_config_ptr->ddf2_port, allocator);
	li.AddMember(statsrapidjson::StringRef("DDF2 Interface"),
			statsrapidjson::StringRef(dp_config_ptr->ddf2_intfc), allocator);
	li.AddMember("DDF3 IP",
		statsrapidjson::Value(inet_ntoa(*((struct in_addr *)&dp_config_ptr->ddf3_ip)), allocator).Move(), allocator);
	li.AddMember("DDF3 Port", dp_config_ptr->ddf3_port, allocator);
	li.AddMember(statsrapidjson::StringRef("DDF3 Interface"),
			statsrapidjson::StringRef(dp_config_ptr->ddf3_intfc), allocator);
	li.AddMember("West Bound LI IP",
		statsrapidjson::Value(inet_ntoa(*((struct in_addr *)&dp_config_ptr->wb_li_ip)), allocator).Move(), allocator);
	li.AddMember("West Bound LI Mask",
			statsrapidjson::Value(inet_ntoa(*((struct in_addr *)&dp_config_ptr->wb_li_mask)), allocator).Move(), allocator);
	li.AddMember(statsrapidjson::StringRef("West Bound LI Interface"),
			statsrapidjson::StringRef(dp_config_ptr->wb_li_iface_name), allocator);
	document.AddMember("LI Interface", li, allocator);

	statsrapidjson::StringBuffer strbuf;
	statsrapidjson::Writer<statsrapidjson::StringBuffer> writer(strbuf);
	document.Accept(writer);
	json = strbuf.GetString();
	*response = strdup(json.c_str());
	return REST_SUCESSS;
}

int csGetLiveAll(char **response)
{
       std::string live;
       CStats::singleton().serializeJSON(live,false);
       *response = strdup(live.c_str());
       return REST_SUCESSS;
}

int csResetStats(const char *json, char **response)
{

	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return REST_FAIL;
	}

	if (response)
		*response = strdup("{\"result\": \"OK\"}");
	 return REST_SUCESSS;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CStats *CStats::m_singleton = NULL;

CStats::CStats()
	: m_logger(NULL)
{
}

CStats::~CStats()
{
}

void CStats::onInit()
{
	m_timer.setInterval(m_interval);
	m_timer.setOneShot(false);
	initTimer(m_timer);
	m_timer.start();
}

void CStats::onQuit()
{
}

void CStats::onTimer(SEventThread::Timer &t)
{
	postMessage(new CStatsGenerateCSV());
}

void CStats::dispatch(SEventThreadMessage &msg)
{
	switch (msg.getId())
	{
		case CSTAT_UPDATE_INTERVAL:
		{
			m_timer.stop();
			m_interval = ((CStatsUpdateInterval&)msg).getInterval();
			m_timer.setInterval(m_interval);
			m_timer.start();
			break;
		}
		case CSTAT_GENERATE_CSV:
		{
			std::string json;
			serializeJSON(json,statLoggingSuppress);
            m_logger->info(json);                    //after 5 sec write
            m_logger->flush();                       //to the file
			break;
		}
	}
}

void CStats::updateInterval(long interval)
{
	CStatsUpdateInterval *e = new CStatsUpdateInterval(interval);
	postMessage(e);
}

void CStats::serializeJSON(std::string &json,bool suppressed=true)
{
    string data;
    statsrapidjson::Document document;
    document.SetObject();
    statsrapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    statsrapidjson::Value valArray(statsrapidjson::kArrayType);

    CStatGateway gateway(suppressed);
    gateway.serialize(cli_node_ptr, document, valArray, allocator);

    statsrapidjson::StringBuffer strbuf;
    statsrapidjson::Writer<statsrapidjson::StringBuffer> writer(strbuf);
    document.Accept(writer);
    json = strbuf.GetString();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

typedef long long int _timer_t;

#define TIMER_GET_CURRENT_TP(now)                                             \
({                                                                            \
 struct timespec ts;                                                          \
 now = clock_gettime(CLOCK_REALTIME,&ts) ?                                    \
    -1 : (((_timer_t)ts.tv_sec) * 1000000000) + ((_timer_t)ts.tv_nsec);   \
 now;                                                                         \
 })

#define TIMER_GET_ELAPSED_NS(start)                                           \
({                                                                            \
 _timer_t ns;                                                                 \
 TIMER_GET_CURRENT_TP(ns);                                                    \
 if (ns != -1){                                                               \
    ns -= start;                                                          \
 }                                        \
 ns;                                                                          \
 })


extern _timer_t st_time;

