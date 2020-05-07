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

#include <stdio.h>
#include <stdarg.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <queue>

//#define RAPIDJSON_NAMESPACE cprapidjson
#define RAPIDJSON_NAMESPACE statsrapidjson
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "clogger.h"
#include "slogger.h"
#include "stime.h"
//#define SPDLOG_LEVEL_NAMES { "trace", "debug", "info",  "warning", "error", "critical", "off" };
//#define SPDLOG_LEVEL_NAMES { "trace", "debug", "info",  "startup", "warn", "error", "off" };

#define SPDLOG_LEVEL_NAMES { "trace", "debug", "info",  "major", "minor", "critical", "off" };

#define SPDLOG_ENABLE_SYSLOG
#include <spdlog/spdlog.h>


class Xmessage
{

public:
       Xmessage(const char *category,const char *log_level,const char *message)
               :m_category(category),m_log_level(log_level),m_message(message)
       {
       }


       void setCategory(std::string category)  {  m_category = category;   }
       void setLoglevel(std::string log_level) {  m_log_level = log_level; }
       void setMessage(std::string message)    {  m_message = message;     }

       std::string &getCategory()               {  return m_category;  }
       std::string &getLoglevel()               {  return m_log_level; }
       std::string &getMessage()                {  return m_message;   }

       void serialize(statsrapidjson::Value &arrayValues, statsrapidjson::Document::AllocatorType &allocator);

private:

        std::string m_category;
        std::string m_log_level;
        std::string m_message;

};


class RecentLog
{
  public:

         RecentLog()
         {
         }

         static RecentLog &singleton() { if (!m_singleton) m_singleton = new RecentLog(); return *m_singleton; }

         void setName(const char *name)                    { m_name = name; }
         void setAppname(const char *appname)              { m_application = appname; }
         void setMaxsize(int max)                           { m_maxsize = max; }

         const std::string &getName()                       {  return m_name;   }
         const std::string &getAppname()                    {  return m_application;   }
                      int getMaxsize()                     { return m_maxsize; }

         std::queue<Xmessage*> &getQueue()                                  { return m_queue; }

         void maintain_queue()
         {
                 while(m_queue.size() > getMaxsize() )
                 {
                     Xmessage *temp_ptr = m_queue.front();
                     m_queue.pop();
                     delete(temp_ptr);
                 }
         }

         void add_message(const char *category, char *log_level, const char *message)
         {
                 Xmessage *ptr = new Xmessage(category,log_level,message);
                 if( m_queue.size() == getMaxsize())
                 {
                     Xmessage *temp_ptr = m_queue.front();
                     m_queue.pop();
                     delete(temp_ptr);
                 }
                 m_queue.push(ptr);
         }

         void serializeJSON(std::string &json);

  private:


      static RecentLog  *m_singleton;

      std::string m_name;
      std::string m_application;

      std::queue<Xmessage*> m_queue;
      int m_maxsize;
};


class Logger
{
	public:

		static void init( const char *app, uint8_t cp_logger ) { singleton()._init( app, cp_logger ); }
		static void init( const std::string &app ) { init( app.c_str() ); }
		static void cleanup() { singleton()._cleanup(); }
		static void flush() { singleton()._flush(); }
		static std::string serialize() { return singleton()._serialize(); }
		static bool updateLogger(const std::string &loggerName, int value) { return singleton()._updateLogger(loggerName, value); }

		static SLogger &log(const int l) { return *singleton().m_loggers[l]; }
		static int logCount() { return singleton().m_loggers.size(); }

		static SLogger &stat() { return *singleton().m_stat; }
		static SLogger &audit() { return *singleton().m_audit; }

		static Logger &singleton() { if (!m_singleton) m_singleton = new Logger(); return *m_singleton; }

		static int addLogger(const char *logname, uint8_t cp_logger) { return singleton()._addLogger(logname, cp_logger); }
		static int change_file_size(int size)     { return singleton()._change_file_size(size); }


	private:
		static Logger *m_singleton;

		Logger() {}
		~Logger() {}

		void _init( const char *app, uint8_t cp_logger);
		void _cleanup();
		void _flush();
		std::string _serialize();
		bool _updateLogger(const std::string &loggerName, int value);
		int _addLogger(const char *logname, uint8_t cp_logger);
		int _change_file_size(int size);

