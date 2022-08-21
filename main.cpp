#include <iostream>
#include <asio.hpp>
#include <asio/experimental/as_single.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <regex>
#include <unordered_map>
#include <filesystem>
#include <typeinfo>

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
    int incrementor = 0;
};
enum class method
{
    POST, GET, PATCH, DELETE, OPTIONS, UNKNOWN
};
enum class side_type
{
    UNKNOWN, BUY, SELL
};
auto to_side_type(std::string str) -> side_type
{
    std::transform(str.begin(), str.end(), str.begin(), [](char c) { return std::tolower(c); });
    if(str == "buy")
    {
        return side_type::BUY;
    }
    else if(str == "sell")
    {
        return side_type::SELL;
    }
    else
    {
        return side_type::UNKNOWN;
    }
}

auto to_string(side_type side) -> std::string
{
    switch (side) {
        case side_type::BUY:
            return "buy";
        case side_type::SELL:
            return "sell";
        default:
            return "unknown";
    }
}
auto to_method(std::string str) -> method
{
    std::string lower_str;
    std::transform(str.cbegin(), str.cend(), std::back_inserter(lower_str), [](unsigned char c)
                   {
                       return std::tolower(c);
                   }
    );
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
    std::string http_info;
};
std::tuple<std::string, method, std::string> parse_header(std::string str)
{
    using namespace std::literals;
    std::smatch s_m;
    std::smatch s_u;
    std::regex method_regex(R"(\s*[GgPpDdPpOo][EeOoEeAaPp][TteOoSsLl][TtEeCcIi]?[TtHhOo]?[EeNn]?[Ss]?)");
    std::regex url_regex(R"(\s.[/\w]*\s)");
    std::string url_;
    method method_;
    std::string http_;
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
        http_ = str.substr(str.find("HTTP/"));
    }
    catch (std::exception ex)
    {
        std::cerr << "ERROR " << __FUNCTION__  << ":" << __LINE__ << ": " << ex.what() << std::endl;
    }
    return std::make_tuple(url_, method_, http_);
}

bool is_first_header(const std::string str)
{
    try {
        std::regex method_regex(R"(\s*[GgPpDdPpOo][EeOoEeAaPp][TteOoSsLl][TtEeCcIi]?[TtHhOo]?[EeNn]?[Ss]?)");
        if (std::regex_search(str, method_regex)) {
            return true;
        }
    }
    catch (std::exception& ex)
    {
        std::cerr << "ERROR " << __FUNCTION__  << ":" << __LINE__ << ": " << ex.what() << std::endl;
        throw;
    }
    return false;
}

template<typename T>
std::string auto_fill_zero(T input, size_t size)
{
    std::string result;
    result.resize(size, '0');
    auto converted_input = std::to_string(input);
    auto o_iter = result.rbegin();
    auto i_iter = converted_input.rbegin();
    while(o_iter != result.rend() && i_iter != converted_input.rend())
    {
        *o_iter = *i_iter;
        o_iter++; i_iter++;
    }

    return result;
}

template<>
std::string auto_fill_zero(std::string input, size_t size)
{
    std::string result; result.resize(size, '0');
    auto o_iter = result.rbegin();
    auto i_iter = input.rbegin();
    while(o_iter != result.rend() && i_iter != input.rend())
    {
        *o_iter = *i_iter;
        o_iter++; i_iter++;
    }
    return result;
}

