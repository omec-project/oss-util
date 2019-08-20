#include <stdlib.h>

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <time.h>


#include "sthread.h"
#include "slogger.h"
#include "stime.h"
#include "cstats.h"
#include "clogger.h"

#include <sys/stat.h>

//include "../../cp_adapter.h"

#define RAPIDJSON_NAMESPACE statsrapidjson
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


//extern void get_current_file_size(size_t len);

class CStatValue
{
	public:
		CStatValue(const char *name, const char *dir,int id)
			: m_name(name), m_dir(dir),m_id(id), m_value(0)
		{
		}

		const std::string &getName() { return m_name; }
		const std::string &getDname() { return m_dir; }
		int getId()			{ return m_id; }

		int64_t setValue(int64_t v)	{ return m_value = v; }
		int64_t getValue()		{ return m_value; }

		void setTime(const char *str)	{  m_time = str; }
		const std::string &getTime()    { return m_time; }

	private:
		CStatValue();

		std::string m_name;
		std::string m_dir;
		std::string m_time;
		int m_id;
		int64_t m_value;
};

class CStatHealth
{
	public:
		CStatHealth(int64_t resptimeout,int64_t maxtimeouts,int64_t timeouts,int64_t id)
			:m_resptimeout(resptimeout),m_maxtimeouts(maxtimeouts),m_timeouts(timeouts),m_id(id)
		{
		}

		int64_t setRespTimeout(int64_t v)  { return m_resptimeout = v; }
		int64_t setMaxTimeouts(int64_t v)  { return m_maxtimeouts = v; }
		int64_t setTimeouts(int64_t v)     { return m_timeouts = v; }

		int64_t setReqSent(int64_t v)       { return m_req_sent = v; }
		int64_t setReqReceived(int64_t v)   { return m_req_received = v; }
		int64_t setRespSent(int64_t v)       { return m_resp_sent = v; }
		int64_t setRespReceived(int64_t v)   { return m_resp_received = v; }

		int64_t getRespTimeout()            { return m_resptimeout; }
		int64_t getMaxTimeouts()            { return m_maxtimeouts; }
		int64_t getTimeouts()               { return m_timeouts;    }

		int64_t getReqSent()                { return m_req_sent; }
		int64_t getReqReceived()            { return m_req_received; }
		int64_t getRespSent()                { return m_resp_sent; }
		int64_t getRespReceived()            { return m_resp_received; }
		int64_t getId()                     { return m_id; }

		void serialize(statsrapidjson::Value &row, statsrapidjson::Value &arrayObjects, statsrapidjson::Document::AllocatorType &allocator);

	private:
		CStatHealth();

		int64_t m_resptimeout;
		int64_t m_maxtimeouts;
		int64_t m_timeouts;
		int64_t m_req_sent;
		int64_t m_req_received;
		int64_t m_resp_sent;
		int64_t m_resp_received;
		int64_t m_id;

};



class CStatPeer
{
	public:
		CStatPeer(const char *active,const char *address,int id)
			:m_active(active),m_address(address),m_id(id)
		{
		}

		const std::string &getAddress() { return m_address; }
		const std::string &getActive()  { return m_active; }

		void setLastactivity(const char *lastactivity)  { m_lastactivity = lastactivity; }
		void setActive(const char *active)   {m_active = active; }

		void setStatus(int st)
		{
			if (st == 1)
				setActive("true");
			else
				setActive("false");
		}

		const std::string &getLastactivity() { return m_lastactivity; }

		int getId() {return m_id;}

		int addMvalue(const char *name,const char *dir)
		{
			CStatValue *v = new CStatValue(name,dir, m_values.size());
			m_values.push_back(v);
			return v->getId();
		}

		int addHvalue(int64_t resptimeout,int64_t maxtimeouts,int64_t timeouts)
		{
			CStatHealth *v= new CStatHealth(resptimeout,maxtimeouts,timeouts,m_health.size());
			m_health.push_back(v);
			return v->getId();
		}


