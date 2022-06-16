#include "HTTPRequest.hpp"
#include "Server.hpp"
#include "test_common.hpp"

#define LF (unsigned char)'\n'
#define CR (unsigned char)'\r'
#define CRLF "\r\n"

HTTPRequestBuilder::HTTPRequestBuilder(void)
{
}

HTTPRequestBuilder::~HTTPRequestBuilder(void)
{
}

bool HTTPRequestBuilder::is_received_up(int fd)
{
    size_t start = 0;
    // 先頭の空行を飛ばす
    while (in_process[fd].data.find(CRLF, start) == start)
    {
        start += 2;
    }
    // ヘッダーの終わりまである場合
    size_t header_end = in_process[fd].data.find("\r\n\r\n", start);
    if (header_end != std::string::npos)
    {
        const std::string header_content_length = "Content-Length:";

        size_t start_pos = in_process[fd].data.find(header_content_length, start);
        // bodyがない場合
        if (start_pos == std::string::npos)
        {
            return true;
        }
        else
        {
            size_t end_pos = in_process[fd].data.find(CRLF, start_pos);
            std::string str = in_process[fd].data.substr(start_pos + header_content_length.size() + 1, end_pos);
            debug(str);
            in_process[fd].content_length = stol(str);

            if (header_end + in_process[fd].content_length <= in_process[fd].data.size())
            {
                return true;
            }
        }
    }
    return false;
}
/**
 * startline, header, bodyに分割する
 */
void HTTPRequestBuilder::divide_data(int fd)
{
    DOUT() << __func__ << std::endl;
    DOUT() << "Request:" << std::endl;
    std::cout << in_process[fd].data << std::endl;

    in_process[fd].phase = START_LINE_END;

    size_t start = 0;
    // 先頭の空行を飛ばす
    while (in_process[fd].data.find(CRLF, start) == start)
    {
        start += 2;
    }
    if (start != 0)
    {
        in_process[fd].data = in_process[fd].data.substr(start, in_process[fd].data.size());
    }
    debug(in_process[fd].phase);

    // 3つに分割する
    size_t end = 0;
    while (in_process[fd].phase != DONE)
    {
        switch (in_process[fd].phase)
        {
        case START_LINE_END:
            end = in_process[fd].data.find(CRLF, start);
            debug(end);
            break;
        case HEADER:
            end = in_process[fd].data.find("\r\n\r\n", start);
            break;
        case BODY:
            end = in_process[fd].content_length;
            break;
        default:;
        }

        switch (in_process[fd].phase)
        {
        case START_LINE_END:
            in_process[fd].start_line = in_process[fd].data.substr(start, end);
            start = end + 2;
            in_process[fd].phase = HEADER;
            break;
        case HEADER:
            in_process[fd].header = in_process[fd].data.substr(start, end - start);
            start = end + 4;
            in_process[fd].phase = BODY;
            break;
        case BODY:
            in_process[fd].body = in_process[fd].data.substr(start, end);
            in_process[fd].phase = DONE;
        default:;
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
void HTTPRequestBuilder::parse_start_line(int fd)
{
    std::vector<std::string> parsing_start_line;

    parsing_start_line = split_string(in_process[fd].start_line, " ");
    if (parsing_start_line.size() != 3)
    {
        throw std::runtime_error("invalid start line");
    }
    parsed_data[fd].start_line = parsing_start_line;
}

// 文字列: 値
void HTTPRequestBuilder::parse_header(int fd)
{
    std::stringstream ss(in_process[fd].header);

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
    parsed_data[fd].header = mp;
}

void HTTPRequestBuilder::parse_body(int fd)
{
    parsed_data[fd].body = in_process[fd].body;
}

// それぞれを適切に分割する
void HTTPRequestBuilder::parse_data(int fd)
{
    parse_start_line(fd);
    parse_header(fd);
    parse_body(fd);

    // DEBUG:
    show_request(fd);
}

// DEBUG:
void HTTPRequestBuilder::show_request(int fd)
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

    std::cout << "METHOD : " << parsed_data[fd].start_line[HTTPRequest::SL_METHOD] << std::endl;
    std::cout << "PATH   : " << parsed_data[fd].start_line[HTTPRequest::SL_PATH] << std::endl;
    std::cout << "VERSION: " << parsed_data[fd].start_line[HTTPRequest::SL_VERSION] << std::endl;

    std::cout << YELLOW
              << "[HEADER]"
              << RESET
              << std::endl;

    for (h_it it = parsed_data[fd].header.begin(); it != parsed_data[fd].header.end(); it++)
    {
        std::cout << it->first << ": " << it->second << std::endl;
    }

    std::cout << YELLOW
              << "[BODY]"
              << RESET
              << std::endl;

    std::cout << parsed_data[fd].body << std::endl;
    std::cout << std::endl;
}
