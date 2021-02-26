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

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gw_adapter.h"
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
        if((peer->stats.s11[i].cnt[SENT] == 0 && peer->stats.s11[i].cnt[RCVD] == 0) && suppress) {
            continue;
        }

        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossS11MessageDefs[i].msgname), allocator);

        switch(ossS11MessageDefs[i].dir) {
        case dIn:
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.s11[i].cnt[RCVD], allocator);
            break;

        case dOut:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.s11[i].cnt[SENT], allocator);
            break;

        case dRespSend:
            value.AddMember(statsrapidjson::StringRef("sent_acc"), peer->stats.s11[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("sent_rej"), peer->stats.s11[i].cnt[REJ], allocator);
            break;

        case dRespRcvd:
            value.AddMember(statsrapidjson::StringRef("rcvd_acc"), peer->stats.s11[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd_rej"), peer->stats.s11[i].cnt[REJ], allocator);
            break;

        case dBoth:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.s11[i].cnt[SENT], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.s11[i].cnt[RCVD], allocator);
            break;

        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), peer->stats.s11[i].cnt[BOTH], allocator);
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
        if((peer->stats.s5s8[i].cnt[SENT] == 0 && peer->stats.s5s8[i].cnt[RCVD] == 0) && suppress) {
            continue;
        }
        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossS5s8MessageDefs[i].msgname), allocator);

        switch(ossS5s8MessageDefs[i].dir) {
        case dIn:
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.s5s8[i].cnt[RCVD], allocator);
            break;

        case dOut:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.s5s8[i].cnt[SENT], allocator);
            break;

        case dRespSend:
            value.AddMember(statsrapidjson::StringRef("sent_acc"), peer->stats.s5s8[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("sent_rej"), peer->stats.s5s8[i].cnt[REJ], allocator);
            break;

        case dRespRcvd:
            value.AddMember(statsrapidjson::StringRef("rcvd_acc"), peer->stats.s5s8[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd_rej"), peer->stats.s5s8[i].cnt[REJ], allocator);
            break;

        case dBoth:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.s5s8[i].cnt[SENT], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.s5s8[i].cnt[RCVD], allocator);
            break;

        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), peer->stats.s5s8[i].cnt[BOTH], allocator);
            break;
        }

	if((peer->stats.s5s8[i].ts != NULL) && (strlen(peer->stats.s5s8[i].ts) != 0))
        	value.AddMember(statsrapidjson::StringRef("last"), statsrapidjson::StringRef(peer->stats.s5s8[i].ts), allocator);
        arrayObjects.PushBack(value, allocator);
    }
}

void CStatMessages::serializeSx(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    for(int i = 0; i < SX_MSG_TYPE_LEN; i++) {
        if((peer->stats.sx[i].cnt[SENT] == 0 && peer->stats.sx[i].cnt[RCVD] == 0) && suppress) {
            continue;
        }
        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossSxMessageDefs[i].msgname), allocator);

        switch(ossSxMessageDefs[i].dir) {
        case dIn:
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.sx[i].cnt[RCVD], allocator);
            break;

        case dOut:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.sx[i].cnt[SENT], allocator);
            break;

        case dRespSend:
            value.AddMember(statsrapidjson::StringRef("sent_acc"), peer->stats.sx[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("sent_rej"), peer->stats.sx[i].cnt[REJ], allocator);
            break;

        case dRespRcvd:
            value.AddMember(statsrapidjson::StringRef("rcvd_acc"), peer->stats.sx[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd_rej"), peer->stats.sx[i].cnt[REJ], allocator);
            break;

        case dBoth:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.sx[i].cnt[SENT], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.sx[i].cnt[RCVD], allocator);
            break;

        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), peer->stats.sx[i].cnt[BOTH], allocator);
            break;
        }

    if((peer->stats.sx[i].ts != NULL) && (strlen(peer->stats.sx[i].ts) != 0))
        	value.AddMember(statsrapidjson::StringRef("last"), statsrapidjson::StringRef(peer->stats.sx[i].ts), allocator);
        arrayObjects.PushBack(value, allocator);
    }
}

