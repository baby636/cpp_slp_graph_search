#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>

#include <grpc++/grpc++.h>
#include <libbase64.h>
#include <gs++/bhash.hpp>
#include "graphsearch.grpc.pb.h"

class GraphSearchServiceClient
{
public:
    GraphSearchServiceClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(graphsearch::GraphSearchService::NewStub(channel))
    {}

    bool GraphSearch(const std::string& txid)
    {
        graphsearch::GraphSearchRequest request;
        request.set_txid(txid);

        graphsearch::GraphSearchReply reply;

        grpc::ClientContext context;
        grpc::Status status = stub_->GraphSearch(&context, request, &reply);

        if (status.ok()) {
            for (auto n : reply.txdata()) {
                // 4/3 is recommended size with a bit of a buffer
                std::string b64(n.size()*1.5, '\0');
                std::size_t b64_len = 0;
                base64_encode(n.data(), n.size(), const_cast<char*>(b64.data()), &b64_len, 0);
                b64.resize(b64_len);
                std::cout << b64 << "\n";
            }

            return true;
        } else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return false;
        }
    }

    bool UtxoSearchByOutpoints(
        const std::vector<std::pair<std::string, std::uint32_t>> outpoints
    ) {
        graphsearch::UtxoSearchByOutpointsRequest request;

        for (auto o : outpoints) {
            auto * outpoint = request.add_outpoints();
            outpoint->set_txid(o.first);
            outpoint->set_vout(o.second);
        }

        graphsearch::UtxoSearchReply reply;

        grpc::ClientContext context;
        grpc::Status status = stub_->UtxoSearchByOutpoints(&context, request, &reply);

        if (status.ok()) {
            for (auto n : reply.outputs()) {
                const std::string   prev_tx_id_str = n.prev_tx_id();
                const std::uint32_t prev_out_idx   = n.prev_out_idx();
                const std::uint32_t height         = n.height();
                const std::uint64_t value          = n.value();
                const std::string   pk_script_str  = n.pk_script();

                gs::txid prev_tx_id(prev_tx_id_str);

                std::string pk_script_b64(pk_script_str.size()*1.5, '\0');
                std::size_t pk_script_b64_len = 0;
                base64_encode(
                    pk_script_str.data(),
                    pk_script_str.size(),
                    const_cast<char*>(pk_script_b64.data()),
                    &pk_script_b64_len,
                    0
                );
                pk_script_b64.resize(pk_script_b64_len);

                std::cout
                    << "prev_tx_id:   " << prev_tx_id.decompress(true) << "\n"
                    << "prev_out_idx: " << prev_out_idx                << "\n"
                    << "height:       " << height                      << "\n"
                    << "value:        " << value                       << "\n"
                    << "pk_script:    " << pk_script_b64               << "\n\n";
            }

            return true;
        } else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return false;
        }
    }

    bool UtxoSearchByScriptSig(const std::string script_sig_b64)
    {
        graphsearch::UtxoSearchByScriptSigRequest request;

        std::string script_sig('\0', script_sig_b64.size());
        std::size_t script_sig_len;

        base64_decode(
            script_sig_b64.data(),
            script_sig_b64.size(),
            script_sig.data(),
            &script_sig_len,
            0
        );
        script_sig.resize(script_sig_len);

        request.set_scriptsig(script_sig);

        graphsearch::UtxoSearchReply reply;

        grpc::ClientContext context;
        grpc::Status status = stub_->UtxoSearchByScriptSig(&context, request, &reply);

        if (status.ok()) {
            for (auto n : reply.outputs()) {
                const std::string   prev_tx_id_str = n.prev_tx_id();
                const std::uint32_t prev_out_idx   = n.prev_out_idx();
                const std::uint32_t height         = n.height();
                const std::uint64_t value          = n.value();
                const std::string   pk_script_str  = n.pk_script();

                gs::txid prev_tx_id(prev_tx_id_str);

                std::string pk_script_b64(pk_script_str.size()*1.5, '\0');
                std::size_t pk_script_b64_len = 0;
                base64_encode(
                    pk_script_str.data(),
                    pk_script_str.size(),
                    const_cast<char*>(pk_script_b64.data()),
                    &pk_script_b64_len,
                    0
                );
                pk_script_b64.resize(pk_script_b64_len);

                std::cout
                    << "prev_tx_id:   " << prev_tx_id.decompress(true) << "\n"
                    << "prev_out_idx: " << prev_out_idx                << "\n"
                    << "height:       " << height                      << "\n"
                    << "value:        " << value                       << "\n"
                    << "pk_script:    " << pk_script_b64               << "\n\n";
            }

            return true;
        } else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return false;
        }
    }

