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
#include "simplewebserver/server_https.hpp"
#include "xserver_response_code.h"
#include "xjson_proc.h"
#include "xrule_manager.h"
#include "xaction_manager.h"
#ifdef HAVE_OPENSSL
#include "crypto.hpp"
#endif
namespace top 
{
    namespace httpserver 
    {
        using namespace std;
        using HttpsServer = SimpleWeb::Server<SimpleWeb::HTTPS>;


        class xhttps_server
        {
            public:
                void start(const std::string &cert_file, const std::string &private_key_file, uint16_t nPort, uint32_t nThreadNum = 1);
                void start_service(std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> response, std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> request);
                ~xhttps_server();
            private:
            std::shared_ptr<HttpsServer> m_pServer;
            std::thread m_server_thread;
            xrule_manager m_rule_mgr;
            xaction_manager m_action_mgr;
        };

    }
}