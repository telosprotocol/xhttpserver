// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <memory>
#include "json/json.h"
#include "xserver_response_code.h"
#include "xchaincore.h"
#include "xstring.h"
namespace top 
{
    namespace httpserver 
    {
#define SET_RESPONSE_CODE(error_code) m_response_code.m_code = error_code;\
                m_response_json["result"] = m_response_code.m_code;\
                m_response_json["msg"] = m_response_code.to_string();

#define SET_JSON_RESPONSE_OK json_proc.m_response_code.m_code = xhttp_error_code::ok;\
                json_proc.m_response_json["result"] = json_proc.m_response_code.m_code;\
                json_proc.m_response_json["msg"] = json_proc.m_response_code.to_string();

#define SET_JSON_PROC_RESPONSE(error_code)                json_proc.m_response_code.m_code = error_code;\
                json_proc.m_response_json["result"] = json_proc.m_response_code.m_code;\
                json_proc.m_response_json["msg"] = json_proc.m_response_code.to_string();
#define TIMEOUT_RESPONSE "{\"result\":99, \"msg\":\"request time out\"}"

        inline std::string set_response_from_consensus(const chain::xconsensus_reply_t& consensus_reply)
        {
            xJson::FastWriter writer;
            xJson::Value result;
            result["result"]    = consensus_reply.m_result_code;
            result["msg"]       = consensus_reply.m_result_msg;
            result["hash_id"]   = string_utl::base64_encode(consensus_reply.m_tx_hash.data(), consensus_reply.m_tx_hash.size());
            return writer.write(result);
        }
        class xjson_proc
        {
            public:
            bool parse_json(const std::string& str_content);
            std::string get_response();
            std::string get_request();
            public:
            xJson::Reader m_reader;
            xJson::FastWriter m_writer;
            xJson::Value m_request_json;
            xJson::Value m_response_json;
            xserver_response_code m_response_code;
            std::shared_ptr<top::chain::xtransaction_t> m_tx;
        };


        class xjson_parse
        {
            public:
            bool parse_json(const std::string& str_content);
            public:
            xJson::Reader m_reader;
            xJson::Value m_json;
        };


    }
}