// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xhttps_server.h"


namespace top
{
	namespace httpserver
	{
        void xhttps_server::start(const std::string &cert_file, const std::string &private_key_file, uint16_t nPort, uint32_t nThreadNum)
        {
            m_pServer = std::make_shared<HttpsServer>(cert_file,private_key_file);
            m_pServer->config.port = nPort;
            m_pServer->config.thread_pool_size = nThreadNum;
            
            m_pServer->resource["^/$"]["POST"] = std::bind(&xhttps_server::start_service, this, std::placeholders::_1, std::placeholders::_2);
            m_pServer->resource["^/$"]["OPTIONS"] = [](shared_ptr<HttpsServer::Response> response, shared_ptr<HttpsServer::Request> request) {
                *response << "HTTP/1.1 200 OK\r\n";
                *response << "Access-Control-Allow-Origin:*\r\n";
                *response << "Access-Control-Request-Method:POST\r\n";
                *response << "Access-Control-Allow-Headers:Origin,X-Requested-With,Content-Type,Accept\r\n";
                *response << "Access-Control-Request-Headers:Content-type\r\n";
            };
            m_pServer->on_error = [](std::shared_ptr<HttpsServer::Request> request, const SimpleWeb::error_code & ec) {
                // Handle errors here
                // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
                //xwarn("on_error:%s,%d", ec.code().message().c_str(), ec.code().value());
            };
            m_server_thread = std::thread([this]() {
                // Start server
                this->m_pServer->start();
            });


        }
        void xhttps_server::start_service(std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Response> response, std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTPS>::Request> request)
        {
                // Retrieve string:
            auto content = request->content.string();
            xjson_proc json_proc;
            if(!json_proc.parse_json(content))
            {
                response->write(json_proc.get_response());
                return;
            }
            if(!m_rule_mgr.filter(json_proc))
            {
                response->write(json_proc.get_response());
                return;
            }
            if(m_action_mgr.do_action(json_proc))
            {
                response->write(json_proc.get_response());
                return;
            }

            SET_JSON_PROC_RESPONSE(xhttp_error_code::server_unknown_error)
            response->write(json_proc.get_response());
        }
		xhttps_server::~xhttps_server()
		{
            m_server_thread.join();
		}

    }
}