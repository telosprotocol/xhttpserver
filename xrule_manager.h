// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include<unordered_map>
#include "xstring.h"
#include "xstore_manager.h"
#include "xjson_proc.h"
namespace top 
{
    namespace httpserver 
    {
#define LARGEST_PER_PAGE 100
        class xrule_manager
        {
            public:
            xrule_manager(){m_store_intf = xsingleton<top::store::xstore_manager>::Instance();}
            bool filter(xjson_proc& json_proc);
            bool account_balance_filter(xjson_proc& json_proc);
            bool account_info_filter(xjson_proc& json_proc);
            bool account_create_filter(xjson_proc& json_proc);
            bool transfer_out_filter(xjson_proc& json_proc);
            bool transfer_in_filter(xjson_proc& json_proc);
            bool account_history_filter(xjson_proc& json_proc);
            bool set_property_filter(xjson_proc& json_proc);
            bool give_filter(xjson_proc& json_proc);
            public:
            top::store::xstore_base* m_store_intf;
        };

    }
}