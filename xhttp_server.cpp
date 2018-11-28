// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xhttp_server.h"


namespace top
{
	namespace httpserver
	{
        void xhttp_server::start(uint16_t nPort, uint32_t nThreadNum)
        {
            m_server.config.port = nPort;
            m_server.config.thread_pool_size = nThreadNum;
            
            m_server.resource["^/$"]["POST"] = std::bind(&xhttp_server::start_service, this, std::placeholders::_1, std::placeholders::_2);
            m_server.resource["^/$"]["OPTIONS"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
                *response << "HTTP/1.1 200 OK\r\n";
                *response << "Access-Control-Allow-Origin:*\r\n";
                *response << "Access-Control-Request-Method:POST\r\n";
                *response << "Access-Control-Allow-Headers:Origin,X-Requested-With,Content-Type,Accept\r\n";
                *response << "Access-Control-Request-Headers:Content-type\r\n";
            };
            m_server.on_error = [](std::shared_ptr<HttpServer::Request> request, const SimpleWeb::error_code & ec) {
                // Handle errors here
                // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
                //xwarn("on_error:%s,%d", ec.code().message().c_str(), ec.code().value());
            };
            m_server_thread = std::thread([this]() {
                // Start server
                this->m_server.start();
            });

            //init rpc proc handler
            m_forward_msg_handler = std::make_shared<xforward_msg_handler>();
            m_forward_msg_handler->init();
            std::shared_ptr<xmessage_handler> forward_msg_handler = m_forward_msg_handler;
			xmessage_dispatcher::get_instance().register_handler(top::enum_xtop_msg_type_rpc_request, forward_msg_handler);
			xmessage_dispatcher::get_instance().register_handler(top::enum_xtop_msg_type_rpc_reply, forward_msg_handler);
            xmessage_dispatcher::get_instance().register_handler(top::enum_xtop_msg_type_reply_message, forward_msg_handler);

        }
        void xhttp_server::forward_msg(std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Request> request)
        {
            auto content = request->content.string();

        }

        bool xhttp_server::deal_msg(const std::string& request_content, xjson_proc& json_proc)
        {
            if(!m_rule_mgr.filter(json_proc))
            {
                return false;
            }
            if(!m_action_mgr.do_action(json_proc))
            {
                return false;
            }
            if(json_proc.m_tx != NULL)
            {
                m_action_mgr.do_transaction(json_proc.m_tx);
                SET_JSON_RESPONSE_OK
            }
            return true;
        }


        void xhttp_server::start_service(std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Request> request)
        {
            //check node type,if it's edge and forward message
            auto content = request->content.string();
            xinfo("rpc request:%s", content.c_str());
            xjson_proc json_proc;
            if(!json_proc.parse_json(content))
            {
                response->write(json_proc.get_response());
                return;
            }
            if(elect::enum_node_type_rpc == xconfig::get_instance().m_node_type && (json_proc.m_request_json["local"].isNull() || "1" != json_proc.m_request_json["local"].asString()))
            {
                m_forward_msg_handler->send_request(response, json_proc, *(m_server.io_service));
            }
            else
            {
                deal_msg(content, json_proc);
                std::string result = json_proc.get_response();
                response->write(result);
                xinfo("rpc response:%s", result.c_str());
            }

            //SET_JSON_PROC_RESPONSE(xhttp_error_code::server_unknown_error)
            //response->write(json_proc.get_response());
        }
		xhttp_server::~xhttp_server()
		{
            m_server_thread.join();
		}

    }
}