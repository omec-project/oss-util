#include <stdio.h>
#include <stdarg.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define RAPIDJSON_NAMESPACE cprapidjson
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "clogger.h"
#include "slogger.h"

//#define SPDLOG_LEVEL_NAMES { "trace", "debug", "info",  "warning", "error", "critical", "off" };
#define SPDLOG_LEVEL_NAMES { "trace", "debug", "info",  "startup", "warn", "error", "off" };

#define SPDLOG_ENABLE_SYSLOG
#include <spdlog/spdlog.h>

class Logger
{
public:

        static void init( const char *app ) { singleton()._init( app ); }
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

	static int addLogger(const char *logname) { return singleton()._addLogger(logname); }

private:
        static Logger *m_singleton;

        Logger() {}
        ~Logger() {}

        void _init( const char *app );
        void _cleanup();
        void _flush();
        std::string _serialize();
        bool _updateLogger(const std::string &loggerName, int value);
	int _addLogger(const char *logname);

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
static int optStatMaxSize = 20; /* MB */
static int optStatNumberFiles = 5;

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

void clInit()
{
        Logger::init( "sgwc" );
}

void clStart(void)
{
}

void clStop(void)
{
	Logger::flush();
	Logger::cleanup();
}

int clAddLogger(const char *logname)
{
	return Logger::addLogger(logname);
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
        va_start (args, fmt);

	switch (sev)
	{
		case eCLSeverityTrace:   { Logger::log(logid).trace_args( fmt, args );   break; }
		case eCLSeverityDebug:   { Logger::log(logid).debug_args( fmt, args );   break; }
		case eCLSeverityInfo:    { Logger::log(logid).info_args( fmt, args );    break; }
		case eCLSeverityStartup: { Logger::log(logid).startup_args( fmt, args ); break; }
		case eCLSeverityWarn:    { Logger::log(logid).warn_args( fmt, args );    break; }
		case eCLSeverityError:   { Logger::log(logid).error_args( fmt, args );   break; }
	}

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

Logger *Logger::m_singleton = NULL;
int clSystemLog = -1;

void Logger::_init( const char *app )
{
        m_sinks.push_back( std::make_shared<spdlog::sinks::syslog_sink>() );
        m_sinks.push_back( std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>() );
        m_sinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		optLogFileName, optLogMaxSize * 1024 * 1024, optLogNumberFiles ) );

        m_sinks[0]->set_level( spdlog::level::warn );
        m_sinks[1]->set_level( spdlog::level::info );
        m_sinks[2]->set_level( spdlog::level::trace );

        m_statsinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		optStatFileName, optStatMaxSize * 1024 * 1024, optStatNumberFiles ) );
        m_statsinks[0]->set_level( spdlog::level::info );

        m_auditsinks.push_back( std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		optAuditFileName, optAuditMaxSize * 1024 * 1024, optAuditNumberFiles ) );
        m_statsinks[0]->set_level( spdlog::level::trace );

        std::stringstream ss;
        ss << "[%Y-%m-%dT%H:%M:%S.%e] [" << app << "] [%n] [%l] %v";
	m_pattern = ss.str();

	clSystemLog  = _addLogger("system");
        m_stat = new SLogger( "stat", m_statsinks, "%v", optLogQueueSize );
        m_audit = new SLogger( "audit", m_auditsinks, "%v", optLogQueueSize );

        m_loggers[clSystemLog]->set_level( spdlog::level::info );
        m_stat->set_level(spdlog::level::info);
        m_audit->set_level(spdlog::level::trace);
}

int Logger::_addLogger(const char *logname)
{
	m_loggers.push_back(new SLogger(logname, m_sinks, m_pattern.c_str(), optLogQueueSize));
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
