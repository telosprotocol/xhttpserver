// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xserver_response_code.h"

namespace top 
{
    namespace httpserver 
    {
        std::string xserver_response_code::to_string() const
        {
            std::string result;
            switch (m_code) 
            {
                case ok:
                    result = "ok";
                    break;
                case json_parser_error:
                    result = "json parser error: ";
                    break;
                case action_not_find:
                    result = "action is empty";
                    break;
                case account_is_empty:
                    result = "account is empty";
                    break;
                case property_key_is_empty:
                    result = "property_key is empty";
                    break;
                case account_is_not_exist:
                    result = "account is not exist";
                    break;
                case property_key_is_not_exist:
                    result = "property_key is not exist";
                    break;
                case account_is_exist:
                    result = "account is exist";
                    break;
                case signature_verify_fail:
                    result = "signature verify fail";
                    break;
                case balance_is_not_enough:
                    result = "balance is not enough";
                    break;
                case source_and_destination_same:
                    result = "source and destination is the same";
                    break;
                case last_hash_is_not_equal:
                    result = "last_hash is not equal";
                    break;
                case miss_parameter:
                    result = "miss parameter";
                    break;
                case per_page_too_large:
                    result = "per_page too large";
                    break;
                case property_key_not_valid:
                    result = "property_key not valid";
                    break;
                case already_give:
                    result = "already give";
                    break;
                case server_unknown_error:
                    result = "server unknown error";
                    break;
                case request_time_out:
                    result = "request time out";
                    break;
                default:
                    result = "Unknown code " + std::to_string(m_code);
                    break;
            }
            return result;
        }
    }
}