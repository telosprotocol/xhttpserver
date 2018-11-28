// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include<unordered_map>
#include "xnet.h"
#include "xmessage_dispatcher.hpp"
#include "xnode_manager.hpp"
#include "xaction_manager.h"
#include "xjson_proc.h"
#include "xrule_manager.h"
#include "xhttp_server.h"
namespace top 
{
    namespace httpserver 
    {
#define TIME_OUT 5
#define BROADCAST_TTL 1
#define BROADCAST_NUM 1

        class xforward_session;
        class xforward_session_mgr
        {
            public:
            std::unordered_map<std::string, std::shared_ptr<xforward_session>> m_response_session;
            std::mutex m_mutex;
        };
        class xforward_session : public std::enable_shared_from_this<xforward_session>
        {
            public:
            std::string m_uuid;
            std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> m_response;
            std::unique_ptr<asio::steady_timer> m_timer;
            std::string m_reply_result[BROADCAST_NUM];
            std::shared_ptr<top::chain::xtransaction_t> m_tx;
            int16_t m_response_num = 0;
            int16_t m_retry_num = 0;
            void set_timeout(long seconds, asio::io_service& ioc, std::shared_ptr<xforward_session_mgr>& session_mgr) noexcept 
            {
                if(seconds == 0) 
                {
                    m_timer = nullptr;
                    return;
                }
                if(m_timer == NULL)
                {
                    m_timer = std::unique_ptr<asio::steady_timer>(new asio::steady_timer(ioc));
                }
                m_timer->expires_from_now(std::chrono::seconds(seconds));
                auto self = this->shared_from_this();
                m_timer->async_wait([self, &ioc, &session_mgr](const error_code &ec) {
                if(!ec)
                {
                    std::unique_lock<std::mutex> lock(session_mgr->m_mutex);
                    if(self->m_retry_num-- > 0 && self->m_tx != NULL)
                    {
                        //change edge timestamp to retry
                        top::xtop_message_t msg;
                        msg.m_msg_type = top::enum_xtop_msg_type_proposal_message;
                        msg.m_content = self->m_tx->serialize_no_type();
                        msg.m_transaction_unique_seqno = get_transaction_unique_id();
                        self->m_tx->to_log("retry rpc:" + get_message_uuid_string(msg));
                        elect::xshard_info shard_info = std::move(elect::xelect_manager_intf::get_instance()->get_account_shard_info(self->m_tx->m_sender_addr));
                        xmessage_dispatcher::get_instance().broadcast(msg, enum_xtop_node_type_consensus_node, shard_info);
                        self->set_timeout(TIME_OUT, ioc, session_mgr);
                    }
                    else
                    {
                        self->m_response->write(TIMEOUT_RESPONSE);
                        session_mgr->m_response_session.erase(self->m_uuid);
                    }

                }
                });
            }
            void cancel_timeout() noexcept 
            {
                if(m_timer) 
                {
                    error_code ec;
                    m_timer->cancel(ec);
                }
            }
        };

        class xforward_msg_handler : public top::xmessage_handler
        {
            public:
            int32_t init();
            void on_message(const top::xtop_message_t & msg, const top::xtop_node & from);
            bool deal_msg(const std::string& request_content, xjson_proc& json_proc);
            int32_t reply_query_msg(const top::xtop_message_t & msg);
            int32_t reply_consensus_msg(const top::xtop_message_t & msg);
            int32_t send_request(std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> response, xjson_proc& json_proc, asio::io_service& ioc);
            int32_t cmp_msg(const std::shared_ptr<xforward_session> & forward_session);
            public:
            std::shared_ptr<xforward_session_mgr> m_session_mgr;
            xrule_manager m_rule_mgr;
            xaction_manager m_action_mgr;
        };


    }
}