		std::vector<spdlog::sink_ptr> m_sinks;
		std::vector<spdlog::sink_ptr> m_statsinks;
		std::vector<spdlog::sink_ptr> m_auditsinks;

		std::string m_pattern;
		int m_system;

		std::vector<SLogger*> m_loggers;
		SLogger *m_stat;
		SLogger *m_audit;
};




static std::string optLogFileName = "logs/cp.log";
static int optLogMaxSize = 20; /* MB */
static int optLogNumberFiles = 5;

static std::string optStatFileName = "logs/cp_stat.log";
int optStatMaxSize = 20; /* MB */
//static int optStatNumberFiles = 5;
static int optStatNumberFiles = 0;

static std::string optAuditFileName = "logs/cp_audit.log";
static int optAuditMaxSize = 20; /* MB */
static int optAuditNumberFiles = 5;

static size_t optLogQueueSize = 8192;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void clSetOption(CLoggerOptions opt, const char *val)
{
          switch (opt)
          {
                 case eCLOptLogFileName:
                 {
                        optLogFileName = val;
                        break;
                 }
                 case eCLOptLogMaxSize:
                 {
                        optLogMaxSize = atoi(val);
                        break;
                 }
                 case eCLOptLogNumberFiles:
                 {
                        optLogNumberFiles = atoi(val);
                        break;
                 }
                 case eCLOptStatFileName:
                 {
                        optStatFileName = val;
                        break;
                 }
                 case eCLOptStatMaxSize:
                 {
                        optStatMaxSize = atoi(val);
                        break;
                 }
                 case eCLOptStatNumberFiles:
                 {
                        optStatNumberFiles = atoi(val);
                        break;
                 }
                 case eCLOptAuditFileName:
                 {
                        optAuditFileName = val;
                        break;
                 }
                 case eCLOptAuditMaxSize:
                 {
                        optAuditMaxSize = atoi(val);
                        break;
                 }
                 case eCLOptAuditNumberFiles:
                 {
                        optAuditNumberFiles = atoi(val);
                        break;
                 }
                case eCLOptLogQueueSize:
                {
                        optLogQueueSize = strtoul(val, NULL, 0);
                        break;
                }
        }
}

void clInit(const char *app, uint8_t cp_logger)
{
	Logger::init(app, cp_logger);
}

void clStart(void)
{
}

void clStop(void)
{
	Logger::flush();
	Logger::cleanup();
}

int clAddLogger(const char *logname, uint8_t cp_logger)
{
	return Logger::addLogger(logname, cp_logger);
}

char *clGetLoggers()
{
	std::string loggers = Logger::serialize();
	return strdup(loggers.c_str());
}

int clUpdateLogger(const char *json, char **response)
{
	RAPIDJSON_NAMESPACE::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		*response = strdup("{\"result\": \"ERROR\"}");
		return 400;
	}
	if(!doc.HasMember("name") || !doc["name"].IsString())
	{
		*response = strdup("{\"result\": \"ERROR\"}");
		return 400;
	}
	if(!doc.HasMember("level") || !doc["level"].IsInt())
	{
		*response = strdup("{\"result\": \"ERROR\"}");
		return 400;
	}

	std::string loggerName = doc["name"].GetString();
	int level = doc["level"].GetInt();
	if(Logger::updateLogger(loggerName, level))
	{
		*response = strdup("{\"result\": \"OK\"}");
		return 200;
	}
	else
	{
		*response = strdup("{\"result\": \"ERROR\"}");
		return 400;
	}
}

void clLog(const int logid, enum CLoggerSeverity sev, const char *fmt, ...)
{
	if (logid < 0 || logid >= Logger::logCount())
		return;

        va_list args;
        va_list args1;
        va_start (args, fmt);
        va_start (args1, fmt);

        char buffer[1024] = {0};
	char buf1[32] = {0};
        std::string category;

        category=Logger::log(logid).get_name();

	//strcpy(buf2,"Info");

        vsnprintf( buffer, sizeof(buffer), fmt, args1 );

        //clAddobject(buf1, buf2, buffer);
        //clAddobject(category.c_str(), buf1, buffer);

	switch (sev)
	{
		case eCLSeverityTrace:   { Logger::log(logid).trace_args( fmt, args );
                                           break; }
		case eCLSeverityDebug:   { Logger::log(logid).debug_args( fmt, args );
                                           break; }
		case eCLSeverityInfo:    { Logger::log(logid).info_args( fmt, args );
                                           break; }
		case eCLSeverityMinor: { Logger::log(logid).startup_args( fmt, args );
	                                   strcpy(buf1,"minor");
                                           break; }
		case eCLSeverityMajor:    { Logger::log(logid).warn_args( fmt, args );
	                                   strcpy(buf1,"major");
                                           break; }
		case eCLSeverityCritical:   { Logger::log(logid).error_args( fmt, args );
	                                   strcpy(buf1,"critical");
                                           break; }
	}
        if(buf1[0]!= 0)
        clAddobject(category.c_str(), buf1, buffer);
        va_end (args);
}