void CStatMessages::serializeGx(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    for(int i = 0; i < GX_MSG_TYPE_LEN; i++) {
        if((peer->stats.gx[i].cnt[SENT] == 0 && peer->stats.gx[i].cnt[RCVD] == 0) && suppress) {
            continue;
        }
        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossGxMessageDefs[i].msgname), allocator);

        switch(ossGxMessageDefs[i].dir) {
        case dIn:
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.gx[i].cnt[RCVD], allocator);
            break;

        case dOut:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.gx[i].cnt[SENT], allocator);
            break;

        case dRespSend:
            value.AddMember(statsrapidjson::StringRef("sent_acc"), peer->stats.gx[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("sent_rej"), peer->stats.gx[i].cnt[REJ], allocator);
            break;

        case dRespRcvd:
            value.AddMember(statsrapidjson::StringRef("rcvd_acc"), peer->stats.gx[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd_rej"), peer->stats.gx[i].cnt[REJ], allocator);
            break;

        case dBoth:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.gx[i].cnt[SENT], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.gx[i].cnt[RCVD], allocator);
            break;

        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), peer->stats.gx[i].cnt[BOTH], allocator);
            break;
        }

    if((peer->stats.gx[i].ts != NULL) && (strlen(peer->stats.gx[i].ts) != 0))
               value.AddMember(statsrapidjson::StringRef("last"), statsrapidjson::StringRef(peer->stats.gx[i].ts), allocator);
        arrayObjects.PushBack(value, allocator);
    }
}