private:
    std::unique_ptr<graphsearch::GraphSearchService::Stub> stub_;
};

int main(int argc, char* argv[])
{
    std::string grpc_host = "0.0.0.0";
    std::string grpc_port = "50051";
    std::string query_type = "graphsearch";

    const std::string usage_str = "usage: gs++-cli [--version] [--help] [--host host_address] [--port port]\n"
                                  "[--graphsearch TXID] [--utxo TXID:VOUT] [--utxo_scriptsig PK]\n";

    while (true) {
        static struct option long_options[] = {
            { "help",    no_argument,       nullptr, 'h' },
            { "version", no_argument,       nullptr, 'v' },
            { "host",    required_argument, nullptr, 'b' },
            { "port",    required_argument, nullptr, 'p' },

            { "graphsearch",  no_argument,   nullptr, 1000 },
            { "utxo",         no_argument,   nullptr, 1001 },
            { "utxo_scriptsig", no_argument,   nullptr, 1002 },
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "hvb:p:", long_options, &option_index);

        if (c == -1) {
            break;
        }

        std::stringstream ss(optarg != nullptr ? optarg : "");
        switch (c) {
            case 0:
                if (long_options[option_index].flag != 0) {
                    break;
                }

                break;
            case 'h':
                std::cout << usage_str;
                return EXIT_SUCCESS;
            case 'v':
                std::cout <<
                    "gs++-cli v" << GS_VERSION << std::endl;
                return EXIT_SUCCESS;
            case 'b': ss >> grpc_host; break;
            case 'p': ss >> grpc_port; break;

            case 1000: query_type = "graphsearch";  break;
            case 1001: query_type = "utxo";         break;
            case 1002: query_type = "utxo_scriptsig"; break;

            case '?':
                return EXIT_FAILURE;
            default:
                return EXIT_FAILURE;
        }
    }
    if (argc < 2) {
        std::cout << usage_str;
        return EXIT_FAILURE;
    }

    grpc::ChannelArguments ch_args;
    ch_args.SetMaxReceiveMessageSize(-1);

    GraphSearchServiceClient graphsearch(
        grpc::CreateCustomChannel(
            grpc_host+":"+grpc_port,
            grpc::InsecureChannelCredentials(),
            ch_args
        )
    );

    if (query_type == "graphsearch") {
        graphsearch.GraphSearch(argv[argc-1]);
    } else if (query_type == "utxo") {
        std::vector<std::pair<std::string, std::uint32_t>> outpoints;
        for (int optidx=optind; optidx < argc; ++optidx) {
            std::vector<std::string> seglist;
            {
                std::stringstream ss(argv[optidx]);
                std::string segment;

                while (std::getline(ss, segment, ':')) {
                    seglist.push_back(segment);
                }
            }

            if (seglist.size() != 2) {
                std::cerr << "bad format: expected TXID:VOUT\n";
                return EXIT_FAILURE;
            }

            gs::txid txid(seglist[0]);
            std::reverse(txid.v.begin(), txid.v.end());
            std::uint32_t vout = 0;
            {
                std::stringstream ss(seglist[1]);
                ss >> vout;
            }

            outpoints.push_back({ txid.decompress(), vout });
        }

        graphsearch.UtxoSearchByOutpoints(outpoints);
    } else if (query_type == "utxo_scriptsig") {
        const std::string script_sig = argv[argc-1];
        graphsearch.UtxoSearchByScriptSig(script_sig);
    }

    return EXIT_SUCCESS;
}