void *clGetAuditLogger()
{
	return &Logger::audit();
}

void *clGetStatsLogger()
{
	return &Logger::stat();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
RecentLog *RecentLog::m_singleton = NULL;

void clAddRecentLogger(const char *name,const char *app_name, int max)
{
      RecentLog::singleton().setName(name);
      RecentLog::singleton().setAppname(app_name);
      RecentLog::singleton().setMaxsize(max);

}

void clAddobject(const char *category, char *log_level, const char *message)
{

      RecentLog::singleton().add_message(category, log_level, message);

}


int clRecentLogger(const char *request, char **response)
{
  std::string live;
  RecentLog::singleton().serializeJSON(live);
  *response = strdup(live.c_str());
  return 200;

}

int clRecentLogMaxsize(const char *request, char **response)
{

  std::string res = "{\"Recent_Log_Max_Size\": " + std::to_string(RecentLog::singleton().getMaxsize()) + "}";
  *response = strdup(res.c_str());
  return 200;

}

int clRecentSetMaxsize(const char *json, char **response)
{

	RAPIDJSON_NAMESPACE::Document doc;
	doc.Parse(json);
	if(doc.HasParseError())
	{
		*response = strdup("{\"result\": \"ERROR\"}");
		return 400;
	}
	if(!doc.HasMember("max_size") || !doc["max_size"].IsInt())
	{
		*response = strdup("{\"result\": \"ERROR\"}");
		return 400;
	}

	int size = doc["max_size"].GetInt();
        RecentLog::singleton().setMaxsize(size);
        RecentLog::singleton().maintain_queue();
        std::string res = "{\"Recent_Log_Max_Size\": " + std::to_string(RecentLog::singleton().getMaxsize()) + "}";
        *response = strdup(res.c_str());
        return 200;

}
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

Logger *Logger::m_singleton = NULL;
int clSystemLog = -1;

void Logger::_init( const char *app, uint8_t cp_logger)
{
        m_sinks.push_back( std::make_shared<spdlog::sinks::syslog_sink>() );
        m_sinks.push_back( std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>() );
        m_sinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		optLogFileName, optLogMaxSize * 1024 * 1024, optLogNumberFiles ) );


        m_sinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		optAuditFileName, optLogMaxSize * 1024 * 1024, optAuditNumberFiles ) );



        m_sinks[0]->set_level( spdlog::level::warn );
        m_sinks[1]->set_level( spdlog::level::trace );  //set base level for console
        m_sinks[2]->set_level( spdlog::level::trace );
        m_sinks[3]->set_level( spdlog::level::warn );


		/*m_statsinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
					optStatFileName, optStatMaxSize * 1024 * 1024, optStatNumberFiles ) );
		m_statsinks[0]->set_level( spdlog::level::info );*/

		m_statsinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
					optStatFileName, optStatMaxSize * 1024 * 1024, optStatNumberFiles ) );
		m_statsinks[0]->set_level( spdlog::level::info );
		/*   m_auditsinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
			 optAuditFileName, optAuditMaxSize * 1024 * 1024, optAuditNumberFiles ) );
			 m_statsinks[0]->set_level( spdlog::level::warn ); */

        std::stringstream ss;
        ss << "[%Y-%m-%dT%H:%M:%S.%e] [" << app << "] [%n] [%l] %v";
		m_pattern = ss.str();

		clSystemLog  = _addLogger("system", cp_logger);
        m_stat = new SLogger( "stat", m_statsinks, "%v", optLogQueueSize );
        m_audit = new SLogger( "audit", m_auditsinks, "%v", optLogQueueSize );

        //m_loggers[clSystemLog]->set_level( spdlog::level::info );
        m_stat->set_level(spdlog::level::info);
        m_audit->set_level(spdlog::level::warn);
}

