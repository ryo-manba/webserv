#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP
#include <string>
#include <map>
#include <vector>

class HTTPRequest
{
public:
    enum STARTLINE_KEYS
    {
        SL_METHOD = 0,
        SL_PATH,
        SL_VERSION
    };

    HTTPRequest(void);
    ~HTTPRequest(void);

    std::vector<std::string> start_line;
    std::map<std::string, std::string> header;
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

    // 処理中のデータを入れておく箱(fdごとに管理する)
    std::map<int, std::string> in_processing_data;

    ParsingPhase phase;

    HTTPRequestBuilder(void);
    ~HTTPRequestBuilder(void);

    bool divide_data(int fd, std::string data);

    void parse_start_line(void);
    void parse_header(void);
    void parse_body(void);

    void parse_data(void);

    // debug
    void show_request(void);

private:
};

#endif
