#include <iostream>
#include <pistache/endpoint.h>
#include <pistache/router.h>

#include <map>
#include <memory>
#include <string>
#include <sstream>

#include "slogger.h"
#include "stime.h"

#include "clogger.h"
#include "cstats.h"
#include "crest.h"

#define RAPIDJSON_NAMESPACE crestrj
#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class _RestStaticHandler
{
public:
	_RestStaticHandler(enum CRCommand cmd, const char *route, CRestStaticHandler handler)
		: m_cmd(cmd), m_route(route), m_handler(handler)
	{
	}

	enum CRCommand getCommand() { return m_cmd; }
	std::string &getRoute() { return m_route; }
	CRestStaticHandler getHandler() { return m_handler; }

	_RestStaticHandler &setCommand(enum CRCommand cmd) { m_cmd = cmd; return *this; }
	_RestStaticHandler &setHandler(CRestStaticHandler handler) { m_handler = handler; return *this; }
	
private:
	_RestStaticHandler();

	enum CRCommand m_cmd;
	std::string m_route;
	CRestStaticHandler m_handler;
};

class _RestDynamicHandler
{
public:
	_RestDynamicHandler(enum CRCommand cmd, const char *baseroute, const char *param, const char *route, CRestDynamicHandler handler)
		: m_cmd(cmd), m_baseroute(baseroute), m_param(param), m_route(route), m_handler(handler)
	{
	}

	enum CRCommand getCommand() { return m_cmd; }
	std::string &getBaseRoute() { return m_baseroute; }
	std::string &getParam() { return m_param; }
	std::string &getRoute() { return m_route; }
	CRestDynamicHandler getHandler() { return m_handler; }

	_RestDynamicHandler &setCommand(enum CRCommand cmd) { m_cmd = cmd; return *this; }
	_RestDynamicHandler &setParam(const char *param) { m_param = param; return *this;  }
	_RestDynamicHandler &setRoute(const char *route) { m_route = route; return *this; }
	_RestDynamicHandler &setHandler(CRestDynamicHandler handler) { m_handler = handler; return *this; }

private:
	_RestDynamicHandler();

	enum CRCommand m_cmd;
	std::string m_baseroute;
	std::string m_param;
	std::string m_route;
	CRestDynamicHandler m_handler;
};

class RestEndpoint;

class RestHandler
{
public:
	static RestHandler &singleton() { if (!m_singleton) m_singleton = new RestHandler(); return *m_singleton; }

	static void init(SLogger *audit, int port, size_t thread_count) { singleton()._init(audit, port, thread_count); }
	static void registerRoutes(RestEndpoint &ep) { singleton()._registerRoutes(ep); }
	static void registerStaticHandler(enum CRCommand cmd, const char *route, CRestStaticHandler handler) { singleton()._registerStaticHandler(cmd, route, handler); }
	static void registerDynamicHandler(enum CRCommand cmd, const char *baseroute, const char *param, const char *route, CRestDynamicHandler handler) { singleton()._registerDynamicHandler(cmd, baseroute, param, route, handler); }

	void staticHandler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
	void dynamicHandler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);

private:
	RestHandler() {}
	~RestHandler() {}

	void _init(SLogger *audit, int port, size_t thread_count);
	void _registerRoutes(RestEndpoint &ep);
	void _registerStaticHandler(enum CRCommand cmd, const char *route, CRestStaticHandler handler);
	void _registerDynamicHandler(enum CRCommand cmd, const char *baseroute, const char *param, const char *route, CRestDynamicHandler handler);

	void _auditLog(const Pistache::Rest::Request &request);

	static RestHandler *m_singleton;

	SLogger *m_audit;
	std::map<std::string,_RestStaticHandler*> m_staticroutes;
	std::map<std::string,_RestDynamicHandler*> m_dynamicroutes;
};

class RestEndpoint
{
public:
	static RestEndpoint &singleton() { if (!m_singleton) m_singleton = new RestEndpoint(); return *m_singleton; }

	void init(int port, size_t thread_count = 1)
	{
		Pistache::Address addr(Pistache::Ipv4::any(), port);
		auto options = Pistache::Http::Endpoint::options()
			.threads(thread_count)
			.flags(Pistache::Tcp::Options::ReuseAddr);
		
		m_httpendpoint = std::make_shared<Pistache::Http::Endpoint>(addr);
		m_httpendpoint->init(options);
	}