int Logger::_addLogger(const char *logname, uint8_t cp_logger)
{
	m_loggers.push_back(new SLogger(logname, m_sinks, m_pattern.c_str(), optLogQueueSize));

	if(cp_logger == activate_log_level)
		m_loggers[m_loggers.size() - 1]->set_level( spdlog::level::trace );
	else
		m_loggers[m_loggers.size() - 1]->set_level( spdlog::level::critical );

	return m_loggers.size() - 1;
}

void Logger::_cleanup()
{
	while (!m_loggers.empty())
	{
		SLogger *l = m_loggers.back();
		m_loggers.pop_back();
		delete l;
	}
        if ( m_stat )
                delete m_stat;
        if ( m_audit )
                delete m_audit;
}

void Logger::_flush()
{
	for (auto it = m_loggers.begin(); it != m_loggers.end(); ++it)
		(*it)->flush();
        if ( m_stat )
                m_stat->flush();
        if ( m_audit )
                m_audit->flush();
}


void RecentLog::serializeJSON(std::string &json)
{

        std::queue<Xmessage*> temp_queue = m_queue;
        STime now = STime::Now();
	std::string nowstr;
	statsrapidjson::Document document;
	document.SetObject();
	statsrapidjson::Document::AllocatorType &allocator = document.GetAllocator();

        std::cout<< "temp_queue.size() :" << temp_queue.size();

        document.AddMember("name", statsrapidjson::StringRef(getName().c_str()), allocator);
        document.AddMember("application", statsrapidjson::StringRef(getAppname().c_str()), allocator);


        now.Format(nowstr, "%Y-%m-%dT%H:%M:%S.%0", false);
        document.AddMember("reporttime", statsrapidjson::StringRef(nowstr.c_str()), allocator);

	statsrapidjson::Value arrayValues(statsrapidjson::kArrayType);

        while(!temp_queue.empty())
        {
             (temp_queue.front())->serialize(arrayValues,allocator);
              temp_queue.pop();
        }

        document.AddMember("Messages", arrayValues, allocator);
        statsrapidjson::StringBuffer strbuf;
	statsrapidjson::Writer<statsrapidjson::StringBuffer> writer(strbuf);
	document.Accept(writer);

	json = strbuf.GetString();

}


void Xmessage::serialize(statsrapidjson::Value &arrayValues, statsrapidjson::Document::AllocatorType &allocator)
{

        STime now = STime::Now();
	std::string nowstr;
	statsrapidjson::Value row(statsrapidjson::kObjectType);
	statsrapidjson::Value values(statsrapidjson::kArrayType);

	row.AddMember(statsrapidjson::StringRef("category"), statsrapidjson::StringRef(getCategory().c_str()), allocator);
	row.AddMember(statsrapidjson::StringRef("loglevel"), statsrapidjson::StringRef(getLoglevel().c_str()), allocator);


	row.AddMember(statsrapidjson::StringRef("description"), statsrapidjson::StringRef(getMessage().c_str()), allocator);

	arrayValues.PushBack(row, allocator);

}

std::string Logger::_serialize()
{
        RAPIDJSON_NAMESPACE::Document document;
        document.SetObject();
        RAPIDJSON_NAMESPACE::Document::AllocatorType& allocator = document.GetAllocator();

        RAPIDJSON_NAMESPACE::Value array(RAPIDJSON_NAMESPACE::kArrayType);

	for (auto it = m_loggers.begin(); it != m_loggers.end(); ++it)
	{
		RAPIDJSON_NAMESPACE::Value l(RAPIDJSON_NAMESPACE::kObjectType);
		l.AddMember("name", RAPIDJSON_NAMESPACE::StringRef((*it)->get_name().c_str()), allocator);
		l.AddMember("level", (*it)->get_level(), allocator);

		array.PushBack(l, allocator);
	}

        document.AddMember("loggers", array, allocator);

        RAPIDJSON_NAMESPACE::StringBuffer strbuf;
        RAPIDJSON_NAMESPACE::Writer<RAPIDJSON_NAMESPACE::StringBuffer> writer(strbuf);
        document.Accept(writer);
        return strbuf.GetString();
}

bool Logger::_updateLogger(const std::string &loggerName, int value)
{
	for (auto it = m_loggers.begin(); it != m_loggers.end(); ++it)
	{
		if ((*it)->get_name() == loggerName)
		{
                	(*it)->set_level(static_cast<spdlog::level::level_enum>(value));
			return true;
		}
	}

        return false;
}
