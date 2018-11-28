// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xjson_proc.h"
#include "xnet.h"
#include "xmessage_dispatcher.hpp"
#include "xstore_manager.h"
#include "xconfig.hpp"
#include "xlog.h"
#include "xstring.h"
#include "xtimer.h"
#include "xreward_intf.h"
namespace top 
{
    namespace httpserver 
    {
#define TAG_NAME "@name"

        static uint64_t get_timestamp()
        {
            struct timeval val;
            gettimeofday(&val, NULL);
            return (uint64_t)val.tv_sec;
        }
        static uint64_t get_transaction_unique_id()
        {
            return get_timestamp() + (uint64_t)rand();
        }
        class xaction_manager
        {
            public:
            xaction_manager(){m_store_intf = xsingleton<top::store::xstore_manager>::Instance();}
            bool do_action(xjson_proc& json_proc);
            bool query_balance(xjson_proc& json_proc);
            bool query_account_info(xjson_proc& json_proc);
            bool account_create(xjson_proc& json_proc);
            uint32_t do_transaction(const std::shared_ptr<top::chain::xtransaction_t>& tx);
            bool set_and_check(xjson_proc& json_proc);
            bool transfer_out(xjson_proc& json_proc);
            bool transfer_in(xjson_proc& json_proc);
            bool account_history(xjson_proc& json_proc);
            void set_property_value(std::unordered_map<std::string, std::string>& user_tag, xJson::Value& block_json);
            bool account_pending(xjson_proc& json_proc);
            bool last_digest(xjson_proc& json_proc);
            bool tps_query(xjson_proc& json_proc);
            bool set_property(xjson_proc& json_proc);
            bool query_property(xjson_proc& json_proc);
            bool query_all_property(xjson_proc& json_proc);
            bool give(xjson_proc& json_proc);
            bool get_genesis_sign(std::shared_ptr<top::chain::xtransaction_t>& tx);
            bool query_online_tx(xjson_proc& json_proc);
            bool query_reward(xjson_proc& json_proc);
            public:
            top::store::xstore_base* m_store_intf;
        };
    }
}