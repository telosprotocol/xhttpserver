// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xjson_proc.h"

namespace top 
{
    namespace httpserver 
    {
        bool xjson_proc::parse_json(const std::string& str_content)
        {
            if (!m_reader.parse(str_content, m_request_json)) 
            {
                SET_RESPONSE_CODE(xhttp_error_code::json_parser_error)
                return false;
            }
            if(m_request_json["action"].isNull())
            {
                SET_RESPONSE_CODE(xhttp_error_code::action_not_find)
                return false;
            }
            return true;
        }

        std::string xjson_proc::get_response()
        {
            return m_writer.write(m_response_json);
        }

        std::string xjson_proc::get_request()
        {
            return m_writer.write(m_request_json);
        }

        bool xjson_parse::parse_json(const std::string& str_content)
        {
            if (!m_reader.parse(str_content, m_json)) 
            {
                return false;
            }
            return true;
        }
    }
}