		std::vector<CStatValue*> &getValues() { return m_values; }
		void serialize(statsrapidjson::Value &row, statsrapidjson::Value &arrayObjects, statsrapidjson::Document::AllocatorType &allocator);

		std::vector<CStatHealth*> &getHValues() {return m_health;}

	private:
		CStatPeer();

		std::string m_active;
		std::string m_address;
		std::string m_lastactivity;
		std::vector<CStatHealth*> m_health;
		int m_id;
		std::vector<CStatValue*> m_values;
};

class CStatCategory
{
public:
	CStatCategory(const char *name,const char *pname, int id)
		: m_name(name),m_pname(pname), m_id(id)
	{
	}

	~CStatCategory()
	{
		while (!m_peers.empty())
		{
			CStatPeer *v = m_peers.back();
			m_peers.pop_back();
			delete v;
		}
	}

	const std::string &getName()	{ return m_name; }
	const std::string &getPName()   { return m_pname; }
	int getId()			{ return m_id; }


	int addPeer(const char *active,const char *address)
	{
		CStatPeer *v = new CStatPeer(active,address, m_peers.size());
		m_peers.push_back(v);
		return v->getId();
	}

	int addValue(int peerid,const char *name,const char *dir)
	{
		return m_peers[peerid]->addMvalue(name,dir);

	}

	int addHealth(int peerid,int64_t resptimeout,int64_t maxtimeouts,int64_t timeouts)
	{
		return m_peers[peerid]->addHvalue(resptimeout,maxtimeouts,timeouts);
	}

	void addLastactivity(int peerid,const char *lastactivity)
	{
		m_peers[peerid]->setLastactivity(lastactivity);
	}


	std::vector<CStatPeer*> &getValues() { return m_peers; }

	void serialize(statsrapidjson::Value &row,statsrapidjson::Value &arrayObjects, statsrapidjson::Document::AllocatorType &allocator);
	void serialize(std::stringstream &ss);

private:
	std::string m_name;
	std::string m_pname;
	int m_id;
	std::vector<CStatPeer*> m_peers;
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
	void setStatGetter_time(CStatsGetter_time  getstat)	{ m_getstat_time = getstat; }
	void setStatGetter_common(CStatsGetter_common getstat)        { common_getstat = getstat; }
	void setStatGetter_health(CStatsGetter_health getstat)        { health_getstat = getstat; }

	const std::string &getName()			{ return m_name; }
	void setName(const char *str)			{  m_name = str; }

	int getMaxValues()				{ return m_maxvalues; }
	int setMaxValues(int v)				{ return m_maxvalues = v; }

	int setUpsecs(long v)                          { return m_upsecs = v; }
	long getUpsecs()                               { return m_upsecs; }

	int setResetsecs(long v)                       { return m_resetsecs = v; }
	long getResetsecs()                            { return m_resetsecs; }

	int setActive(long v)                           { return m_active = v; }
	long getActive()                                { return m_active;}

	void getCurrentStats();

	int addCategory(const char *name,const char *pname);
	int addValue(int categoryid,int peerid, const char *name,const char *dir);
	int addPeer(int categoryid,const char *status,const char *ipaddress);
	int addHealth(int categoryid,int peerid,int64_t resptimeout,int64_t maxtimeouts,int64_t timeouts);
	void addLastactivity(int categoryid,int peerid,const char *lastactivity);



	void serializeJSON(std::string &json);
	void serializeCSV();

	void dump();
private:
	static CStats *m_singleton;

	std::string m_name;
	long m_upsecs;
	long m_resetsecs;
	long m_interval;
	long m_active;
	SLogger *m_logger;
	CStatsGetter m_getstat;
	CStatsGetter_time m_getstat_time;
	CStatsGetter_common common_getstat;
	CStatsGetter_health health_getstat;
	int m_maxvalues;
	std::vector<CStatCategory*> m_categories;
	SEventThread::Timer m_timer;
};