	void start()
	{
		m_httpendpoint->setHandler(m_router.handler());
		m_httpendpoint->serveThreaded();
	}

	void stop()
	{
		m_httpendpoint->shutdown();
	}

	Pistache::Rest::Router &getRouter() { return m_router; }

private:
	static RestEndpoint *m_singleton;

	RestEndpoint() {}

	std::shared_ptr<Pistache::Http::Endpoint> m_httpendpoint;
	Pistache::Rest::Router m_router;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void crInit(void *auditLogger, int port, size_t thread_count)
{
	RestHandler::init((SLogger*)auditLogger, port, thread_count);
}

void crStart()
{
	RestHandler::registerRoutes(RestEndpoint::singleton());
	RestEndpoint::singleton().start();
}

void crStop()
{
	RestEndpoint::singleton().stop();
}

void crRegisterStaticHandler(enum CRCommand cmd, const char *route, CRestStaticHandler handler)
{
	RestHandler::registerStaticHandler(cmd, route, handler);
}

void crRegisterDynamicHandler(enum CRCommand cmd, const char *baseroute, const char *param, const char *route, CRestDynamicHandler handler)
{
	RestHandler::registerDynamicHandler(cmd, baseroute, param, route, handler);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

RestHandler *RestHandler::m_singleton = NULL;
RestEndpoint *RestEndpoint::m_singleton = NULL;

void RestHandler::_init(SLogger *audit, int port, size_t thread_count)
{
	m_audit = audit;
	RestEndpoint::singleton().init(port, thread_count);
}

void RestHandler::_registerStaticHandler(enum CRCommand cmd, const char *route, CRestStaticHandler handler)
{
	std::string srchstr;

	switch (cmd)
	{
		case eCRCommandGet:	{ srchstr = "GET:"; break; }
		case eCRCommandPost:	{ srchstr = "POST:"; break; }
		case eCRCommandPut:	{ srchstr = "PUT:"; break; }
		case eCRCommandDelete:	{ srchstr = "DELETE:"; break; }
	}

	srchstr += route;

	auto srch = m_staticroutes.find(srchstr);

	if (srch == m_staticroutes.end())
	{
		_RestStaticHandler *rh = new _RestStaticHandler(cmd, route, handler);
		m_staticroutes[srchstr] = rh;
	}
	else
	{
		srch->second->setCommand(cmd);
		srch->second->setHandler(handler);
	}
}

void RestHandler::_registerDynamicHandler(enum CRCommand cmd, const char *baseroute, const char *param, const char *route, CRestDynamicHandler handler)
{
	std::string srchstr;

	switch (cmd)
	{
		case eCRCommandGet:	{ srchstr = "GET:"; break; }
		case eCRCommandPost:	{ srchstr = "POST:"; break; }
		case eCRCommandPut:	{ srchstr = "PUT:"; break; }
		case eCRCommandDelete:	{ srchstr = "DELETE:"; break; }
	}

	srchstr += baseroute;

	auto srch = m_dynamicroutes.find(srchstr);

	if (srch == m_dynamicroutes.end())
	{
		_RestDynamicHandler *rh = new _RestDynamicHandler(cmd, baseroute, param, route, handler);
		m_dynamicroutes[srchstr] = rh;
	}
	else
	{
		srch->second->setCommand(cmd);
		srch->second->setParam(param);
		srch->second->setRoute(route);
		srch->second->setHandler(handler);
	}
}

void RestHandler::staticHandler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
{
	_auditLog(request);

	std::string srchstr;

	switch (request.method())
	{
		case Pistache::Http::Method::Get:	srchstr = "GET:";	break;
		case Pistache::Http::Method::Post:	srchstr = "POST:";	break;
		case Pistache::Http::Method::Put:	srchstr = "PUT:";	break;
		case Pistache::Http::Method::Delete:	srchstr = "DELETE:";	break;
	}

	srchstr += request.resource();

	auto srch = m_staticroutes.find(srchstr);

	if (srch == m_staticroutes.end())
	{
		std::stringstream ss;
		ss << "{\"result\": \"Unrecognized resource [" << request.resource() << "]\"}";
		response.send(Pistache::Http::Code::Bad_Request, ss.str());
		return;
	}

	char *responseBody = NULL;
	int code = (*srch->second->getHandler())(request.body().c_str(), &responseBody);

	if (responseBody)
	{
		response.send((Pistache::Http::Code)code, responseBody);
		free(responseBody);
	}
	else
	{
		response.send((Pistache::Http::Code)code);
	}
}

void RestHandler::dynamicHandler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
{
	_auditLog(request);

	std::string resource(request.resource());

	for (auto it = m_dynamicroutes.begin(); it != m_dynamicroutes.end(); ++it)
	{
		if (resource.compare(0,it->first.size(),it->first) == 0)
		{
			std::string param(request.param(it->second->getParam().c_str()).as<std::string>());
			char *responseBody = NULL;
			int code = (*it->second->getHandler())(param.c_str(), request.body().c_str(), &responseBody);
			if (responseBody)
			{
				response.send((Pistache::Http::Code)code, responseBody);
				free(responseBody);
			}
			else
			{
				response.send((Pistache::Http::Code)code);
			}
			return;
		}
	}

	std::stringstream ss;
	ss << "{\"result\": \"Unrecognized resource [" << request.resource() << "]\"}";
	response.send(Pistache::Http::Code::Bad_Request, ss.str());
}

void RestHandler::_registerRoutes(RestEndpoint &ep)
{
	for (auto sit = m_staticroutes.begin(); sit != m_staticroutes.end(); ++sit)
	{
		switch (sit->second->getCommand())
		{
			case eCRCommandGet:
			{
				Pistache::Rest::Routes::Get(
					ep.getRouter(),
					sit->second->getRoute().c_str(),
					Pistache::Rest::Routes::bind(&RestHandler::staticHandler, m_singleton)
				);
				break;
			}
			case eCRCommandPost:
			{
				Pistache::Rest::Routes::Post(
					ep.getRouter(),
					sit->second->getRoute().c_str(),
					Pistache::Rest::Routes::bind(&RestHandler::staticHandler, m_singleton)
				);
				break;
			}
			case eCRCommandPut:
			{
				Pistache::Rest::Routes::Put(
					ep.getRouter(),
					sit->second->getRoute().c_str(),
					Pistache::Rest::Routes::bind(&RestHandler::staticHandler, m_singleton)
				);
				break;
			}
			case eCRCommandDelete:
			{
				Pistache::Rest::Routes::Delete(
					ep.getRouter(),
					sit->second->getRoute().c_str(),
					Pistache::Rest::Routes::bind(&RestHandler::staticHandler, m_singleton)
				);
				break;
			}
		}
	}

	for (auto dit = m_staticroutes.begin(); dit != m_staticroutes.end(); ++dit)
	{
		switch (dit->second->getCommand())
		{
			case eCRCommandGet:
			{
				Pistache::Rest::Routes::Get(
					ep.getRouter(),
					dit->second->getRoute().c_str(),
					Pistache::Rest::Routes::bind(&RestHandler::dynamicHandler, m_singleton)
				);
				break;
			}
			case eCRCommandPost:
			{
				Pistache::Rest::Routes::Post(
					ep.getRouter(),
					dit->second->getRoute().c_str(),
					Pistache::Rest::Routes::bind(&RestHandler::dynamicHandler, m_singleton)
				);
				break;
			}
			case eCRCommandPut:
			{
				Pistache::Rest::Routes::Put(
					ep.getRouter(),
					dit->second->getRoute().c_str(),
					Pistache::Rest::Routes::bind(&RestHandler::dynamicHandler, m_singleton)
				);
				break;
			}
			case eCRCommandDelete:
			{
				Pistache::Rest::Routes::Delete(
					ep.getRouter(),
					dit->second->getRoute().c_str(),
					Pistache::Rest::Routes::bind(&RestHandler::dynamicHandler, m_singleton)
				);
				break;
			}
		}
	}
}

void RestHandler::_auditLog(const Pistache::Rest::Request &request)
{
	std::stringstream ss;
	STime now = STime::Now();
	std::string nowstr;
	now.Format(nowstr, "%Y-%m-%dT%H:%M:%S.%0", false);
	ss <<
		"\"" << nowstr << "\""
		<< "," << "\"administrator\""
		<< "," << "\"" << request.method() << "\""
		<< "," << "\"" << request.resource() << "\""
		<< "," << "\"" << request.body() << "\"";
	m_audit->info(ss.str().c_str());
}
