#include <iostream>
#include <asio.hpp>
#include <asio/experimental/as_single.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <regex>
#include <unordered_map>
#include <filesystem>

using asio::ip::tcp;
using std::chrono::steady_clock;
using asio::co_spawn;
using asio::detached;
using asio::ip::tcp;
using asio::use_awaitable;
using namespace asio::experimental::awaitable_operators;
namespace this_coro = asio::this_coro;
using namespace std::literals;

constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(use_awaitable);
struct server_info
{
    std::unordered_map<std::string, std::string> exchange_codes;
    std::string server_id;
    std::string data_center_id;
    std::chrono::time_point<std::chrono::system_clock> one_day_look_back = std::chrono::system_clock::now();
};
enum class method
{
    POST, GET, PATCH, DELETE, OPTIONS, UNKNOWN
};
auto to_method(std::string str) -> method
{
    std::string lower_str;
    std::transform(str.cbegin(), str.cend(), std::back_inserter(lower_str), [](unsigned char c)
                   {
                       return std::tolower(c);
                   }
    );
    std::cout << "origin string " << str << " transform string " << lower_str << std::endl;
    if (lower_str == "post") {
        return method::POST;
    }
    else if(lower_str == "get")
    {
        return method::GET;
    }
    else if(lower_str == "patch")
    {
        return method::PATCH;
    }
    else if(lower_str == "options")
    {
        return method::OPTIONS;
    }
    else if(lower_str == "delete")
    {
        return method::DELETE;
    }
    return method::UNKNOWN;
}
auto to_string(method m) -> std::string
{
    switch (m) {
        case method::GET:
            return "GET";
        case method::POST:
            return "POST";
        case method::PATCH:
            return "PATCH";
        case method::OPTIONS:
            return "OPTIONS";
        case method::DELETE:
            return "DELETE";
        default:
            break;
    }
    return "UNKNOWN";

}

struct http_fields
{
    std::unordered_map<std::string, std::string> fields_;
    std::string url_;
    method method;
};
std::pair<std::string, method> parse_header(std::string str)
{
    using namespace std::literals;
    std::smatch s_m;
    std::smatch s_u;
    std::regex method_regex(R"(\s*[GgPp][EePp][TeOo][Tt]?)");
    std::regex url_regex(R"(\s.[/\w]*\s)");
    std::string url_;
    method method_;
    try {
        if(std::regex_search(str, s_m, method_regex))
        {
            auto m =  s_m.str();
            m.erase(std::remove_if(m.begin(), m.end(), [](const auto a ) { return std::isspace(a); } ), m.end());
            method_ = to_method(m);

        }
        if(std::regex_search(str, s_u, url_regex))
        {
            auto m =  s_u.str();
            m.erase(std::remove_if(m.begin(), m.end(), [](const auto a ) { return std::isspace(a); } ), m.end());
            url_ = m;

        }
    }
    catch (std::exception ex)
    {
        std::cout << ex.what();
    }
    return std::make_pair(url_, method_);
}

bool is_first_header(const std::string str)
{
    std::regex method_regex(R"(\s*[GgPp][EePp][TeOo][Tt]?)");
    if(std::regex_search(str, method_regex))
    {
        return true;
    }
    return false;
}

template<typename Stream>
struct message_reader
{
    message_reader(Stream& stream) : stream_(stream){}
    asio::awaitable<http_fields> read_message()
    {
        while(true) {
            auto[e, n] = co_await asio::async_read_until(stream_, asio::dynamic_buffer(data_), "\n",
                                                         use_nothrow_awaitable);
            if (e) {
                std::cout << "error " << e << std::endl;
                break;
            }
            auto msg = data_.substr(0, n);
            data_.erase(0, n);
            if(is_first_header(msg))
            {
                auto [url, m] = parse_header(msg);
                http_data_.url_ = url;
                http_data_.method = m;
            }
            else
            {
                auto find_next_asterisk = msg.find(":");
                if(find_next_asterisk != std::string::npos)
                    http_data_.fields_.template emplace(msg.substr(9, find_next_asterisk), msg.substr(find_next_asterisk + 1));
            }
            buffers.push_back(msg);
            co_return http_data_;
        }
        co_return http_data_;

    }
    Stream& stream_;
    std::string data_;
    std::vector<std::string> buffers;
    http_fields http_data_;
};


std::string generate_id(bool is_buy, const std::string exchange, const server_info& info)
{
    std::stringstream  ss;
    std::bitset<10> b; // ..... exchange, .. server id, .
    if(is_buy)
    {
        ss << "0" << std::chrono::system_clock::now().time_since_epoch().count() << info.exchange_codes.at(exchange) << info.server_id << info.data_center_id << std::endl;
    }
    else
    {
        ss << "1" << std::chrono::system_clock::now().time_since_epoch().count() << info.exchange_codes.at(exchange) << info.server_id << info.data_center_id << std::endl;
    }
    return {};
}

#include <fstream>

std::unordered_map<std::string, std::string> get_exchange_code(std::filesystem::path path)
{
    std::ifstream is(path, std::ios::in);
    std::string line;
    std::unordered_map<std::string ,std::string> results;
    while(getline(is, line))
    {
        line.erase(std::remove_if(line.begin(), line.end(), [](const char c) { return std::isspace(c); }));
        results.emplace(line.substr(0,line.find(":")), line.substr(line.find(":") +  1));
    }
    return results;
}

asio::awaitable<void> session(tcp::socket client, const server_info& exchange_codes)
{
    message_reader<tcp::socket > reader(client);
    for(;;)
    {
        auto result = co_await reader.read_message();
        std::cout << "Received : " << to_string(result.method) << " " << result.url_ << std::endl;
        std::stringstream  ss;

    }
}
asio::awaitable<void> listen(tcp::acceptor& acceptor, const server_info& exchange_codes)
{
    for(;;)
    {
        auto [e, client] = co_await acceptor.async_accept(use_nothrow_awaitable);
        if(e)
            break;
        auto ex = client.get_executor();
        co_spawn(ex, session(std::move(client), exchange_codes), detached);
    }
}

int main(int argc, char* argv[])
{
    try
    {
        if(argc != 4 )
        {
            std::cerr << "Usage: Ticker Server ";
            std::cerr << "<listen_address> <listen_port> <exchange_code file> <server_id> <datacenter id>";
            return 1;
        }
        asio::io_context ctx;
        auto listen_endpoint = *tcp::resolver(ctx).resolve(argv[1], argv[2], tcp::resolver::passive);
        auto exchange_codes = get_exchange_code(argv[3]);
        auto data_center_info = server_info {
            .exchange_codes = exchange_codes,
            .server_id = argv[4],
            .data_center_id = argv[5],
        };
        tcp::acceptor acceptor(ctx, listen_endpoint);
        co_spawn(ctx, listen(acceptor, data_center_info), detached);
        ctx.run();
    }
    catch (std::exception ex)
    {
        std::cout << ex.what();
    }
    get_exchange_code(std::filesystem::path{"exchange_code.txt"});
}