extern struct in_addr dp_comm_ip;
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void csSetName(const char *name)
{
   CStats::singleton().setName(name);
}

//void csInit(void *logger, CStatsGetter getter, CStatsGetter_time time_getter, int maxvalues, long interval)
void csInit(void *logger, CStatsGetter getter, CStatsGetter_common common_getter, CStatsGetter_health health_getter, int maxvalues, long interval)
{
	CStats::singleton().setLogger((SLogger*)logger);
	CStats::singleton().setStatGetter(getter);
	CStats::singleton().setStatGetter_common(common_getter);
	CStats::singleton().setStatGetter_health(health_getter);
	//CStats::singleton().setStatGetter_time(time_getter);   //add
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

int csAddUpsecs(long upsecs,long resetsecs)
{
    CStats::singleton().setResetsecs(resetsecs);
   return  CStats::singleton().setUpsecs(upsecs);
}

int csAddInterface(const char *name,const char *pname)
{
	return CStats::singleton().addCategory(name,pname);
}

int csAddMessage(int categoryid,int peerid, const char *name,const char *dir)
{
	return CStats::singleton().addValue(categoryid,peerid,name,dir);
}

int csAddPeer(int categoryid, const char *status,const char *ipaddress)
{
        return CStats::singleton().addPeer(categoryid,status,ipaddress);
}

void csAddLastactivity(int categoryid,int peerid, const char *lastactivity)
{
       CStats::singleton().addLastactivity(categoryid,peerid,lastactivity);
}


int csAddHealth(int categoryid,int peerid,int64_t resptimeout,int64_t maxtimeouts,int64_t timeouts)
{
        return CStats::singleton().addHealth(categoryid,peerid,resptimeout,maxtimeouts,timeouts);
}

int csAddActive(long v)
{
   return CStats::singleton().setActive(v);
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
			//serializeCSV();

			std::string json;
			serializeJSON(json);
			break;
		}
	}
}

int CStats::addCategory(const char *name,const char *pname)
{
	CStatCategory *c = new CStatCategory(name,pname, m_categories.size());
	m_categories.push_back(c);
	return c->getId();
}

int CStats::addPeer(int categoryid,const char *status,const char *ipaddress)
{
        return m_categories[categoryid]->addPeer(status,ipaddress);
}

void CStats::addLastactivity(int categoryid,int peerid,const char *lastactivity)
{
    m_categories[categoryid]->addLastactivity(peerid,lastactivity);
}

int CStats::addHealth(int categoryid,int peerid,int64_t resptimeout,int64_t maxtimeouts,int64_t timeouts)
{
        return m_categories[categoryid]->addHealth(peerid,resptimeout,maxtimeouts,timeouts);
}

int CStats::addValue(int categoryid,int peerid,const char *name,const char *dir)
{
	if (categoryid < 0 && categoryid >= m_categories.size())
		return -1;

         return	 m_categories[categoryid]->addValue(peerid,name,dir);
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

	for (auto cit = m_categories.begin(); cit != m_categories.end(); ++cit)    //class CStat
	{
		for( auto pit = (*cit)->getValues().begin(); pit != (*cit)->getValues().end(); ++pit)
		{
			//int id = 1;
			     //printf("cid:%d , 0 , pit:%d \n\n",(*cit)->getId(),(*pit)->getId());
			    (*pit)->setStatus( (*health_getstat)((*cit)->getId(),0,(*pit)->getId()));
			for (auto vit = (*pit)->getValues().begin(); vit != (*pit)->getValues().end(); ++vit)     //class CStatPeer
			{
				(*vit)->setValue( (*m_getstat)((*cit)->getId(), (*vit)->getId(), (*pit)->getId()) );
				//(*vit)->setTime( (*m_getstat_time)(0, (*vit)->getId(), 0) );    //get_time
			}

			for (auto vit = (*pit)->getHValues().begin(); vit != (*pit)->getHValues().end(); ++vit)     //class CStatPeer
			{
				(*vit)->setTimeouts( (*health_getstat)((*cit)->getId(), 1, (*pit)->getId()) );
				(*vit)->setReqSent( (*health_getstat)((*cit)->getId(), 2, (*pit)->getId()) );
				(*vit)->setReqReceived( (*health_getstat)((*cit)->getId(), 3, (*pit)->getId()));
				(*vit)->setRespSent( (*health_getstat)((*cit)->getId(), 4, (*pit)->getId()) );
				(*vit)->setRespReceived( (*health_getstat)((*cit)->getId(), 5, (*pit)->getId()));
			}
		}
	}

	CStats::singleton().setActive( (*common_getstat)(0));
	CStats::singleton().setUpsecs((*common_getstat)(1));
	//CStats::singleton().setResetsecs((*m_getstat)(-1,2,0));   //resetsec
	int len = 965;
	get_current_file_size(len);
}

