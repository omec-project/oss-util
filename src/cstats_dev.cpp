#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cp_adapter.h"
#include "stime.h"

#define RAPIDJSON_NAMESPACE statsrapidjson
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "cstats.h"
#include "cstats_dev.hpp"

using namespace std;

cli_node_t *cli_node_ptr;
int *oss_cnt_peer;

extern  long oss_resetsec;
extern int s5s8MessageTypes[];

MessageType ossS5s8MessageDefs[] = {
        {       3       , "Version Not Supported Indication",dNone      },
        {       32      , "Create Session Request",  dIn        	},//if SGWC then send, if PGWC then recv
        {       33      , "Create Session Response",dRespRcvd   	},//if SGWC then recv, if PGWC then send
        {       36      , "Delete Session Request",  dIn        	},//if SGWC then send, if PGWC then recv
        {       37      , "Delete Session Response",dRespRcvd   	},//if SGWC then recv, if PGWC then send
        {       34      , "Modify Bearer Request",dIn 			},	  //if SGWC then send, if PGWC then recv
        {       35      , "Modify Bearer Response",dRespRcvd    	},//if SGWC then recv, if PGWC then send
        {       40      , "Remote UE Report Notification",dNone 	},
        {       41      , "Remote UE Report Acknowledge",dNone  	},
        {       38      , "Change Notification Request",dNone   	},
        {       39      , "Change Notification Response",dNone  	},
        {       164     , "Resume Notification",dNone   		},
        {       165     , "Resume Acknowledge",dNone    		},
        {       64      , "Modify Bearer Command",dNone 		},
        {       65      , "Modify Bearer Failure Indication",dNone      },
        {       66      , "Delete Bearer Command",dNone 		},
        {       67      , "Delete Bearer Failure Indication",dNone      },
        {       68      , "Bearer Resource Command",dNone       	},
        {       69      , "Bearer Resource Failure Indication",dNone    },
        {       71      , "Trace Session Activation",dNone      	},
        {       72      , "Trace Session Deactivation",dNone    	},
        {       95      , "Create Bearer Request",dNone 		},//if SGWC then recv, if PGWC then send
        {       96      , "Create Bearer Response",dNone        	},//if SGWC then send, if PGWC then recv
        {       97      , "Update Bearer Request",dNone 		},
        {       98      , "Update Bearer Response",dNone        	},
        {       99      , "Delete Bearer Request",dNone 		},
        {       100     , "Delete Bearer Response",dNone        	},
        {       101     , "Delete PDN Connection Set Request",dNone     },
        {       102     , "Delete PDN Connection Set Response",dNone    },
        {       103     , "PGW Downlink Triggering Notification",dNone  },
        {       104     , "PGW Downlink Triggering Acknowledge",dNone   },
        {       162     , "Suspend Notification",dNone  		},
        {       163     , "Suspend Acknowledge",dNone   		},
        {       200     , "Update PDN Connection Set Request",dNone     },
        {       201     , "Update PDN Connection Set Response",dNone    },
        {       -1      , NULL  					}
};

