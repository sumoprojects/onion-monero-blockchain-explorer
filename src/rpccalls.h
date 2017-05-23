//
// Created by mwo on 13/04/16.
//


#ifndef CROWXMR_RPCCALLS_H
#define CROWXMR_RPCCALLS_H

#include "monero_headers.h"

#include <mutex>

namespace xmreg
{

    using namespace cryptonote;
    using namespace crypto;
    using namespace std;


    class rpccalls
    {
        string deamon_url ;
        uint64_t timeout_time;

        epee::net_utils::http::url_content url;

        epee::net_utils::http::http_simple_client m_http_client;
        std::mutex m_daemon_rpc_mutex;

        string port;

    public:

        rpccalls(string _deamon_url = "http:://127.0.0.1:19734",
                 uint64_t _timeout = 200000)
        : deamon_url {_deamon_url}, timeout_time {_timeout}
        {
            epee::net_utils::parse_url(deamon_url, url);

            port = std::to_string(url.port);
        }

        bool
        connect_to_monero_deamon()
        {
            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            if(m_http_client.is_connected())
            {
                return true;
            }

            return m_http_client.connect(url.host,
                                         port,
                                         timeout_time);
        }

        uint64_t
        get_current_height()
        {
            COMMAND_RPC_GET_HEIGHT::request   req;
            COMMAND_RPC_GET_HEIGHT::response  res;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            bool r = epee::net_utils::invoke_http_json_remote_command2(
                    deamon_url + "/getheight",
                    req, res, m_http_client, timeout_time);

            if (!r)
            {
                cerr << "Error connecting to Monero deamon at "
                     << deamon_url << endl;
                return 0;
            }
            else
            {
                cout << "rpc call /getheight OK: " << endl;
            }

            return res.height;
        }

        bool
        get_mempool(vector<tx_info>& mempool_txs) {

            COMMAND_RPC_GET_TRANSACTION_POOL::request  req;
            COMMAND_RPC_GET_TRANSACTION_POOL::response res;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            bool r = epee::net_utils::invoke_http_json_remote_command2(
                    deamon_url + "/get_transaction_pool",
                    req, res, m_http_client, timeout_time);

            if (!r)
            {
                cerr << "Error connecting to Monero deamon at "
                     << deamon_url << endl;
                return false;
            }


            mempool_txs = res.transactions;

            return true;
        }


        bool
        commit_tx(tools::wallet2::pending_tx& ptx, string& error_msg)
        {
            COMMAND_RPC_SEND_RAW_TX::request  req;
            COMMAND_RPC_SEND_RAW_TX::response res;

            req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(
                    tx_to_blob(ptx.tx)
            );

            req.do_not_relay = false;

            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            bool r = epee::net_utils::invoke_http_json_remote_command2(deamon_url
                                                                       + "/sendrawtransaction",
                                                                       req, res,
                                                                       m_http_client, 200000);;

            if (!r || res.status == "Failed")
            {
                error_msg = res.reason;

                cerr << "Error sending tx: " << res.reason << endl;
                return false;
            }

            return true;
        }
        
        uint64_t
        get_coinbase_tx_sum()
        {
            epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_COINBASE_TX_SUM::request> req_t = AUTO_VAL_INIT(req_t);
            epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_COINBASE_TX_SUM::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
            
            req_t.jsonrpc = "2.0";
            req_t.method = "get_coinbase_tx_sum";
            req_t.id = epee::serialization::storage_entry(0);
            req_t.params.height = 0;
            req_t.params.count = 1000000000;
            
            std::lock_guard<std::mutex> guard(m_daemon_rpc_mutex);

            bool r = epee::net_utils::invoke_http_json_remote_command2(
                    deamon_url + "/json_rpc",
                    req_t, resp_t, m_http_client, 200000);

            if (!r)
            {
                cerr << "Error connecting to Monero deamon at "
                     << deamon_url << endl;
                return 0;
            }
            
            return resp_t.result.emission_amount;
        }

    };


}



#endif //CROWXMR_RPCCALLS_H
