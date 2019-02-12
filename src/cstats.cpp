#include <stdlib.h>

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "sthread.h"
#include "slogger.h"
#include "stime.h"
#include "cstats.h"

#define RAPIDJSON_NAMESPACE statsrapidjson
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

class CStatValue
{
public:
	CStatValue(const char *name, int id)
		: m_name(name), m_id(id), m_value(0)
	{
	}

	const std::string &getName() { return m_name; }
	int getId()			{ return m_id; }

	int64_t setValue(int64_t v)	{ return m_value = v; }
	int64_t getValue()		{ return m_value; }

private:
	CStatValue();

	std::string m_name;
	int m_id;
	int64_t m_value;
};

class CStatCategory
{
public:
	CStatCategory(const char *name, int id)
		: m_name(name), m_id(id)
	{
	}

	~CStatCategory()
	{
		while (!m_values.empty())
		{
			CStatValue *v = m_values.back();
			m_values.pop_back();
			delete v;
		}
	}

	const std::string &getName()	{ return m_name; }
	int getId()			{ return m_id; }

	int addValue(const char *name)
	{
		CStatValue *v = new CStatValue(name, m_values.size());
		m_values.push_back(v);
		return v->getId();
	}

	std::vector<CStatValue*> &getValues() { return m_values; }

	void serialize(statsrapidjson::Value &arrayObjects, statsrapidjson::Document::AllocatorType &allocator);
	void serialize(std::stringstream &ss);

private:
	std::string m_name;
	int m_id;
	std::vector<CStatValue*> m_values;
};

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

	void setStatGetter(CStatsGetter getstat)	{ m_getstat = getstat; }

	int getMaxValues()				{ return m_maxvalues; }
	int setMaxValues(int v)				{ return m_maxvalues = v; }

	void getCurrentStats();

	int addCategory(const char *name);
	int addValue(int categoryid, const char *name);

	void serializeJSON(std::string &json);
	void serializeCSV();

	void dump();
private:
	static CStats *m_singleton;

	
	long m_interval;
	SLogger *m_logger;
	CStatsGetter m_getstat;
	int m_maxvalues;
	std::vector<CStatCategory*> m_categories;
	SEventThread::Timer m_timer;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void csInit(void *logger, CStatsGetter getter, int maxvalues, long interval)
{
	CStats::singleton().setLogger((SLogger*)logger);
	CStats::singleton().setStatGetter(getter);
	CStats::singleton().setMaxValues(maxvalues);
	CStats::singleton().setInterval(interval);
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

int csAddCategory(const char *name)
{
	return CStats::singleton().addCategory(name);
}

int csAddValue(int categoryid, const char *name)
{
	return CStats::singleton().addValue(categoryid, name);
}

int csGetInterval(char **response)
{
	std::string res = "{\"statfreq\": " + std::to_string(CStats::singleton().getInterval()) + "}";
	*response = strdup(res.c_str());
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
	CStats::singleton().serializeJSON(live);
	*response = strdup(live.c_str());
	return 200;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CStats *CStats::m_singleton = NULL;

CStats::CStats()
	: m_logger(NULL), m_getstat(NULL), m_maxvalues(0)
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
			getCurrentStats();
			serializeCSV();
	
			//std::string json;
			//serializeJSON(json);
			break;
		}
	}
}

int CStats::addCategory(const char *name)
{
	CStatCategory *c = new CStatCategory(name, m_categories.size());
	m_categories.push_back(c);
	return c->getId();
}

int CStats::addValue(int categoryid, const char *name)
{
	if (categoryid < 0 && categoryid >= m_categories.size())
		return -1;

	return m_categories[categoryid]->addValue(name);
}

void CStats::updateInterval(long interval)
{
	CStatsUpdateInterval *e = new CStatsUpdateInterval(interval);
	postMessage(e);
} 

void CStats::getCurrentStats()
{
	if (!m_getstat)
		return;

	for (auto cit = m_categories.begin(); cit != m_categories.end(); ++cit)
	{
		for (auto vit = (*cit)->getValues().begin(); vit != (*cit)->getValues().end(); ++vit)
		{
			(*vit)->setValue( (*m_getstat)((*cit)->getId(), (*vit)->getId()) );
		}
	}
}

void CStats::serializeJSON(std::string &json)
{
	STime now = STime::Now();
	std::string nowstr;
	statsrapidjson::Document document;
	document.SetObject();
	statsrapidjson::Document::AllocatorType &allocator = document.GetAllocator();

        now.Format(nowstr, "%Y-%m-%dT%H:%M:%S.%0", false);
        document.AddMember("time_utc", statsrapidjson::StringRef(nowstr.c_str()), allocator);

	statsrapidjson::Value arrayValues(statsrapidjson::kArrayType);

	for (auto cit = m_categories.begin(); cit != m_categories.end(); ++cit)
	{
		(*cit)->serialize(arrayValues, allocator);
	}

	document.AddMember("statistics", arrayValues, allocator);

	statsrapidjson::StringBuffer strbuf;
	statsrapidjson::Writer<statsrapidjson::StringBuffer> writer(strbuf);
	document.Accept(writer);

	json = strbuf.GetString();
}

void CStats::serializeCSV()
{
	std::stringstream ss;
	STime now = STime::Now();
	std::string nowstr;
	std::string str;

        now.Format(nowstr, "%Y-%m-%dT%H:%M:%S.%0", false);

	for (auto cit = m_categories.begin(); cit != m_categories.end(); ++cit)
	{
		ss.str("");
		ss << "\"" << nowstr << "\",\"" << (*cit)->getName() << "\"";
		(*cit)->serialize(ss);
		m_logger->info(ss.str());
	}

	m_logger->flush();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void CStatCategory::serialize(statsrapidjson::Value &arrayValues, statsrapidjson::Document::AllocatorType &allocator)
{
	STime now = STime::Now();
	std::string nowstr;
	statsrapidjson::Value row(statsrapidjson::kObjectType);
	statsrapidjson::Value values(statsrapidjson::kArrayType);

	row.AddMember(statsrapidjson::StringRef("category"), statsrapidjson::StringRef(getName().c_str()), allocator);

	for (auto vit = getValues().begin(); vit != getValues().end(); ++vit)
	{
		statsrapidjson::Value value(statsrapidjson::kObjectType);
		value.AddMember(statsrapidjson::StringRef("name"), statsrapidjson::StringRef((*vit)->getName().c_str()), allocator);
		value.AddMember(statsrapidjson::StringRef("value"), (*vit)->getValue(), allocator);

		values.PushBack(value, allocator);
	}

	row.AddMember(statsrapidjson::StringRef("values"), values, allocator);

	arrayValues.PushBack(row, allocator);
}

void CStatCategory::serialize(std::stringstream &ss)
{
	int cnt = 0;

	for (auto vit = getValues().begin(); vit != getValues().end(); ++vit)
	{
		ss << "," << (*vit)->getValue();
		cnt++;
	}

	for (; cnt < CStats::singleton().getMaxValues(); cnt++)
		ss << ",";
}
