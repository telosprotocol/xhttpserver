// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif//ASIO_STANDALONE
#ifndef USE_STANDALONE_ASIO
#define USE_STANDALONE_ASIO
#endif//ASIO_STANDALONE
#include <algorithm>
#include <fstream>
#include <vector>
#include <memory>
#include <thread>
#include "json/json.h"
#include "simplewebserver/server_http.hpp"
#include "xserver_response_code.h"
#include "xjson_proc.h"
#include "xrule_manager.h"
#include "xaction_manager.h"
#include "xforward_manager.h"
#ifdef HAVE_OPENSSL
#include "crypto.hpp"
#endif
namespace top 
{
    namespace httpserver 
    {
        using namespace std;
        using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
        class xforward_msg_handler;
        class xhttp_server
        {
            public:
                void start(uint16_t nPort, uint32_t nThreadNum = 1);
                void start_service(std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Request> request);
                void forward_msg(std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Request> request);
                bool deal_msg(const std::string& request_content, xjson_proc& json_proc);
                ~xhttp_server();
            private:
            HttpServer m_server;
            std::thread m_server_thread;
            xrule_manager m_rule_mgr;
            xaction_manager m_action_mgr;
            std::shared_ptr<xforward_msg_handler> m_forward_msg_handler;
        };
    }
}