MessageType ossS11MessageDefs[] = {
        {       3       ,"Version Not Supported Indication", dNone 	},
        {       32      ,"Create Session Request", dIn 			},
        {       33      ,"Create Session Response", dRespSend 		},
        {       36      ,"Delete Session Request", dIn 			},
        {       37      ,"Delete Session Response", dRespSend 		},
        {       34      ,"Modify Bearer Request", dIn 			},
        {       35      ,"Modify Bearer Response", dRespSend 		},
        {       40      ,"Remote UE Report Notification", dNone 	},
        {       41      ,"Remote UE Report Acknowledge", dNone 		},
        {       38      ,"Change Notification Request", dNone 		},
        {       39      ,"Change Notification Response", dNone 		},
        {       164     ,"Resume Notification", dNone 			},
        {       165     ,"Resume Acknowledge", dNone 			},
        {       64      ,"Modify Bearer Command", dNone 		},
        {       65      ,"Modify Bearer Failure Indication", dNone 	},
        {       66      ,"Delete Bearer Command", dNone 		},
        {       67      ,"Delete Bearer Failure Indication", dNone 	},
        {       68      ,"Bearer Resource Command", dNone 		},
        {       69      ,"Bearer Resource Failure Indication", dNone 	},
        {       70      ,"Downlink Data Notification Failure Indication", dNone },
        {       71      ,"Trace Session Activation", dNone 		},
        {       72      ,"Trace Session Deactivation", dNone 		},
        {       73      ,"Stop Paging Indication", dNone 		},
        {       95      ,"Create Bearer Request", dOut	 		},
        {       96      ,"Create Bearer Response", dIn	 		},
        {       97      ,"Update Bearer Request", dNone 		},
        {       98      ,"Update Bearer Response", dNone 		},
        {       99      ,"Delete Bearer Request", dNone 		},
        {       100     ,"Delete Bearer Response", dNone 		},
        {       101     ,"Delete PDN Connection Set Request", dNone 	},
        {       102     ,"Delete PDN Connection Set Response", dNone 	},
        {       103     ,"PGW Downlink Triggering Notification", dNone 	},
        {       104     ,"PGW Downlink Triggering Acknowledge", dNone 	},
        {       162     ,"Suspend Notification", dNone 			},
        {       163     ,"Suspend Acknowledge", dNone 			},
        {       160     ,"Create Forwarding Tunnel Request", dNone 	},
        {       161     ,"Create Forwarding Tunnel Response", dNone 	},
        {       166     ,"Create Indirect Data Forwarding Tunnel Request", dNone },
        {       167     ,"Create Indirect Data Forwarding Tunnel Response", dNone },
        {       168     ,"Delete Indirect Data Forwarding Tunnel Request", dNone },
        {       169     ,"Delete Indirect Data Forwarding Tunnel Response", dNone },
        {       170     ,"Release Access Bearers Request", dIn 		},
        {       171     ,"Release Access Bearers Response", dOut 	},
        {       176     ,"Downlink Data Notification", dOut 		},
        {       177     ,"Downlink Data Notification Acknowledge", dIn 	},
        {       179     ,"PGW Restart Notification", dNone 		},
        {       180     ,"PGW Restart Notification Acknowledge", dNone 	},
        {       211     ,"Modify Access Bearers Request", dNone 	},
        {       212     ,"Modify Access Bearers Response", dNone 	},
        {       -1      , NULL  					}
};

MessageType ossSxaMessageDefs[] = {
        {	   1	  ,"PFCP Heartbeat Request",dNone		},
        {	   2	  ,"PFCP Heartbeat Response",dNone		},
        {	   5	  ,"PFCP Association Setup Request",dOut	},
        {	   6	  ,"PFCP Association Setup Response",dRespRcvd	},
        {	   7	  ,"PFCP Association Update Request",dNone	},
        {	   8	  ,"PFCP Association Update Response",dNone	},
        {	   9	  ,"PFCP Association Release Request",dNone	},
        {	   10	  ,"PFCP Association Release Response",dNone	},
        {	   11	  ,"PFCP Version Not Supported Response",dNone	},
        {	   12	  ,"PFCP Node Report Request",dNone		},
        {	   13	  ,"PFCP Node Report Response",dNone		},
        {	   14	  ,"PFCP Session Set Deletion Request",dNone	},
        {	   15	  ,"PFCP Session Set Deletion Response",dNone	},
        {	   50	  ,"PFCP Session Establishment Request",dOut	},
        {	   51	  ,"PFCP Session Establishment Response",dRespRcvd},
        {	   52	  ,"PFCP Session Modification Request",dOut	},
        {	   53	  ,"PFCP Session Modification Response",dRespRcvd},
        {	   54	  ,"PFCP Session Deletion Request",dOut		},
        {	   55	  ,"PFCP Session Deletion Response",dRespRcvd	},
        {	   56	  ,"PFCP Session Report Request",dOut		},
        {	   57	  ,"PFCP Session Report Response",dRespRcvd	},
        {          -1     , NULL  }
};