void CStatMessages::serializeGx(const SPeer* peer,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    for(int i = 0; i < GX_MSG_TYPE_LEN; i++) {
        if((peer->stats.gx[i].cnt[SENT] == 0 && peer->stats.gx[i].cnt[RCVD] == 0) && suppress) {
            continue;
        }
        statsrapidjson::Value value(statsrapidjson::kObjectType);
        value.AddMember(
            statsrapidjson::StringRef("type"), statsrapidjson::StringRef(ossGxMessageDefs[i].msgname), allocator);

        switch(ossGxMessageDefs[i].dir) {
        case dIn:
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.gx[i].cnt[RCVD], allocator);
            break;

        case dOut:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.gx[i].cnt[SENT], allocator);
            break;

        case dRespSend:
            value.AddMember(statsrapidjson::StringRef("sent_acc"), peer->stats.gx[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("sent_rej"), peer->stats.gx[i].cnt[REJ], allocator);
            break;

        case dRespRcvd:
            value.AddMember(statsrapidjson::StringRef("rcvd_acc"), peer->stats.gx[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd_rej"), peer->stats.gx[i].cnt[REJ], allocator);
            break;

        case dBoth:
            value.AddMember(statsrapidjson::StringRef("sent"), peer->stats.gx[i].cnt[ACC], allocator);
            value.AddMember(statsrapidjson::StringRef("rcvd"), peer->stats.gx[i].cnt[REJ], allocator);
            break;

        case dNone:
            value.AddMember(statsrapidjson::StringRef("count"), peer->stats.gx[i].cnt[BOTH], allocator);
            break;
        }

    if((peer->stats.gx[i].ts != NULL) && (strlen(peer->stats.gx[i].ts) != 0))
               value.AddMember(statsrapidjson::StringRef("last"), statsrapidjson::StringRef(peer->stats.gx[i].ts), allocator);
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
		if (cli_node_ptr->gw_type == OSS_CONTROL_PLANE)
        serializeS5S8(peer, row, arrayObjects, allocator);
        break;
    case itSx:
    	serializeSx(peer, row, arrayObjects, allocator);
        break;
    case itGx:
	serializeGx(peer, row, arrayObjects, allocator);
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

    row.AddMember(statsrapidjson::StringRef("responsetimeout"), (*(peer->response_timeout)), allocator);
    row.AddMember(statsrapidjson::StringRef("maxtimeouts"), (*(peer->maxtimeout))+1, allocator);
    row.AddMember(statsrapidjson::StringRef("timeouts"), peer->timeouts, allocator);

    reqValue.AddMember(statsrapidjson::StringRef("sent"), peer->hcrequest[SENT], allocator);
    reqValue.AddMember(statsrapidjson::StringRef("received"), peer->hcrequest[RCVD], allocator);
    row.AddMember(statsrapidjson::StringRef("requests"), reqValue, allocator);

    respValue.AddMember(statsrapidjson::StringRef("sent"), peer->hcresponse[SENT], allocator);
    respValue.AddMember(statsrapidjson::StringRef("received"), peer->hcresponse[RCVD], allocator);
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
    switch(cli_node->gw_type) {

    case OSS_CONTROL_PLANE:
        serializeInterface(cli_node,row,arrayObjects,allocator,itS11);
        serializeInterface(cli_node,row,arrayObjects,allocator,itS5S8);
        serializeInterface(cli_node,row,arrayObjects,allocator,itSx);
		serializeInterface(cli_node,row,arrayObjects,allocator,itGx);
        break;

    case OSS_USER_PLANE:
	serializeInterface(cli_node,row,arrayObjects,allocator,itSx);
	serializeInterface(cli_node,row,arrayObjects,allocator,itS1U);
	serializeInterface(cli_node,row,arrayObjects,allocator,itS5S8);
	serializeInterface(cli_node,row,arrayObjects,allocator,itSGI);
	break;

    }
}

void CStatGateway::serialize(cli_node_t *cli_node,
    statsrapidjson::Value& row,
    statsrapidjson::Value& arrayObjects,
    statsrapidjson::Document::AllocatorType& allocator)
{
    STime now = STime::Now();
	initInterfaceDirection( static_cast<cp_config>(cli_node->gw_type));
    statsrapidjson::Value valArray(statsrapidjson::kArrayType);

    row.AddMember("nodename", statsrapidjson::StringRef(ossGatewayStr[cli_node->gw_type]), allocator);
    now.Format(reportTimeStr, "%Y-%m-%dT%H:%M:%S.%0", false);
    row.AddMember("reporttime", statsrapidjson::StringRef( reportTimeStr.c_str()  ), allocator);
    row.AddMember("upsecs", *(cli_node->upsecs), allocator);
    row.AddMember("resetsecs", *(cli_node->upsecs) - reset_time , allocator);
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

		case OSS_CONTROL_PLANE:
			if(cli_node_ptr->s5s8_selection == OSS_S5S8_SENDER) {
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_SESSION_REQ]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_SESSION_RSP]].dir = dRespRcvd;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_SESSION_REQ]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_SESSION_RSP]].dir = dRespRcvd;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_REQ]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_RSP]].dir = dRespRcvd;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_BEARER_REQ]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_BEARER_RSP]].dir = dRespSend;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_UPDATE_BEARER_REQ]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_UPDATE_BEARER_RSP]].dir = dRespSend;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_BEARER_REQ]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_BEARER_RSP]].dir = dRespSend;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CHANGE_NOTIFICATION_REQ]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CHANGE_NOTIFICATION_RSP]].dir = dRespRcvd;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_PDN_CONNECTION_SET_REQ]].dir = dBoth;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_UPDATE_PDN_CONNECTION_SET_REQ]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_UPDATE_PDN_CONNECTION_SET_RSP]].dir = dRespRcvd;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_BEARER_CMD]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_BEARER_FAILURE_IND]].dir = dRespRcvd;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_CMD]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_FAILURE_IND]].dir = dRespRcvd;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_BEARER_RESOURCE_CMD]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_BEARER_RESOURCE_FAILURE_IND]].dir = dRespRcvd;
			}
			if(cli_node_ptr->s5s8_selection == OSS_S5S8_RECEIVER) {
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_SESSION_REQ]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_SESSION_RSP]].dir = dRespSend;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_SESSION_REQ]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_SESSION_RSP]].dir = dRespSend;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_REQ]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_RSP]].dir = dRespSend;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_BEARER_REQ]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CREATE_BEARER_RSP]].dir = dRespRcvd;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_UPDATE_BEARER_REQ]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_UPDATE_BEARER_RSP]].dir = dRespRcvd;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_BEARER_REQ]].dir = dOut;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_BEARER_RSP]].dir = dRespRcvd;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CHANGE_NOTIFICATION_REQ]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_CHANGE_NOTIFICATION_RSP]].dir = dRespSend;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_PDN_CONNECTION_SET_REQ]].dir = dBoth;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_UPDATE_PDN_CONNECTION_SET_REQ]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_UPDATE_PDN_CONNECTION_SET_RSP]].dir = dRespSend;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_BEARER_CMD]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_DELETE_BEARER_FAILURE_IND]].dir = dRespSend;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_CMD]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_MODIFY_BEARER_FAILURE_IND]].dir = dRespSend;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_BEARER_RESOURCE_CMD]].dir = dIn;
				ossS5s8MessageDefs[s5s8MessageTypes[GTP_BEARER_RESOURCE_FAILURE_IND]].dir = dRespSend;
			}
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
