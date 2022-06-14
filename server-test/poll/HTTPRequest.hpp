#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP
#include <string>

class HTTPRequest
{
public:
    HTTPRequest();
    ~HTTPRequest();

    std::string start_line;
    std::string header;
    std::string body;
};

// 最終的にパースしたものを返す
class HTTPRequestBuilder
{
public:
    enum ParsingPhase
    {
        START_LINE_START, // 0個以上の連続した改行
        START_LINE_END,
        HEADER,
        BODY,
        DONE
    };
    // これにパースしたものを詰めていって返す
    HTTPRequest parsed_data;

    // まずは分割する
    std::string start_line;
    std::string header;
    std::string body;

    // 処理中のデータを入れておく箱
    std::string in_processing_data;

    ParsingPhase phase;

    HTTPRequestBuilder();
    ~HTTPRequestBuilder();

    bool divide_data(std::string data);

    HTTPRequest parse_data();
    void parse_start_line();
    void parse_header();
    void parse_body();

private:
};

#endif
