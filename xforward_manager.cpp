// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xforward_manager.h"

namespace top 
{
    namespace httpserver 
    {
        int32_t xforward_msg_handler::init()
        {
            m_session_mgr = std::make_shared<xforward_session_mgr>();
            return 0;
        }

        void xforward_msg_handler::on_message(const top::xtop_message_t & msg, const top::xtop_node & from)
        {
            xinfo("rpc forward:%d,%s", msg.m_msg_type, msg.m_content.c_str());
            switch (msg.m_msg_type)
            {
                case (top::enum_xtop_msg_type::enum_xtop_msg_type_rpc_request):
                {
                    xjson_proc json_proc;
                    deal_msg(msg.m_content, json_proc);
                    //add origin uuid to content
                    json_proc.m_response_json["uuid"] = get_message_uuid_string(msg);
                    std::string response = std::move(json_proc.get_response());

                    top::xtop_node to_node = xnode_manager::get_instance().query_node_by_account(msg.m_origin_account);
                    xtop_message_t send_msg;
                    send_msg.m_msg_type = top::enum_xtop_msg_type::enum_xtop_msg_type_rpc_reply;
                    send_msg.m_content = response;
                    xmessage_dispatcher::get_instance().send(send_msg, to_node);
                    xinfo("rpc response:%s,%s", to_node.get_key_str().c_str(), response.c_str());
                    break;
                }
                case (top::enum_xtop_msg_type::enum_xtop_msg_type_rpc_reply):
                {
                    reply_query_msg(msg);
                    break;
                }
                case (top::enum_xtop_msg_type::enum_xtop_msg_type_reply_message):
                {
                    reply_consensus_msg(msg);
                    break;
                }
                default:
                    xerror("xforward_msg_handler msg type error:%d", msg.m_msg_type);
                    break;
            }
        }

        int32_t xforward_msg_handler::reply_query_msg(const top::xtop_message_t & msg)
        {
            xjson_parse json_parse;
            if(json_parse.parse_json(msg.m_content) && !json_parse.m_json["uuid"].isNull())
            {
                std::string uuid = json_parse.m_json["uuid"].asString();
                std::unique_lock<std::mutex> lock(m_session_mgr->m_mutex);
                auto iter = m_session_mgr->m_response_session.find(uuid);
                if(iter != m_session_mgr->m_response_session.end())
                {
                    iter->second->m_reply_result[iter->second->m_response_num++] = msg.m_content;
                    if( iter->second->m_response_num >= BROADCAST_NUM)
                    {
                        iter->second->cancel_timeout();
                        iter->second->m_response->write(msg.m_content);
                        m_session_mgr->m_response_session.erase(uuid);
                    }
                }
                else
                {
                    xwarn("query uuid %s can't find session", uuid.c_str());
                }
                return 0;
            }
            else
            {
                xwarn("reply_query_msg error:%s", msg.m_content.c_str());
                return -1;
            }
        }

        int32_t xforward_msg_handler::reply_consensus_msg(const top::xtop_message_t & msg)
        {
            chain::xconsensus_reply_t consensus_reply;
            consensus_reply.unserialize(msg.m_content);
            std::string uuid((char*)consensus_reply.m_tx_hash.data(), consensus_reply.m_tx_hash.size());
            std::unique_lock<std::mutex> lock(m_session_mgr->m_mutex);
            auto iter = m_session_mgr->m_response_session.find(uuid);
            if(iter != m_session_mgr->m_response_session.end())
            {
                iter->second->cancel_timeout();
                iter->second->m_response->write(set_response_from_consensus(consensus_reply));
                m_session_mgr->m_response_session.erase(uuid);
            }
            else
            {
                xwarn("consensus uuid %s can't find session", chain::str_to_hex(uuid).c_str());
            }
            return 0;
        }

        bool xforward_msg_handler::deal_msg(const std::string& request_content, xjson_proc& json_proc)
        {
           if(!json_proc.parse_json(request_content))
            {
                return false;
            }
            if(!m_rule_mgr.filter(json_proc))
            {
                return false;
            }
            if(m_action_mgr.do_action(json_proc))
            {
                return false;
            }
            return true;
        }

        int32_t xforward_msg_handler::send_request(std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> response, xjson_proc& json_proc, asio::io_service& ioc)
        {
            auto iter = g_action_map.find(json_proc.m_request_json["action"].asString());
            if(iter == g_action_map.end())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::action_not_find)
                response->write(json_proc.get_response());
                return -1;
            }
            if(iter->second >= DO_TX_ACTION)//do tx
            {
                if(m_action_mgr.do_action(json_proc) && json_proc.m_tx != NULL)
                {
                    std::string uuid((char*)json_proc.m_tx->m_tx_digest.data(), json_proc.m_tx->m_tx_digest.size());
                    std::shared_ptr<xforward_session> forward_session = make_shared<xforward_session>();
                    forward_session->m_uuid = uuid;
                    forward_session->m_response = response;
                    forward_session->m_retry_num = 1;
                    forward_session->m_tx = json_proc.m_tx;
                    forward_session->set_timeout(TIME_OUT, ioc, m_session_mgr);
                    m_session_mgr->m_response_session.insert(make_pair(uuid, forward_session));

                    top::xtop_message_t msg;
                    msg.m_msg_type = top::enum_xtop_msg_type_proposal_message;
                    msg.m_content = json_proc.m_tx->serialize_no_type();
                    msg.m_transaction_unique_seqno = get_transaction_unique_id();
                    json_proc.m_tx->to_log("rpc:" + get_message_uuid_string(msg));
  
                    elect::xshard_info shard_info = std::move(elect::xelect_manager_intf::get_instance()->get_account_shard_info(json_proc.m_tx->m_sender_addr));
                    xmessage_dispatcher::get_instance().broadcast(msg, enum_xtop_node_type_consensus_node, shard_info);
                }
                else
                {
                    xwarn("do tx is error");
                    SET_JSON_PROC_RESPONSE(xhttp_error_code::server_unknown_error)
                    response->write(json_proc.get_response());
                    return -1;
                }
            }
            else
            {
                xtop_message_t msg(BROADCAST_TTL);
                std::string uuid = get_message_uuid_string(msg);
                json_proc.m_request_json["uuid"] = uuid;

                msg.m_msg_type = top::enum_xtop_msg_type::enum_xtop_msg_type_rpc_request;
                msg.m_content = json_proc.get_request();

                std::shared_ptr<xforward_session> forward_session = make_shared<xforward_session>();
                forward_session->m_uuid = uuid;
                forward_session->m_response = response;
                forward_session->set_timeout(TIME_OUT, ioc, m_session_mgr);
                m_session_mgr->m_response_session.insert(make_pair(uuid, forward_session));

                elect::xshard_info shard_info = std::move(elect::xelect_manager_intf::get_instance()->get_account_shard_info(json_proc.m_request_json["account"].asString()));
                xmessage_dispatcher::get_instance().broadcast(msg, enum_xtop_node_type_consensus_node, shard_info, BROADCAST_NUM);
            }
            return 0;
        }
 

    }
}