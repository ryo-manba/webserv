#include "HTTPRequest.hpp"
#include "Server.hpp"
#include "test_common.hpp"

#define LF (unsigned char)'\n'
#define CR (unsigned char)'\r'
#define CRLF "\r\n"

HTTPRequestBuilder::HTTPRequestBuilder(void)
    : start_line(""),
      header(""),
      body(""),
      phase(START_LINE_END)
{
}

HTTPRequestBuilder::~HTTPRequestBuilder(void)
{
}

/**
 * startline, header, bodyに分割する
 */
bool HTTPRequestBuilder::divide_data(int fd, std::string data)
{
    DOUT() << __func__ << std::endl;
    DOUT() << data << std::endl;

    size_t start = 0;
    // 先頭の空行を飛ばす
    while (data.find(CRLF, start) == start)
    {
        start += 2;
    }
    if (start != 0)
    {
        data = data.substr(start, data.size());
    }

    // 3つに分割する
    size_t end = 0;
    while (1)
    {
        switch (phase)
        {
        case (START_LINE_END):
            end = data.find(CRLF, start);
            break;
        case (HEADER):
            end = data.find("\r\n\r\n", start);
            break;
        case (BODY):
            // content length にする
            end = data.size();
            break;
        default:;
            //            throw std::runtime_error("invalid phase");
        }

        if (end == std::string::npos)
        {
            // CRLFがない不正なリクエスト or そこまで読み込めていない
            // まだ読み終えてない場合は、もう一度recvする
            in_processing_data[fd] = data.substr(start, data.size());
            return false;
        }

        switch (phase)
        {
        case START_LINE_END:
            start_line = data.substr(start, end);
            start = end + 2;
            phase = HEADER;
            break;
        case HEADER:
            header = data.substr(start, end - start);
            start = end + 4;
            phase = BODY;
            break;
        case BODY:
            body = data.substr(start, end - start);
            phase = DONE;
            return true;
        default:;
            //            throw std::runtime_error("invalid phase");
        }
    }
}

std::vector<std::string> split_string(const std::string &s, std::string delim)
{
    std::vector<std::string> split_strs;
    size_t pos = 0;
    size_t start = 0;
    std::string append = "";
    size_t delim_len = delim.size();

    while ((pos = s.find(delim, start)) != std::string::npos)
    {
        append = s.substr(start, pos - start);
        if (!append.empty())
        {
            split_strs.push_back(append);
        }
        start = pos + delim_len;
    }
    if (start < s.size())
    {
        split_strs.push_back(s.substr(start, s.size()));
    }
    return split_strs;
}

// method path version
void HTTPRequestBuilder::parse_start_line(void)
{
    std::vector<std::string> parsing_start_line;

    parsing_start_line = split_string(start_line, " ");
    if (parsing_start_line.size() != 3)
    {
        throw std::runtime_error("invalid start line");
    }
    parsed_data.start_line = parsing_start_line;
}

// 文字列: 値
void HTTPRequestBuilder::parse_header(void)
{
    std::stringstream ss(header);

    std::map<std::string, std::string> mp;

    std::string s;
    while (std::getline(ss, s))
    {
        size_t pos = s.find(":");
        if (pos == std::string::npos)
        {
            throw std::runtime_error("invalid header 400 error");
        }
        std::string key = s.substr(0, pos);
        std::string value = s.substr(pos + 1, s.size());

        mp[key] = value;
    }
    parsed_data.header = mp;
}

void HTTPRequestBuilder::parse_body(void)
{
    parsed_data.body = body;
}

void HTTPRequestBuilder::show_request(void)
{
//    typedef std::vector<std::string>::iterator s_it;
    typedef std::map<std::string, std::string>::iterator h_it;
    const std::string YELLOW = "\e[33m";
    const std::string RESET = "\e[0m";

    std::cout << YELLOW
              << "======= Show request ======="
              << RESET
              << std::endl;

    std::cout << YELLOW
              << "[START LINE]"
              << RESET
              << std::endl;

    std::cout << "METHOD : " << parsed_data.start_line[HTTPRequest::SL_METHOD] << std::endl;
    std::cout << "PATH   : " << parsed_data.start_line[HTTPRequest::SL_PATH] << std::endl;
    std::cout << "VERSION: " << parsed_data.start_line[HTTPRequest::SL_VERSION] << std::endl;

    std::cout << YELLOW
              << "[HEADER]"
              << RESET
              << std::endl;

    for (h_it it = parsed_data.header.begin(); it != parsed_data.header.end(); it++) {
        std::cout << it->first << ": " << it->second << std::endl;
    }

    std::cout << YELLOW
              << "[BODY]"
              << RESET
              << std::endl;

    std::cout << parsed_data.body << std::endl;
    std::cout << std::endl;
}

// それぞれを適切に分割する
void HTTPRequestBuilder::parse_data(void)
{
    parse_start_line();
    parse_header();
    parse_body();

    show_request();
}