std::string generate_id(bool is_buy, const std::string exchange_id, server_info& info)
{
    std::stringstream  ss;
    auto incremental = info.incrementor++;
    if( std::chrono::system_clock::now() - info.one_day_look_back > 24h)
    {
        info.incrementor = 0;
        info.one_day_look_back = std::chrono::system_clock::now();
    }
    if(is_buy)
    {
        ss << "0" << std::chrono::system_clock::now().time_since_epoch().count() << exchange_id << auto_fill_zero(info.server_id, 3) << auto_fill_zero(info.data_center_id,2) << auto_fill_zero<int>(incremental, 6) << std::endl;
    }
    else
    {
        ss << "1" << std::chrono::system_clock::now().time_since_epoch().count() << exchange_id << auto_fill_zero(info.server_id,3) << auto_fill_zero(info.data_center_id,2) << auto_fill_zero<int>(incremental, 6) << std::endl;
    }
    return ss.str();
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

std::string build_404(const std::string msg, const std::string http_version)
{
    std::stringstream  ss;
    ss << "HTTP/1.1 404 Not Found\r\n";
    ss << "Content-Type: text/plain\r\n";
    ss << "Content-Length: " << msg.size() << "\r\n";
    ss << "\r\r";
    ss << msg;
    return ss.str();
}
std::string build_response(const std::string msg, const std::string http_version)
{
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "Content-Type: text/plain\r\n";
    ss << "Content-Length: " << msg.size() << "\r\n";
    ss << "\r\r";
    ss << msg;
    return ss.str();
}


struct Session : public std::enable_shared_from_this<Session>
{
    Session(tcp::socket client,server_info& info): client_(std::move(client)), info_(info){}
    void do_read()
    {
        auto self(shared_from_this());
        asio::async_read_until(client_, asio::dynamic_buffer(buffer), "\r\n", [this,self](asio::error_code ec, size_t n){
            if(!ec)
            {
                if(!n)
                {
                    std::cout << "n is zero" << std::endl;
                    return ;
                }
                auto msg = self->buffer.substr(0, n);
                self->buffer.erase(0, n);
                if(n < 5)
                {
                    self->do_write();
                }
                try {
                    if (!self->was_first_processed && is_first_header(msg)) {
                        auto[url, m, http] = parse_header(msg);
                        try {
                            self->http_data_.url_ = url;
                            self->http_data_.method = m;
                            self->http_data_.http_info = http;
                            self->was_first_processed = true;
                        }
                        catch (std::exception ex) {
                            std::cerr << "ERROR " << __FUNCTION__  << ":" << __LINE__ << ": " << ex.what() << std::endl;
                        }
                    } else {
                        auto find_next_asterisk = msg.find(":");
                        if (find_next_asterisk != std::string::npos)
                            self->http_data_.fields_.template emplace(msg.substr(9, find_next_asterisk),
                                                                msg.substr(find_next_asterisk + 1));
                    }
                } catch (std::exception ex)
                {
                    std::cerr << "ERROR " << __FUNCTION__  << ":" << __LINE__ << ": " << ex.what() << std::endl;
                }
                self->do_read();
            }
            else if (ec != asio::error::operation_aborted)
            {
                self->client_.close();
            }
            else if(ec == asio::error::eof)
            {
                std::cout << "eof" <<std::endl;
            }
            else
            {
                std::cout << "ec = " << ec << std::endl;
            }
        });
    }
    void do_write()
    {
        std::string out;
        auto self = shared_from_this();
        auto url = self->http_data_.url_;
        auto side_type_idx = url.find_last_of('/');
        auto side_type_str = url.substr(side_type_idx + 1);
        auto exchange_idx = url.find_last_of('/',side_type_idx  - 1);
        auto exchange_str = url.substr(exchange_idx + 1, side_type_idx - exchange_idx - 1);
        std::transform(exchange_str.begin(), exchange_str.end(), exchange_str.begin(), [](char c) { return std::toupper(c); });
        auto side = to_side_type(side_type_str);
        auto exchange_info_iter = info_.exchange_codes.find(exchange_str);
        if(http_data_.method != method::POST || side == side_type::UNKNOWN || exchange_info_iter == info_.exchange_codes.end()) {
            auto msg = std::string("Request Not Found. Example: POST /api/ftx/sell | /api/ftx/buy\r\n");
            std::stringstream  ss;
            ss << msg;
            ss << "Support Exchange : \r\r";
            for(auto [k,v] : info_.exchange_codes)
            {
                ss << k << ": " << v << "\r\n";
            }
            out = build_404(ss.str(), "HTTP/1.1");
        }
        else
        {
            std::stringstream  ss;
            ss << generate_id(side == side_type::BUY, exchange_info_iter->second, info_);
            ss << "\r\n";
            out = build_response(ss.str(), "HTTP/1.1");
        }
        asio::const_buffer buffer(out.c_str(), out.size());

        {
            asio::error_code  ec;
            asio::async_write(client_, buffer, [self=shared_from_this()](asio::error_code ec, std::size_t size){
                if (!ec)
                {
                    // Initiate graceful connection closure.
                    asio::error_code ignored_ec;
                    self->client_.shutdown(asio::ip::tcp::socket::shutdown_both,
                                     ignored_ec);
                }
            });
        }
    }

    tcp::socket client_;
    server_info& info_;
    std::string buffer;
    http_fields http_data_;
    bool was_first_processed = false;
};

asio::awaitable<void> listen(tcp::acceptor& acceptor, server_info& exchange_codes)
{
    for(;;)
    {
        auto [e, client] = co_await acceptor.async_accept(use_nothrow_awaitable);
        if(e)
            break;
        auto ex = client.get_executor();
        auto sess = std::make_shared<Session>(std::move(client), exchange_codes);
        sess->do_read();
    }
}

int main(int argc, char* argv[])
{
    try
    {
        if(argc != 6 )
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
        std::cout << ex.what() << std::endl;
    }
}