MessageType ossSxbMessageDefs[] = {
        {	  1	  ,"PFCP Heartbeat Request",dNone		},
        {	  2	  ,"PFCP Heartbeat Response",dNone		},
        {	  5	  ,"PFCP Association Setup Request",dOut	},
        {	  6	  ,"PFCP Association Setup Response",dRespRcvd	},
        {	  7	  ,"PFCP Association Update Request",dNone	},
        {	  8	  ,"PFCP Association Update Response",dNone	},
        {	  9	  ,"PFCP Association Release Request",dNone	},
        {	  10	  ,"PFCP Association Release Response",dNone	},
        {	  11	  ,"PFCP Version Not Supported Response",dNone	},
        {	  12	  ,"PFCP Node Report Request",dNone		},
        {	  13	  ,"PFCP Node Report Response",dNone		},
        {	  14	  ,"PFCP Session Set Deletion Request",dNone	},
        {	  15	  ,"PFCP Session Set Deletion Response",dNone	},
        {	  50	  ,"PFCP Session Establishment Request",dOut	},
        {	  51	  ,"PFCP Session Establishment Response",dRespRcvd},
        {	  52	  ,"PFCP Session Modification Request",dOut	},
        {	  53	  ,"PFCP Session Modification Response",dRespRcvd},
        {	  54	  ,"PFCP Session Deletion Request",dOut		},
        {	  55	  ,"PFCP Session Deletion Response",dRespRcvd	},
        {	  56	  ,"PFCP Session Report Request",dOut		},
        {	  57	  ,"PFCP Session Report Response",dRespRcvd	},
        {         -1      , NULL  }
};

MessageType ossSxaSxbMessageDefs[] = {
        {	  1	 ,"PFCP Heartbeat Request",dNone		},
        {	  2	 ,"PFCP Heartbeat Response",dNone		},
        {	  3	 ,"PFCP PFD Management Request",dNone		},
        {	  4	 ,"PFCP PFD Management Response",dNone		},
        {	  5	 ,"PFCP Association Setup Request",dOut		},
        {	  6	 ,"PFCP Association Setup Response",dRespRcvd	},
        {	  7	 ,"PFCP Association Update Request",dNone	},
        {	  8	 ,"PFCP Association Update Response",dNone	},
        {	  9	 ,"PFCP Association Release Request",dNone	},
        {	  10	 ,"PFCP Association Release Response",dNone	},
        {	  11	 ,"PFCP Version Not Supported Response",dNone	},
        {	  12	 ,"PFCP Node Report Request",dNone		},
        {	  13	 ,"PFCP Node Report Response",dNone		},
        {	  14	 ,"PFCP Session Set Deletion Request",dNone	},
        {	  15	 ,"PFCP Session Set Deletion Response",dNone	},
        {	  50	 ,"PFCP Session Establishment Request",dOut	},
        {	  51	 ,"PFCP Session Establishment Response",dRespRcvd},
        {	  52	 ,"PFCP Session Modification Request",dOut	},
        {	  53	 ,"PFCP Session Modification Response",dRespRcvd},
        {	  54	 ,"PFCP Session Deletion Request",dOut		},
        {	  55	 ,"PFCP Session Deletion Response",dRespRcvd	},
        {	  56	 ,"PFCP Session Report Request",dOut		},
        {	  57	 ,"PFCP Session Report Response",dRespRcvd	},
        {         -1     , NULL  					}
};

MessageType ossSystemMessageDefs[] = {
    {  0    ,"Number of active session",dNone	},
    {  1    ,"Number of ues",dNone  		},
    {  2    ,"Number of bearers",dNone          },
    {  3    ,"Number of pdn connections",dNone  },
    {  -1   , NULL  				}
};

char ossInterfaceStr[][10] = {
    "s11" ,
    "s5s8",
    "sxa",
    "sxb",
    "sxasxb",
    "gx",
    "none"
};

char ossInterfaceProtocolStr[][10] = {
    "gtpv2" ,
    "gtpv2",
    "pfcp",
    "pfcp",
    "pfcp",
    "none",
    "none"
};


char ossGatewayStr[][10] = {
    "none",
    "SGWC",
    "PGWC",
    "SAEGWC"
};

void cli_init(cli_node_t *cli_node,int *cnt_peer)
{
	std::cout << ".................................. cli init................" << std::endl;
	cli_node_ptr = cli_node;
  	oss_cnt_peer = cnt_peer;
}


