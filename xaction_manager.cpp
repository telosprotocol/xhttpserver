// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xaction_manager.h"
namespace top 
{
    namespace httpserver 
    {

        bool xaction_manager::do_action(xjson_proc& json_proc)
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
                    return query_balance(json_proc);
                case xhttp_action::account_info:
                    return query_account_info(json_proc);
                case xhttp_action::account_create:
                    return account_create(json_proc);
                case xhttp_action::transfer_out:
                    return transfer_out(json_proc);
                case xhttp_action::transfer_in:
                    return transfer_in(json_proc);
                case xhttp_action::account_history:
                    return account_history(json_proc);
                case xhttp_action::account_pending:
                    return account_pending(json_proc);
                case xhttp_action::last_digest:
                    return last_digest(json_proc);
                case xhttp_action::tps_query:
                    return tps_query(json_proc);
                case xhttp_action::set_property:
                    return set_property(json_proc);
                case xhttp_action::query_property:
                    return query_property(json_proc);
                case xhttp_action::query_all_property:
                    return query_all_property(json_proc);
                case xhttp_action::give:
                    return give(json_proc);
                case xhttp_action::query_online_tx:
                    return query_online_tx(json_proc);
                case xhttp_action::query_reward:
                    return query_reward(json_proc);
                default:
                    xwarn("exception action %d", iter->second);
            }
            return true;
        }
        bool xaction_manager::query_balance(xjson_proc& json_proc)
        {
            std::string balance;
            int32_t ret = m_store_intf->get_account_balance(json_proc.m_request_json["account"].asString(), json_proc.m_request_json["property_key"].asString(), balance);
            if(store::store_intf_return_type::account_not_exist == ret)
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_not_exist)
                return false;
            }
			else if(store::store_intf_return_type::no_property_key == ret)
			{
				SET_JSON_PROC_RESPONSE(xhttp_error_code::property_key_is_not_exist)
                return false;
			}
            SET_JSON_RESPONSE_OK
            json_proc.m_response_json["balance"] = balance;
            return true;
        }
        bool xaction_manager::query_account_info(xjson_proc& json_proc)
        {
            chain::xaccount_snap_t account_snap;
            if(!m_store_intf->get_account_snap_info(json_proc.m_request_json["account"].asString(), account_snap))
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_not_exist)
                return false;
            }
            string balance = "0";
            account_snap.get_balance("$$", balance);
            SET_JSON_RESPONSE_OK
            json_proc.m_response_json["balance"] = balance;
            json_proc.m_response_json["sequence"] = std::to_string(account_snap.m_sequence_id);
            uint256_t last_hash = account_snap.m_unit_snap.back()->m_associated_event_hash;
            json_proc.m_response_json["lasthash"] = string_utl::base64_encode(last_hash.data(),last_hash.size());
            json_proc.m_response_json["createtime"] = std::to_string(account_snap.m_create_time);
            string property_value;
            account_snap.get_property("@name", property_value);
            json_proc.m_response_json["property_value"] = property_value;
            json_proc.m_response_json["give"] = 1;
            std::vector<std::shared_ptr<chain::xtransaction_t>> settle_list;
            m_store_intf->get_settle_blocks(json_proc.m_request_json["account"].asString(), settle_list);
            for(auto iter=settle_list.begin(); iter<settle_list.end();iter++)
            {
                if((*iter)->m_transaction_type == chain::xtransbase_t::enum_xtransaction_type::enum_xtransaction_type_transfer_asset_in)
                {
                    json_proc.m_response_json["give"] = 0;
                    return true;
                }
            }
            std::vector<std::shared_ptr<chain::xtransaction_t>> receive_list;
            m_store_intf->get_receive_blocks(json_proc.m_request_json["account"].asString(), receive_list);
            if(receive_list.size() > 0)
			{
				json_proc.m_response_json["give"] = 0;
                return true;
            }
            return true;
        }
        //do hash calc,must at end of transation set
        bool xaction_manager::set_and_check(xjson_proc& json_proc)
        {
            json_proc.m_tx->m_timestamp = json_proc.m_request_json["timestamp"].asInt64();
            json_proc.m_tx->m_tx_digest = json_proc.m_tx->sha256();
            json_proc.m_tx->m_tx_signature = string_utl::base64_decode(json_proc.m_request_json["signature"].asString());
            json_proc.m_tx->m_edge_timestamp = top::base::xtimer_t::timestamp_now_ms();
            json_proc.m_tx->m_edge_addr = xconfig::get_instance().m_account;
            if(false == json_proc.m_tx->tx_hash_and_sign_verify())
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::signature_verify_fail)
                return false;
            }
            json_proc.m_tx->to_log("xaction_manager");
            return true;
        }
        uint32_t xaction_manager::do_transaction(const std::shared_ptr<top::chain::xtransaction_t>& tx)
        {
            top::xtop_message_t msg;
            msg.m_msg_type = top::enum_xtop_msg_type_proposal_message;
            msg.m_content = tx->serialize_no_type();
            //rpc server add a unique id in transaction msg for consensus leader selection
            msg.m_transaction_unique_seqno = get_transaction_unique_id();
            tx->to_log("rpc:" + get_message_uuid_string(msg));
  
            if(elect::enum_node_type_rpc != xconfig::get_instance().m_node_type)
            {
                xmessage_dispatcher::get_instance().broadcast_ex(msg);
            }
            else
            {
                elect::xshard_info shard_info = std::move(elect::xelect_manager_intf::get_instance()->get_account_shard_info(tx->m_sender_addr));
                xmessage_dispatcher::get_instance().broadcast(msg, enum_xtop_node_type_consensus_node, shard_info);
            }

            return 0;
        }
        bool xaction_manager::account_create(xjson_proc& json_proc)
        {
            json_proc.m_tx = std::make_shared<top::chain::xtransaction_t>();
            json_proc.m_tx->m_transaction_type = top::chain::xtransbase_t::enum_xtransaction_type_create_account;
            json_proc.m_tx->m_sender_addr = json_proc.m_request_json["account"].asString();
            json_proc.m_tx->m_receiver_addr = json_proc.m_request_json["account"].asString();
            json_proc.m_tx->m_property_key = "$$";
			if (!json_proc.m_request_json["amount"].isNull())
			{
				json_proc.m_tx->m_transaction_params = json_proc.m_request_json["amount"].asString();
			}
			else
			{
                json_proc.m_tx->m_transaction_params = "0";
			}
            if(set_and_check(json_proc))
            {
                // do_transaction(tx);
                // SET_JSON_RESPONSE_OK
                return true;
            }
            else
            {
                return false;
            }
        }

        bool xaction_manager::transfer_out(xjson_proc& json_proc)
        {
            json_proc.m_tx = std::make_shared<top::chain::xtransaction_t>();
            json_proc.m_tx->m_transaction_type = top::chain::xtransbase_t::enum_xtransaction_type_transfer_asset_out;
            json_proc.m_tx->m_sender_addr = json_proc.m_request_json["source"].asString();
            json_proc.m_tx->m_receiver_addr = json_proc.m_request_json["destination"].asString();
            json_proc.m_tx->m_property_key = json_proc.m_request_json["property_key"].asString();
            json_proc.m_tx->m_transaction_params = json_proc.m_request_json["amount"].asString();
            json_proc.m_tx->m_last_msg_digest = uint256_t((uint8_t*)string_utl::base64_decode(json_proc.m_request_json["last_digest"].asString()).c_str());
            if(set_and_check(json_proc))
            {
                // do_transaction(tx);
                // SET_JSON_RESPONSE_OK
                return true;
            }
            else
            {
                return false;
            }
        }

        bool xaction_manager::transfer_in(xjson_proc& json_proc)
        {
            json_proc.m_tx = std::make_shared<top::chain::xtransaction_t>();
            json_proc.m_tx->m_transaction_type = top::chain::xtransbase_t::enum_xtransaction_type_transfer_asset_in;
            json_proc.m_tx->m_sender_addr = json_proc.m_request_json["account"].asString();
            json_proc.m_tx->m_receiver_addr = json_proc.m_request_json["account"].asString();
            json_proc.m_tx->m_property_key = "";
            json_proc.m_tx->m_transaction_params = string_utl::base64_decode(json_proc.m_request_json["tx_digest"].asString());
            json_proc.m_tx->m_last_msg_digest = uint256_t((uint8_t*)string_utl::base64_decode(json_proc.m_request_json["last_digest"].asString()).c_str());
            json_proc.m_tx->m_origin_sender = json_proc.m_request_json["sender"].asString();
            json_proc.m_tx->m_origin_amount = json_proc.m_request_json["amount"].asString();
            if(set_and_check(json_proc))
            {
                // do_transaction(tx);
                // SET_JSON_RESPONSE_OK
                return true;
            }
            else
            {
                return false;
            }
        }

        bool xaction_manager::account_history(xjson_proc& json_proc)
        {
            int count = json_proc.m_request_json["count"].asInt();
            int per_page = json_proc.m_request_json["per_page"].asInt();
            int start_num = count * per_page;
            int end_num = (count + 1) * per_page;
            int num = 0;
            std::vector<std::shared_ptr<chain::xtransaction_t>> settle_list;
            m_store_intf->get_settle_blocks(json_proc.m_request_json["account"].asString(), settle_list);

            std::unordered_map<std::string, std::string> user_tag;
            xJson::Value blocks_json;
            xJson::Value block_json;
            //Response will start with the latest block for the account
            for(auto iter=settle_list.begin(); iter<settle_list.end(); iter++)
            {
                if((*iter)->m_transaction_type != chain::xtransbase_t::enum_xtransaction_type::enum_xtransaction_type_transfer_asset_in &&
                (*iter)->m_transaction_type != chain::xtransbase_t::enum_xtransaction_type::enum_xtransaction_type_transfer_asset_out)
                {
                    continue;
                }
                
                if(num >= start_num && num < end_num)
                {
                    block_json["transaction_type"] = (*iter)->m_transaction_type;
                    block_json["source"] = (*iter)->m_sender_addr;
                    block_json["destination"] = (*iter)->m_receiver_addr;
                    block_json["property_key"] = (*iter)->m_property_key.empty() ? "$$" : (*iter)->m_property_key;
                    if((*iter)->m_transaction_type == 1 || (*iter)->m_transaction_type == 0)
                    {//todo
                            block_json["amount"] = (*iter)->m_transaction_params;
                    }
                    else if((*iter)->m_transaction_type == chain::xtransbase_t::enum_xtransaction_type::enum_xtransaction_type_transfer_asset_in)
                    {
                            block_json["amount"] = (*iter)->m_origin_amount;
                            block_json["source"] = (*iter)->m_origin_sender;
                    }
                    set_property_value(user_tag, block_json);
                    block_json["tx_digest"] = string_utl::base64_encode((*iter)->m_tx_digest.data(),(*iter)->m_tx_digest.size());
                    block_json["last_digest"] = string_utl::base64_encode((*iter)->m_last_msg_digest.data(),(*iter)->m_last_msg_digest.size());
                    block_json["rpc_time"] = std::to_string((*iter)->m_edge_timestamp);
                    blocks_json.append(block_json);
                }
                ++num;
            }
            json_proc.m_response_json["blocks"] = blocks_json;
            json_proc.m_response_json["count"] = std::to_string(num);
            SET_JSON_RESPONSE_OK
            return true;
        }

        void xaction_manager::set_property_value(std::unordered_map<std::string, std::string>& user_tag, xJson::Value& block_json)
        {
            auto iter = user_tag.find(block_json["source"].asString());
            if(iter != user_tag.end())
            {
                block_json["source_tag"] = iter->second;
            }
            else
            {
                block_json["source_tag"] = "";
                chain::xaccount_snap_t account_snap;
                std::string property_value;
                if(m_store_intf->get_account_snap_info(block_json["source"].asString(), account_snap))
                {
                    if(account_snap.get_property(TAG_NAME, property_value))
                    {
                        block_json["source_tag"] = property_value;
                    }
                }
                user_tag.insert(std::make_pair(block_json["source"].asString(), property_value));
            }

            iter = user_tag.find(block_json["destination"].asString());
            if(iter != user_tag.end())
            {
                block_json["destination_tag"] = iter->second;
            }
            else
            {
                block_json["destination_tag"] = "";
                chain::xaccount_snap_t account_snap;
                std::string property_value;
                if(m_store_intf->get_account_snap_info(block_json["destination"].asString(), account_snap))
                {
                    if(account_snap.get_property(TAG_NAME, property_value))
                    {
                        block_json["destination_tag"] = property_value;
                    }
                }
                user_tag.insert(std::make_pair(block_json["destination"].asString(), property_value));
            }
        }

        bool xaction_manager::account_pending(xjson_proc& json_proc)
        {
            int count = json_proc.m_request_json["count"].isNull() ? 10 : json_proc.m_request_json["count"].asInt();
            int num = 0;

            std::vector<std::shared_ptr<chain::xtransaction_t>> receive_list;
            m_store_intf->get_receive_blocks(json_proc.m_request_json["account"].asString(), receive_list);
            xJson::Value blocks_json;
            xJson::Value block_json;
            for(auto iter=receive_list.begin(); iter<receive_list.end();iter++)
            {
                if(num++ >= count)
                {
                    break;
                }
                block_json["sender"] = (*iter)->m_sender_addr;
                block_json["property_key"] = (*iter)->m_property_key;
                block_json["amount"] = (*iter)->m_transaction_params;
                block_json["block"] = string_utl::base64_encode((*iter)->m_tx_digest.data(), (*iter)->m_tx_digest.size());
                blocks_json.append(block_json);
            }
            //add last hash
            uint256_t last_hash;
            if(m_store_intf->get_last_hash(json_proc.m_request_json["account"].asString(), last_hash))
            {
                json_proc.m_response_json["last_digest"] = string_utl::base64_encode(last_hash.data(),last_hash.size());
            }            
            json_proc.m_response_json["blocks"] = blocks_json;
            json_proc.m_response_json["count"] = std::to_string(receive_list.size());
            SET_JSON_RESPONSE_OK
            return true;
        }

        bool xaction_manager::last_digest(xjson_proc& json_proc)
        {
            chain::xaccount_snap_t local_account;
            if(!m_store_intf->get_account_snap_info(json_proc.m_request_json["account"].asString(), local_account))
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_not_exist)
                return false;
            }
            uint256_t last_hash = local_account.m_unit_snap.back()->m_associated_event_hash;
            json_proc.m_response_json["last_digest"] = string_utl::base64_encode(last_hash.data(),last_hash.size());
            SET_JSON_RESPONSE_OK
            return true;
        }

        bool xaction_manager::tps_query(xjson_proc& json_proc)
        {
            json_proc.m_response_json = std::move(m_store_intf->get_stat_info());
            SET_JSON_RESPONSE_OK;
            return true;
        }

        bool xaction_manager::query_reward(xjson_proc& json_proc)
        {
            std::string str_reward;
            xJson::Value statJson = std::move(m_store_intf->get_stat_info());
            if(m_store_intf->get_value(store::enum_xstorage_element::enum_rocksdb_other_block, REWARD_ALREAD_NUM, str_reward))
            {
                xJson::Reader reader;
                xJson::Value rewardJson;
				if (!reader.parse(str_reward, rewardJson)) 
				{
					xinfo("reward_token parse error:%s", str_reward.c_str());
                    SET_JSON_PROC_RESPONSE(xhttp_error_code::server_unknown_error)
					return false;
				}
                json_proc.m_response_json[REWARD_ALREAD_NUM] = std::stoll(rewardJson["REWARD_NUM"].asString());
            }
            else
            {
                json_proc.m_response_json[REWARD_ALREAD_NUM] = 0;
            }
            if(!statJson["TX_TOTAL_NUM"].isNull())
            {
                json_proc.m_response_json["TX_TOTAL_NUM"] = std::stoll(statJson["TX_TOTAL_NUM"].asString());
            }
            else
            {
                json_proc.m_response_json["TX_TOTAL_NUM"] = 0;
            }
            
            SET_JSON_RESPONSE_OK;
            return true;
        }

        bool xaction_manager::set_property(xjson_proc& json_proc)
        {
            json_proc.m_tx = std::make_shared<top::chain::xtransaction_t>();
            json_proc.m_tx->m_transaction_type = top::chain::xtransbase_t::enum_xtransaction_type_data_property_operation;
            json_proc.m_tx->m_sender_addr = json_proc.m_request_json["account"].asString();
            json_proc.m_tx->m_receiver_addr = json_proc.m_request_json["account"].asString();
            json_proc.m_tx->m_property_key = json_proc.m_request_json["property_key"].asString();
            json_proc.m_tx->m_transaction_params = json_proc.m_request_json["property_value"].asString();
            json_proc.m_tx->m_last_msg_digest = uint256_t((uint8_t*)string_utl::base64_decode(json_proc.m_request_json["last_digest"].asString()).c_str());

            if(set_and_check(json_proc))
            {
                // do_transaction(tx);
                // SET_JSON_RESPONSE_OK
                return true;
            }
            else
            {
                return false;
            }
        }

        bool xaction_manager::query_property(xjson_proc& json_proc)
        {
            chain::xaccount_snap_t account_snap;
			if (m_store_intf->get_account_snap_info(json_proc.m_request_json["account"].asString(), account_snap))
			{
                std::string property_value;
				if(account_snap.get_property(json_proc.m_request_json["property_key"].asString(), property_value))
                {
                    json_proc.m_response_json["property_value"] = property_value;
                    SET_JSON_RESPONSE_OK
                    return true;
                }
                else
                {
                    SET_JSON_PROC_RESPONSE(xhttp_error_code::property_key_is_not_exist)
                    return false;
                }
			}
			else
			{
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_not_exist)
                return false;
			}
        }

        bool xaction_manager::query_all_property(xjson_proc& json_proc)
        {
            chain::xaccount_snap_t account_snap;
			if (m_store_intf->get_account_snap_info(json_proc.m_request_json["account"].asString(), account_snap))
			{
                std::unordered_map<std::string, std::string> property_map = std::move(account_snap.get_all_property());
				for(auto iter = property_map.begin(); iter != property_map.end(); ++iter)
                {
                    json_proc.m_response_json[iter->first] = iter->second;
                }
                SET_JSON_RESPONSE_OK
                return true;
			}
			else
			{
                SET_JSON_PROC_RESPONSE(xhttp_error_code::account_is_not_exist)
                return false;
			}
        }

        bool xaction_manager::get_genesis_sign(std::shared_ptr<top::chain::xtransaction_t>& tx)
        {
            tx->m_edge_timestamp = top::base::xtimer_t::timestamp_now_ms();
            tx->m_edge_addr = xconfig::get_instance().m_account;
            top::utl::xecprikey_t pri_key_obj;
            uint8_t  test_genesis_account_pri_key[32] = {0x83,0xab,0x81,0xe3,0xcf,0xe1,0x44,0x25,0xe5,0x30,0xea,0xb1,0xd3,0x87,0xc6,0xff,0x80,0x50,0x63,0x73,0xc0,0xa1,0xcf,0x5c,0x63,0xc7,0x55,0x1c,0x68,0x40,0x06,0xed};
            std::string test_genesis_account_address = "T-Um45LZ8HZYUpm5jdWi6bh99mdNjEHkz6w";
            pri_key_obj = top::utl::xecprikey_t(test_genesis_account_pri_key);
            tx->tx_hash_and_sign_calc(pri_key_obj);
            return true;
        }

        bool xaction_manager::give(xjson_proc& json_proc)
        {
            json_proc.m_tx = std::make_shared<top::chain::xtransaction_t>();
            json_proc.m_tx->m_transaction_type = top::chain::xtransbase_t::enum_xtransaction_type_transfer_asset_out;
            json_proc.m_tx->m_sender_addr = "T-Um45LZ8HZYUpm5jdWi6bh99mdNjEHkz6w";
            json_proc.m_tx->m_receiver_addr = json_proc.m_request_json["destination"].asString();
            json_proc.m_tx->m_property_key = "$$";
            if (!json_proc.m_request_json["amount"].isNull())
			{
				json_proc.m_tx->m_transaction_params = json_proc.m_request_json["amount"].asString();
			}
			else
			{
                json_proc.m_tx->m_transaction_params = "100000";
			}
            json_proc.m_tx->m_timestamp = json_proc.m_request_json["timestamp"].asInt64();
            uint256_t lash_hash;
            if(m_store_intf->get_last_hash(json_proc.m_tx->m_sender_addr, lash_hash))
            {
                json_proc.m_tx->m_last_msg_digest = lash_hash;
            }
            else
            {
                SET_JSON_PROC_RESPONSE(xhttp_error_code::last_hash_is_not_equal)
                return false;
            }
            if(get_genesis_sign(json_proc.m_tx))
            {
                // do_transaction(tx);
                // SET_JSON_RESPONSE_OK
                return true;
            }
            else
            {
                return false;
            }
        }

        bool xaction_manager::query_online_tx(xjson_proc& json_proc)
        {
            int count = json_proc.m_request_json["count"].asInt();
            int per_page = json_proc.m_request_json["per_page"].asInt();
            int start_num = count * per_page;
            int end_num = (count + 1) * per_page;
            int num = 0;
            std::deque<shared_ptr<chain::xtransaction_t>> tx_list;
            if(m_store_intf->query_online_tx(tx_list))
            {
                xJson::Value blocks_json;
                xJson::Value block_json;
                for(auto iter=tx_list.begin(); iter<tx_list.end();iter++)
                {
                    if(num >= start_num && num < end_num)
                    {
                        block_json["sender"] = (*iter)->m_sender_addr;
                        block_json["receiver"] = (*iter)->m_receiver_addr;
                        block_json["property_key"] = (*iter)->m_property_key;
                        block_json["amount"] = (*iter)->m_transaction_params;
                        block_json["timestamp"] = std::to_string((*iter)->m_timestamp);
                        block_json["last_digest"] = string_utl::base64_encode((*iter)->m_last_msg_digest.data(),(*iter)->m_last_msg_digest.size());
                        block_json["tx_digest"] = string_utl::base64_encode((*iter)->m_tx_digest.data(),(*iter)->m_tx_digest.size());
                        block_json["rpc_time"] = std::to_string((*iter)->m_edge_timestamp);
                        blocks_json.append(block_json);
                    }
                    ++num;
                }
                json_proc.m_response_json["blocks"] = blocks_json;
                json_proc.m_response_json["count"] = std::to_string(tx_list.size());
                SET_JSON_RESPONSE_OK
                return true;
            }
            SET_JSON_PROC_RESPONSE(xhttp_error_code::server_unknown_error)
            return false;
        }
    }
}