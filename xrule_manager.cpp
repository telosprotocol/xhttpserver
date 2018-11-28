// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xrule_manager.h"

namespace top 
{
    namespace httpserver 
    {
        bool xrule_manager::account_balance_filter(xjson_proc& json_proc)
        {
            if(json_proc.m_request_json["account"].isNull())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_empty)
                return false;
            }
            if(json_proc.m_request_json["property_key"].isNull())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::property_key_is_empty)
                return false;
            }
            return true;
        }

        bool xrule_manager::account_info_filter(xjson_proc& json_proc)
        {
            if(json_proc.m_request_json["account"].isNull())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_empty)
                return false;
            }
            return true;
        }

        bool xrule_manager::account_create_filter(xjson_proc& json_proc)
        {
            if(json_proc.m_request_json["account"].isNull())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_empty)
                return false;
            }
            if(m_store_intf->exist_account(json_proc.m_request_json["account"].asString()))
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_exist)
                return false;
            }
            return true;
        }

        bool xrule_manager::transfer_out_filter(xjson_proc& json_proc)
        {
            if(json_proc.m_request_json["source"].isNull())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_empty)
                return false;
            }
            if(json_proc.m_request_json["destination"].isNull())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_empty)
                return false;
            }
            if(json_proc.m_request_json["source"].asString() == json_proc.m_request_json["destination"].asString())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::source_and_destination_same);
                return false;
            }

            std::string balance;
			int32_t ret = m_store_intf->get_account_balance(json_proc.m_request_json["source"].asString(), json_proc.m_request_json["property_key"].asString(), balance);
            if(store::store_intf_return_type::account_not_exist == ret)
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_empty)
                return false;
            }
			else if(store::store_intf_return_type::no_property_key == ret)
			{
                SET_JSON_PROC_RESPONSE(xhttp_error_code::property_key_is_not_exist)
                return false;
			}
            int64_t out_amount = std::stoll(json_proc.m_request_json["amount"].asString());
            if(std::stoull(balance) < out_amount && out_amount <= 0)
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::balance_is_not_enough)
                return false;       
            }

            //check last hash
            uint256_t acct_last_hash;
            if(m_store_intf->get_last_hash(json_proc.m_request_json["source"].asString(), acct_last_hash) && acct_last_hash != uint256_t((uint8_t*)string_utl::base64_decode(json_proc.m_request_json["last_digest"].asString()).c_str()))
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::last_hash_is_not_equal)
                return false;   
            }


            return true;
        }

        bool xrule_manager::transfer_in_filter(xjson_proc& json_proc)
        {
            if(json_proc.m_request_json["account"].isNull())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_empty)
                return false;
            }
            //check last hash
            uint256_t acct_last_hash;
            if(m_store_intf->get_last_hash(json_proc.m_request_json["account"].asString(), acct_last_hash) && acct_last_hash != uint256_t((uint8_t*)string_utl::base64_decode(json_proc.m_request_json["last_digest"].asString()).c_str()))
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::last_hash_is_not_equal)
                return false;   
            }
            return true;
        }

        bool xrule_manager::account_history_filter(xjson_proc& json_proc)
        {
            if(json_proc.m_request_json["account"].isNull())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_empty)
                return false;
            }
            if(json_proc.m_request_json["per_page"].asInt() > LARGEST_PER_PAGE)
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::per_page_too_large)
                return false;
            }
            return true;
        }

        bool xrule_manager::set_property_filter(xjson_proc& json_proc)
        {
            std::string property_key = json_proc.m_request_json["property_key"].asString();
                //note: the key_name of any data must start with "@" but NOT allow have "#"or "$"
            if((property_key.find_first_of("@") != 0) || (property_key.find_first_of("#$") != std::string::npos))
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::property_key_not_valid)
                return false;
            }
            return true;
        }

        bool xrule_manager::give_filter(xjson_proc& json_proc)
        {
            std::string dst_account = json_proc.m_request_json["destination"].asString();
            if(!m_store_intf->exist_account(dst_account))
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_not_exist)
                return false;
            }
            std::vector<std::shared_ptr<chain::xtransaction_t>> receive_list;
            m_store_intf->get_receive_blocks(dst_account, receive_list);
            if(receive_list.size() > 0)
			{
                SET_JSON_PROC_RESPONSE(xhttp_error_code::already_give)
                return false;
            }
            std::vector<std::shared_ptr<chain::xtransaction_t>> settle_list;
            m_store_intf->get_settle_blocks(dst_account, settle_list);

            //Response will start with the latest block for the account
            for(auto iter=settle_list.begin(); iter<settle_list.end();iter++)
            {
                if((*iter)->m_transaction_type == chain::xtransbase_t::enum_xtransaction_type::enum_xtransaction_type_transfer_asset_in)
                {
                    SET_JSON_PROC_RESPONSE(xhttp_error_code::already_give)
                    return false;
                }
            }
            return true;
        }

        bool xrule_manager::filter(xjson_proc& json_proc)
        {
            auto iter = g_action_map.find(json_proc.m_request_json["action"].asString());
            if(iter == g_action_map.end())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::action_not_find)
                return false;
            }
            switch(iter->second)
            {
                case xhttp_action::account_balance:
                    return account_balance_filter(json_proc);
                case xhttp_action::account_info:
                    return account_info_filter(json_proc);
                case xhttp_action::account_create:
                    return account_create_filter(json_proc);
                case xhttp_action::transfer_out:
                    return transfer_out_filter(json_proc);
                case xhttp_action::transfer_in:
                    return transfer_in_filter(json_proc);
                case xhttp_action::account_history:
                    return account_history_filter(json_proc);
                case xhttp_action::set_property:
                    return set_property_filter(json_proc);
                case xhttp_action::give:
                    return give_filter(json_proc);
                default:
                    break;
            }
            return true;
        }

    }
}