void CStatMessages::serializeS11(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    for(int i = 0; i < S11_MSG_TYPE_LEN; i++) {
        if((peer->stats.s11[i].cnt[0] == 0 && peer->stats.s11[i].cnt[1] == 0) && suppress) {
            continue;
        }

        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossS11MessageDefs[i].msgname), allocator);

        switch(ossS11MessageDefs[i].dir) {
        case dIn:
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.s11[i].cnt[0], allocator);
            break;

        case dOut:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.s11[i].cnt[0], allocator);
            break;

        case dRespSend:
            value.AddMember(statsrapidjson::StringRef("sent_acc"), peer->stats.s11[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("sent_rej"), peer->stats.s11[i].cnt[1], allocator);
            break;

        case dRespRcvd:
            value.AddMember(statsrapidjson::StringRef("rcvd_acc"), peer->stats.s11[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd_rej"), peer->stats.s11[i].cnt[1], allocator);
            break;

        case dBoth:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.s11[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.s11[i].cnt[1], allocator);
            break;

        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), peer->stats.s11[i].cnt[0], allocator);
            break;
        }

	if((peer->stats.s11[i].ts != NULL) && (strlen(peer->stats.s11[i].ts) != 0))
		value.AddMember(statsrapidjson::StringRef("last"), statsrapidjson::StringRef(peer->stats.s11[i].ts), allocator);
        arrayObjects.PushBack(value, allocator);
    }
}

void CStatMessages::serializeS5S8(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    for(int i = 0; i < S5S8_MSG_TYPE_LEN; i++) {
        if((peer->stats.s5s8[i].cnt[0] == 0 && peer->stats.s5s8[i].cnt[1] == 0) && suppress) {
            continue;
        }
        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossS5s8MessageDefs[i].msgname), allocator);

        switch(ossS5s8MessageDefs[i].dir) {
        case dIn:
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.s5s8[i].cnt[0], allocator);
            break;

        case dOut:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.s5s8[i].cnt[0], allocator);
            break;

        case dRespSend:
            value.AddMember(statsrapidjson::StringRef("sent_acc"), peer->stats.s5s8[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("sent_rej"), peer->stats.s5s8[i].cnt[1], allocator);
            break;

        case dRespRcvd:
            value.AddMember(statsrapidjson::StringRef("rcvd_acc"), peer->stats.s5s8[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd_rej"), peer->stats.s5s8[i].cnt[1], allocator);
            break;

        case dBoth:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.s5s8[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.s5s8[i].cnt[1], allocator);
            break;

        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), peer->stats.s5s8[i].cnt[0], allocator);
            break;
        }

	if((peer->stats.s5s8[i].ts != NULL) && (strlen(peer->stats.s5s8[i].ts) != 0))
        	value.AddMember(statsrapidjson::StringRef("last"), statsrapidjson::StringRef(peer->stats.s5s8[i].ts), allocator);
        arrayObjects.PushBack(value, allocator);
    }
}

void CStatMessages::serializeSxa(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    for(int i = 0; i < SXA_MSG_TYPE_LEN; i++) {
        if((peer->stats.sxa[i].cnt[0] == 0 && peer->stats.sxa[i].cnt[1] == 0) && suppress) {
            continue;
        }
        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossSxaMessageDefs[i].msgname), allocator);

        switch(ossSxaMessageDefs[i].dir) {
        case dIn:
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.sxa[i].cnt[0], allocator);
            break;

        case dOut:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.sxa[i].cnt[0], allocator);
            break;

        case dRespSend:
            value.AddMember(statsrapidjson::StringRef("sent_acc"), peer->stats.sxa[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("sent_rej"), peer->stats.sxa[i].cnt[1], allocator);
            break;

        case dRespRcvd:
            value.AddMember(statsrapidjson::StringRef("rcvd_acc"), peer->stats.sxa[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd_rej"), peer->stats.sxa[i].cnt[1], allocator);
            break;

        case dBoth:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.sxa[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.sxa[i].cnt[1], allocator);
            break;

        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), peer->stats.sxa[i].cnt[0], allocator);
            break;
        }

	if((peer->stats.sxa[i].ts != NULL) && (strlen(peer->stats.sxa[i].ts) != 0))
        	value.AddMember(statsrapidjson::StringRef("last"), statsrapidjson::StringRef(peer->stats.sxa[i].ts), allocator);
        arrayObjects.PushBack(value, allocator);
    }
}

