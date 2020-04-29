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

#define RAPIDJSON_NAMESPACE statsrapidjson
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "cstats_dev.hpp"

long oss_resetsec;

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
	return 200;
}


int csGetStatLogging(char **response)
{
    std::string res;
    if(CStats::singleton().getStatLoggingSuppress())
        res = "{\"statlog\": \"suppress\"}";
    else
        res = "{\"statlog\": \"all\"}";
	*response = strdup(res.c_str());
	return 200;
}

int csUpdateStatLogging(const char *json, char **response)
{
	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return 400;
	}
	if(!doc.HasMember("statlog") || !doc["statlog"].IsString())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return 400;
	}

    string StatLoggingStr = doc["statlog"].GetString();

    if(StatLoggingStr == "suppress")
        CStats::singleton().setStatLoggingSuppress(true);
    else if(StatLoggingStr == "all")
        CStats::singleton().setStatLoggingSuppress(false);
    else{
            if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
            return 400;
    }

	if (response)
		*response = strdup("{\"result\": \"OK\"}");

	return 200;
}


int csUpdateInterval(const char *json, char **response)
{
	statsrapidjson::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return 400;
	}
	if(!doc.HasMember("statfreq") || !doc["statfreq"].IsUint64())
	{
		if (response)
			*response = strdup("{\"result\": \"ERROR\"}");
		return 400;
	}

	unsigned long statfreq = doc["statfreq"].GetUint64();
	CStats::singleton().updateInterval(statfreq);

	if (response)
		*response = strdup("{\"result\": \"OK\"}");

	return 200;
}

int csGetLive(char **response)
{
	std::string live;
    get_current_file_size(len);
	CStats::singleton().serializeJSON(live,true);
	*response = strdup(live.c_str());
	return 200;
}

int csGetLiveAll(char **response)
{
       std::string live;
	   get_current_file_size(len);
       CStats::singleton().serializeJSON(live,false);
       *response = strdup(live.c_str());
       return 200;
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
			get_current_file_size(len);
			serializeJSON(json,statLoggingSuppress);
			len=strlen(json.c_str());
            get_current_file_size(len);
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

void
get_current_file_size(size_t len)
{

	struct stat st;
	stat("logs/cp_stat.log",&st);
	size_t size = st.st_size;
	size_t max_size = optStatMaxSize * 1024 * 1024 ;
	static int flag=0;
	static _timer_t reset_time = 0;
	_timer_t cp_stats_reset_time;

	if( len > (max_size-size) )         //reset log will be take place
	{
		reset_time = *(cli_node_ptr->upsecs);
	}
	cp_stats_reset_time = *(cli_node_ptr->upsecs) - reset_time;
	oss_resetsec = cp_stats_reset_time;
}