void CStats::serializeJSON(std::string &json)
{
	STime now = STime::Now();
	std::string nowstr;
	//std::string nodename = "sgwc-001";
	//std::string nodename = CStats::singleton().getName();
	statsrapidjson::Document document;
	document.SetObject();
	statsrapidjson::Document::AllocatorType &allocator = document.GetAllocator();
	statsrapidjson::Value row5(statsrapidjson::kObjectType);


        statsrapidjson::Value values8(statsrapidjson::kArrayType);


        //document.AddMember("nodename:",statsrapidjson::StringRef(nodename.c_str()),allocator);
        document.AddMember("nodename:",statsrapidjson::StringRef((CStats::singleton().getName()).c_str()),allocator);

        now.Format(nowstr, "%Y-%m-%dT%H:%M:%S.%0", false);
        document.AddMember("reporttime", statsrapidjson::StringRef(nowstr.c_str()), allocator);
        document.AddMember("upsecs", CStats::singleton().getUpsecs(), allocator);
        document.AddMember("resetsecs", CStats::singleton().getResetsecs(), allocator);

	statsrapidjson::Value arrayValues(statsrapidjson::kArrayType);

	for (auto cit = m_categories.begin(); cit != m_categories.end(); ++cit)
	{
	        statsrapidjson::Value row(statsrapidjson::kObjectType);
	        //statsrapidjson::Value row(statsrapidjson::kArrayType);
		(*cit)->serialize(row,arrayValues, allocator);
                //values8.PushBack(row,allocator);
	}

        //arrayValues.PushBack(values8, allocator);
	document.AddMember("interfaces:", arrayValues, allocator);
	//document.AddMember("interfaces:", arrayValues, allocator);

	row5.AddMember(statsrapidjson::StringRef("active"),CStats::singleton().getActive(), allocator);
	document.AddMember("Sessions:",row5, allocator);



	statsrapidjson::StringBuffer strbuf;
	statsrapidjson::Writer<statsrapidjson::StringBuffer> writer(strbuf);
	document.Accept(writer);

	json = strbuf.GetString();
	m_logger->info(json);
	m_logger->flush();
	int len=strlen(json.c_str());
	//printf("Hello nilesh...!!!!!\n");
        //get_current_file_size(len);
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


void CStatCategory::serialize(statsrapidjson::Value &row,statsrapidjson::Value &arrayValues, statsrapidjson::Document::AllocatorType &allocator)
{
	STime now = STime::Now();
	std::string nowstr;
	statsrapidjson::Value values(statsrapidjson::kArrayType);

	statsrapidjson::Value values8(statsrapidjson::kArrayType);

	row.AddMember(statsrapidjson::StringRef("interfaces-name"), statsrapidjson::StringRef(getName().c_str()), allocator);
	row.AddMember(statsrapidjson::StringRef("protocol-name"), statsrapidjson::StringRef(getPName().c_str()), allocator);

             for (auto vit = getValues().begin(); vit != getValues().end(); ++vit)       //class CStatPeer
                  {
		  	statsrapidjson::Value row1(statsrapidjson::kObjectType);
                        (*vit)->serialize(row1,arrayValues,allocator);
			row.AddMember(statsrapidjson::StringRef("peer"), row1, allocator);
			//values8.PushBack(row1,allocator);
			//row.AddMember(statsrapidjson::StringRef("peer"), values8, allocator);
                 }

	//arrayValues.PushBack(values8, allocator);
	//row.AddMember(statsrapidjson::StringRef("peer"), row1, allocator);
	arrayValues.PushBack(row, allocator);

}

void CStatPeer::serialize(statsrapidjson::Value &row,statsrapidjson::Value &arrayValues, statsrapidjson::Document::AllocatorType &allocator)
{

        statsrapidjson::Value value(statsrapidjson::kObjectType);
        statsrapidjson::Value value1(statsrapidjson::kObjectType);
        statsrapidjson::Value value2(statsrapidjson::kObjectType);
        statsrapidjson::Value value3(statsrapidjson::kObjectType);
        statsrapidjson::Value value4(statsrapidjson::kObjectType);

        statsrapidjson::Value values(statsrapidjson::kArrayType);
        statsrapidjson::Value values1(statsrapidjson::kArrayType);
        statsrapidjson::Value values2(statsrapidjson::kArrayType);
        statsrapidjson::Value values3(statsrapidjson::kArrayType);

	statsrapidjson::Value row1(statsrapidjson::kObjectType);
	statsrapidjson::Value row2(statsrapidjson::kObjectType);
	statsrapidjson::Value row3(statsrapidjson::kObjectType);
	statsrapidjson::Value row4(statsrapidjson::kObjectType);

	value4.AddMember(statsrapidjson::StringRef("ipv4"), statsrapidjson::StringRef(getAddress().c_str()), allocator);
	row.AddMember(statsrapidjson::StringRef("Address"), value4, allocator); //1

	row.AddMember(statsrapidjson::StringRef("active"), statsrapidjson::StringRef(getActive().c_str()), allocator); //2,3
	row.AddMember(statsrapidjson::StringRef("lastactivity"), statsrapidjson::StringRef(getLastactivity().c_str()), allocator);

        auto pit = getHValues().begin();

		   row1.AddMember(statsrapidjson::StringRef("responsetimeout"),(*pit)->getRespTimeout(), allocator);
		   row1.AddMember(statsrapidjson::StringRef("maxtimeouts"),(*pit)->getMaxTimeouts(), allocator);
		   row1.AddMember(statsrapidjson::StringRef("timeouts"), (*pit)->getTimeouts(), allocator);

		   value2.AddMember(statsrapidjson::StringRef("sent"),(*pit)->getReqSent(), allocator);
		   value2.AddMember(statsrapidjson::StringRef("received"),(*pit)->getReqReceived(), allocator);
                   values2.PushBack(value2,allocator);
	           row1.AddMember(statsrapidjson::StringRef("request"), values2, allocator);

		   value3.AddMember(statsrapidjson::StringRef("sent"),(*pit)->getRespSent(), allocator);
		   value3.AddMember(statsrapidjson::StringRef("received"),(*pit)->getRespReceived(), allocator);
                   values3.PushBack(value3,allocator);
	           row1.AddMember(statsrapidjson::StringRef("response"), values3, allocator);

	row.AddMember(statsrapidjson::StringRef("Health"), row1, allocator);//4

        for (auto pit = getValues().begin(); pit != getValues().end(); ++pit)
        {
		   statsrapidjson::Value value(statsrapidjson::kObjectType);

		   value.AddMember(statsrapidjson::StringRef("type"), statsrapidjson::StringRef((*pit)->getName().c_str()), allocator);
		   value.AddMember(statsrapidjson::StringRef("direction"), statsrapidjson::StringRef((*pit)->getDname().c_str()), allocator);
		   value.AddMember(statsrapidjson::StringRef("count"), (*pit)->getValue(), allocator);
		   value.AddMember(statsrapidjson::StringRef("last"),(*pit)->getValue(), allocator);
		   //value.AddMember(statsrapidjson::StringRef("last"),statsrapidjson::StringRef((*pit)->getTime().c_str()), allocator);
                   values.PushBack(value,allocator);
        }

	row.AddMember(statsrapidjson::StringRef("Messages"), values, allocator);//5
    //row.AddMember(statsrapidjson::StringRef("Peer"),row3,allocator);
	//arrayValues.PushBack(row, allocator);

}

void CStatCategory::serialize(std::stringstream &ss)
{
	int cnt = 0;

	for (auto pit = getValues().begin(); pit != getValues().end(); ++pit)
	{
              for (auto vit = (*pit)->getValues().begin(); vit != (*pit)->getValues().end(); ++vit)       //class CStatValue
              {
		ss << "," << (*vit)->getValue();
		cnt++;
              }
	}

	for (; cnt < CStats::singleton().getMaxValues(); cnt++)
		ss << ",";
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

int parse_config_param(const char *param,const char *json,char **response,char *buff)
{

     const char *ret;
     statsrapidjson::Document doc;
     doc.Parse(json);


     if(doc.HasParseError())
     {
             if (response)
                     *response = strdup("{\"result\": \"ERROR\"}");
             return -1;
     }
     if(!doc.HasMember(param) || !doc[param].IsString())
     {
             if (response)
                     *response = strdup("{\"result\": \"ERROR\"}");
             return -1;
     }

     ret= doc[param].GetString();
     //printf("In parse_config_param() ret:%s",ret);
     strcpy(buff,ret);

     return 0;

}


void  construct_json(const char *param,const char *value,const char *effect, char *buf)
{
	const char *ret;
	statsrapidjson::Document document;
	document.SetObject();
	statsrapidjson::Document::AllocatorType &allocator = document.GetAllocator();

	document.AddMember("parameter",statsrapidjson::StringRef(param),allocator);
	document.AddMember("set_value",statsrapidjson::StringRef(value),allocator);
	document.AddMember("Effective_on",statsrapidjson::StringRef(effect),allocator);

	statsrapidjson::StringBuffer strbuf;
	statsrapidjson::Writer<statsrapidjson::StringBuffer> writer(strbuf);
	document.Accept(writer);

	ret=strbuf.GetString();
	strcpy(buf,ret);

}


extern clock_t cp_stats_execution_time;

void
get_current_file_size(size_t len)
{

     struct stat st;
     stat("logs/cp_stat.log",&st);
     size_t size = st.st_size;
     size_t max_size = (20*1024*1024);
     //size_t max_size = (5000);
     //printf("current json size is %lu \n",len);
     //printf("current file size is : %lu \n",size);
     static int flag=0;
     static clock_t reset_time;
     clock_t cp_stats_reset_time;

     //reset_time = clock()-cp_stats.execution_time;
     if (flag==0)
     reset_time = cp_stats_execution_time ;

     if( len > max_size-size )         //reset log will be take place
     {
     //reset_time+ = cp_stats.reset_time;
     reset_time = clock();
     flag=1;
     }


     //current_time = clock();
     //reset_time = current_time-cp_stats.execution_time;
      cp_stats_reset_time = clock()-reset_time;
      cp_stats_reset_time = cp_stats_reset_time/CLOCKS_PER_SEC;

     CStats::singleton().setResetsecs(cp_stats_reset_time);

      //cp_stats.reset_time = ((clock()-reset_time)/CLOCKS_PER_SEC);
      //printf("reset time is %lu\n",cp_stats_reset_time);
 }