void CStatMessages::serializeSxb(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    for(int i = 0; i < SXB_MSG_TYPE_LEN; i++) {
        if((peer->stats.sxb[i].cnt[0] == 0 && peer->stats.sxb[i].cnt[1] == 0) && suppress) {
            continue;
        }
        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossSxbMessageDefs[i].msgname), allocator);

        switch(ossSxbMessageDefs[i].dir) {
        case dIn:
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.sxb[i].cnt[0], allocator);
            break;

        case dOut:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.sxb[i].cnt[0], allocator);
            break;

        case dRespSend:
            value.AddMember(statsrapidjson::StringRef("sent_acc"), peer->stats.sxb[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("sent_rej"), peer->stats.sxb[i].cnt[1], allocator);
            break;

        case dRespRcvd:
            value.AddMember(statsrapidjson::StringRef("rcvd_acc"), peer->stats.sxb[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd_rej"), peer->stats.sxb[i].cnt[1], allocator);
            break;

        case dBoth:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.sxb[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.sxb[i].cnt[1], allocator);
            break;

        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), peer->stats.sxb[i].cnt[0], allocator);
            break;
        }

    if((peer->stats.sxb[i].ts != NULL) && (strlen(peer->stats.sxb[i].ts) != 0))
        	value.AddMember(statsrapidjson::StringRef("last"), statsrapidjson::StringRef(peer->stats.sxb[i].ts), allocator);
        arrayObjects.PushBack(value, allocator);
    }
}

void CStatMessages::serializeSxaSxb(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    for(int i = 0; i < SXASXB_MSG_TYPE_LEN; i++) {
        if((peer->stats.sxasxb[i].cnt[0] == 0 && peer->stats.sxasxb[i].cnt[1] == 0) && suppress) {
            continue;
        }
        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossSxaSxbMessageDefs[i].msgname), allocator);

        switch(ossSxaSxbMessageDefs[i].dir) {
        case dIn:
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.sxasxb[i].cnt[0], allocator);
            break;

        case dOut:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.sxasxb[i].cnt[0], allocator);
            break;

        case dRespSend:
            value.AddMember(statsrapidjson::StringRef("sent_acc"), peer->stats.sxasxb[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("sent_rej"), peer->stats.sxasxb[i].cnt[1], allocator);
            break;

        case dRespRcvd:
            value.AddMember(statsrapidjson::StringRef("rcvd_acc"), peer->stats.sxasxb[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd_rej"), peer->stats.sxasxb[i].cnt[1], allocator);
            break;

        case dBoth:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.sxasxb[i].cnt[0], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.sxasxb[i].cnt[1], allocator);
            break;

        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), peer->stats.sxasxb[i].cnt[0], allocator);
            break;
        }

    if((peer->stats.sxasxb[i].ts != NULL) && (strlen(peer->stats.sxasxb[i].ts) != 0))
        	value.AddMember(statsrapidjson::StringRef("last"), statsrapidjson::StringRef(peer->stats.sxasxb[i].ts), allocator);
        arrayObjects.PushBack(value, allocator);
    }
}

void CStatMessages::serializeSystem(const cli_node_t *cli_node,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    // 0 == Number of active session so skip it.
    for(int i = 1; i < SYSTEM_MSG_TYPE_LEN; i++) {
        if((cli_node->stats[i] == 0) && suppress)
            continue;
        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossSystemMessageDefs[i].msgname), allocator);

        switch(ossSystemMessageDefs[i].dir) {
        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), cli_node->stats[i], allocator);
            break;
        }
        arrayObjects.PushBack(value, allocator);
    }
}

