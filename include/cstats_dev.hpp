#ifndef __CSTATS_DEV_H
#define __CSTATS_DEV_H

using namespace std;

enum cp_config {
    SGWC = 1,
    PGWC,
    SAEGWC
};


class CStatMessages
{
    string nodestr;
	bool suppress;
public:
    CStatMessages(bool suppressed)
    {
        nodestr = "messages";
		suppress = suppressed;
    }

    string getNodeName()
    {
        return nodestr;
    }

    void serialize(const SPeer* peer,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

    void serializeS11(const SPeer* peer,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

    void serializeS5S8(const SPeer* peer,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

    void serializeSxa(const SPeer* peer,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

    void serializeSxb(const SPeer* peer,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

    void serializeSxaSxb(const SPeer* peer,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

 	void serializeSystem(const cli_node_t *cli_node,
    	statsrapidjson::Value& row,
    	statsrapidjson::Value& arrayObjects,
    	statsrapidjson::Document::AllocatorType& allocator);

private:
};

class CStatHealth
{
    string nodestr;
	bool suppress;
public:
    CStatHealth(bool suppressed)
    {
        nodestr = "health";
		suppress = suppressed;
    }

    string getNodeName()
    {
        return nodestr;
    }

    void serialize(const SPeer* peer,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

private:
};

class CStatPeers
{
    string nodestr;
	bool suppress;

public:
    CStatPeers(bool suppressed)
    {
        nodestr = "peers";
		suppress = suppressed;
    }

    string getNodeName()
    {
        return nodestr;
    }

    void serialize(const SPeer* peer,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

private:
};

class CStatInterfaces
{
    string nodestr;
    CStatPeers peer;
	bool suppress;
public:
    CStatInterfaces(bool suppressed) : peer(suppressed)
    {
        nodestr = "interfaces";
		suppress = suppressed;
    }

    string getNodeName()
    {
        return nodestr;
    }

    void serialize(cli_node_t *cli_node,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

    void serializeInterface(cli_node_t *cli_node,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator,EInterfaceType it);

private:
};

class CStatGateway
{
    string nodestr,reportTimeStr;
    CStatInterfaces interfaces;
	bool suppress;
public:
    CStatGateway(bool suppressed) : interfaces(suppressed)
    {
        nodestr = "gateway";
		suppress = suppressed;
    }

    string getNodeName()
    {
        return nodestr;
    }

	 void initInterfaceDirection(cp_config gatway);

    void serialize(cli_node_t *cli_node,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

private:
};

class CStatSystem
{
    string nodestr;
	bool suppress;
public:
    CStatSystem(bool suppressed)
    {
        nodestr = "system";
		suppress = suppressed;
    }

    string getNodeName()
    {
        return nodestr;
    }

    void serialize(cli_node_t *cli_node,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

private:
};

class CStatSession
{
    string nodestr;
	bool suppress;
public:
    CStatSession(bool suppressed)
    {
        nodestr = "sessions";
		suppress = suppressed;
    }

    string getNodeName()
    {
        return nodestr;
    }

    void serialize(cli_node_t *cli_node,
        statsrapidjson::Value& row,
        statsrapidjson::Value& arrayObjects,
        statsrapidjson::Document::AllocatorType& allocator);

private:
};

#endif /* __CSTATS_DEV_H */

