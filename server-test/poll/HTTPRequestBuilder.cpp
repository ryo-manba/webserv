#include "HTTPRequest.hpp"

#include "Server.hpp"
#define LF (unsigned char)'\n'
#define CR (unsigned char)'\r'
#define CRLF "\r\n"

HTTPRequestBuilder::HTTPRequestBuilder()
    : parsed_data(HTTPRequest()),
      start_line(""),
      header(""),
      body(""),
      in_processing_data(""),
      phase(START_LINE_END)
{
}

HTTPRequestBuilder::~HTTPRequestBuilder()
{
}


/**
 * startline, header, bodyに分割する
 */
bool HTTPRequestBuilder::divide_data(std::string data)
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
        data = data.substr(start, data.size() - start);
    }

    debug(start);

    // 3つに分割する
    size_t end = 0;
    while (1)
    {
        debug(phase);
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
            debug("in");
            return false;
        }

        switch (phase)
        {
        case START_LINE_END:
            start_line = data.substr(start, end);
            start = end + 2;
            phase = HEADER;
            debug(start_line);
            break;
        case HEADER:
            header = data.substr(start, end - start);
            start = end + 4;
            phase = BODY;
            debug(header);
            break;
        case BODY:
            body = data.substr(start, end - start);
            debug(body);
            phase = DONE;
            return true;
        default:;
            //            throw std::runtime_error("invalid phase");
        }
    }
}

HTTPRequest HTTPRequestBuilder::parse_data()
{
    return parsed_data;
}