void CStatMessages::serialize(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{

    switch(peer->intfctype) {
    case itS11:
        serializeS11(peer, row, arrayObjects, allocator);
        break;
    case itS5S8:
        serializeS5S8(peer, row, arrayObjects, allocator);
        break;
    case itSxa:
    	serializeSxa(peer, row, arrayObjects, allocator);
        break;
    case itSxb:
    	serializeSxb(peer, row, arrayObjects, allocator);
        break;
    case itSxaSxb:
    	serializeSxaSxb(peer, row, arrayObjects, allocator);
        break;
    case itGx:
        cout << "CStatMessages Gx detected " << endl;
        break;
    }
}


void CStatHealth::serialize(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    statsrapidjson::Value reqValue(statsrapidjson::kObjectType);
    statsrapidjson::Value respValue(statsrapidjson::kObjectType);
    statsrapidjson::Value reqValues(statsrapidjson::kArrayType);
    statsrapidjson::Value respValues(statsrapidjson::kArrayType);

    row.AddMember(statsrapidjson::StringRef("responsetimeout"), peer->response_timeout, allocator);
    row.AddMember(statsrapidjson::StringRef("maxtimeouts"), peer->maxtimeout, allocator);
    row.AddMember(statsrapidjson::StringRef("timeouts"), peer->timeouts, allocator);

    reqValue.AddMember(statsrapidjson::StringRef("sent"), peer->hcrequest[0], allocator);
    reqValue.AddMember(statsrapidjson::StringRef("received"), peer->hcrequest[1], allocator);
    row.AddMember(statsrapidjson::StringRef("requests"), reqValue, allocator);

    respValue.AddMember(statsrapidjson::StringRef("sent"), peer->hcresponse[0], allocator);
    respValue.AddMember(statsrapidjson::StringRef("received"), peer->hcresponse[1], allocator);
    row.AddMember(statsrapidjson::StringRef("responses"), respValue, allocator);
}

void CStatPeers::serialize(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    statsrapidjson::Value ipv4Value(statsrapidjson::kObjectType);
    ipv4Value.AddMember(statsrapidjson::StringRef("ipv4"), statsrapidjson::Value(inet_ntoa(peer->ipaddr), allocator).Move(), allocator);
    row.AddMember(statsrapidjson::StringRef("address"), ipv4Value, allocator);
    if(peer->status)
       	row.AddMember(statsrapidjson::StringRef("active"), statsrapidjson::StringRef("true"), allocator);
    else
	row.AddMember(statsrapidjson::StringRef("active"), statsrapidjson::StringRef("false"), allocator);

	if(peer->lastactivity != NULL)
		row.AddMember(
        statsrapidjson::StringRef("lastactivity"), statsrapidjson::StringRef(peer->lastactivity), allocator);

    statsrapidjson::Value rowHealth(statsrapidjson::kObjectType);
    CStatHealth health(suppress);
    health.serialize(peer, rowHealth, arrayObjects, allocator);
    row.AddMember(statsrapidjson::Value(health.getNodeName().c_str(), allocator).Move(), rowHealth, allocator);
    statsrapidjson::Value msgValues(statsrapidjson::kArrayType);
    CStatMessages msg(suppress);
    msg.serialize(peer, row, msgValues, allocator);
    row.AddMember(statsrapidjson::Value(msg.getNodeName().c_str(), allocator).Move(), msgValues, allocator);
}

void CStatInterfaces::serializeInterface(cli_node_t *cli_node,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator,EInterfaceType it)
{
    statsrapidjson::Value value(statsrapidjson::kObjectType);
    value.AddMember(statsrapidjson::StringRef("name"), statsrapidjson::StringRef(ossInterfaceStr[it]), allocator);
    value.AddMember(statsrapidjson::StringRef("protocol"), statsrapidjson::StringRef(ossInterfaceProtocolStr[it]), allocator);
    statsrapidjson::Value peerValues(statsrapidjson::kArrayType);

    SPeer **oss_peer_ar = cli_node->peer;
    for(int i = 0; i < *(oss_cnt_peer); i++) {

        SPeer* peerPtrLoop = *(oss_peer_ar + i) ;
        if(peerPtrLoop == NULL)
        {
            continue;
        }

        if(peerPtrLoop->intfctype == it)
        {
			statsrapidjson::Value value1(statsrapidjson::kObjectType);
            peer.serialize(peerPtrLoop, value1, arrayObjects, allocator);
			peerValues.PushBack(value1, allocator);
        }
    }

    value.AddMember(statsrapidjson::Value(peer.getNodeName().c_str(), allocator).Move(), peerValues, allocator);
    arrayObjects.PushBack(value, allocator);
}


void CStatInterfaces::serialize(cli_node_t *cli_node,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    switch(cli_node->cp_type) {

    case SGWC:
            serializeInterface(cli_node,row,arrayObjects,allocator,itS11);
            serializeInterface(cli_node,row,arrayObjects,allocator,itS5S8);
            serializeInterface(cli_node,row,arrayObjects,allocator,itSxa);
        break;

    case PGWC:
            serializeInterface(cli_node,row,arrayObjects,allocator,itS5S8);
            serializeInterface(cli_node,row,arrayObjects,allocator,itSxb);
        break;

    case SAEGWC:
            serializeInterface(cli_node,row,arrayObjects,allocator,itS11);
            serializeInterface(cli_node,row,arrayObjects,allocator,itSxaSxb);
        break;
    }
}

void CStatGateway::serialize(cli_node_t *cli_node,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    STime now = STime::Now();
	initInterfaceDirection( static_cast<cp_config>(cli_node->cp_type));
    statsrapidjson::Value valArray(statsrapidjson::kArrayType);

    row.AddMember("nodename", statsrapidjson::StringRef(ossGatewayStr[cli_node->cp_type]), allocator);
    now.Format(reportTimeStr, "%Y-%m-%dT%H:%M:%S.%0", false);
    row.AddMember("reporttime", statsrapidjson::StringRef( reportTimeStr.c_str()  ), allocator);
    row.AddMember("upsecs", *(cli_node->upsecs), allocator);
    row.AddMember("resetsecs", oss_resetsec , allocator);
    interfaces.serialize(cli_node, row, valArray, allocator);
    row.AddMember(statsrapidjson::Value(interfaces.getNodeName().c_str(), allocator).Move(), valArray, allocator);

	statsrapidjson::Value sessionValue(statsrapidjson::kObjectType);
	CStatSession session(suppress);
    session.serialize(cli_node, sessionValue, arrayObjects, allocator);
    row.AddMember(statsrapidjson::Value(session.getNodeName().c_str(), allocator).Move(), sessionValue, allocator);

    statsrapidjson::Value systemValue(statsrapidjson::kObjectType);
    CStatSystem system(suppress);
    system.serialize(cli_node, systemValue, arrayObjects, allocator);
    row.AddMember(statsrapidjson::Value(system.getNodeName().c_str(), allocator).Move(), systemValue, allocator);
}

void CStatGateway::initInterfaceDirection(cp_config gatway)
{
    switch(gatway) {

    case SGWC:
        ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_SESSION_REQ]].dir = dOut;
        ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_SESSION_RSP]].dir = dRespRcvd;
        ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_SESSION_REQ]].dir = dOut;
        ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_SESSION_RSP]].dir = dRespRcvd;
        ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_REQ]].dir = dOut;
		ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_RSP]].dir = dRespRcvd;
		ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_BEARER_REQ]].dir = dIn;
		ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_BEARER_RSP]].dir = dOut;
        break;

    case PGWC:
        ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_SESSION_REQ]].dir = dIn;
        ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_SESSION_RSP]].dir = dRespSend;
        ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_SESSION_REQ]].dir = dIn;
        ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_SESSION_RSP]].dir = dRespSend;
        ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_REQ]].dir = dIn;
		ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_RSP]].dir = dRespSend;
		ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_BEARER_REQ]].dir = dOut;
		ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_BEARER_RSP]].dir = dIn;
        break;
    }
}

void CStatSystem::serialize(cli_node_t *cli_node,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    statsrapidjson::Value msgValueArr(statsrapidjson::kArrayType);
    CStatMessages msg(suppress);
    msg.serializeSystem(cli_node, row, msgValueArr, allocator);
	row.AddMember(statsrapidjson::Value(msg.getNodeName().c_str(), allocator).Move(), msgValueArr, allocator);
}

void CStatSession::serialize(cli_node_t *cli_node,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    row.AddMember(statsrapidjson::StringRef("active"),cli_node->stats[0], allocator);
}
