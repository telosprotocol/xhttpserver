// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <unordered_map>
namespace top 
{
    namespace httpserver 
    {
#define DO_TX_ACTION 100
        enum xhttp_action
        {
            account_balance = 0,
            account_info,
            account_history,
            account_pending,
            last_digest,
            tps_query,
            query_property,
            query_all_property,
            query_online_tx,
            query_reward,
            account_create = DO_TX_ACTION,
            transfer_out,
            transfer_in,
            give,
            set_property
        };
        static std::unordered_map<std::string, xhttp_action> g_action_map = {\
        {"account_balance", account_balance},\
        {"account_info", account_info},\
        {"account_create", account_create},\
        {"transfer_out", transfer_out},\
        {"transfer_in", transfer_in},\
        {"account_history", account_history},\
        {"account_pending", account_pending},\
        {"last_digest", last_digest},\
        {"tps_query", tps_query},\
        {"set_property", set_property},\
        {"query_property", query_property},\
        {"query_all_property", query_all_property},\
        {"give", give},\
        {"query_online_tx", query_online_tx},\
        {"query_reward", query_reward}
        };
        enum xhttp_error_code
        {
            ok = 0,
            json_parser_error,
            action_not_find,
            account_is_empty,
            property_key_is_empty,
            account_is_not_exist,
            property_key_is_not_exist,
            account_is_exist,
            signature_verify_fail,
            balance_is_not_enough,
            source_and_destination_same,
            last_hash_is_not_equal,
            miss_parameter,
            per_page_too_large,
            property_key_not_valid,
            already_give,
            server_unknown_error,
            request_time_out,
        };
        class xserver_response_code
        {
            public:
                std::string to_string() const;
            public:
            xhttp_error_code m_code;
        };

    }
}