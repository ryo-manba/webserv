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
    std::map<int, HTTPRequest> parsed_data;

    // 処理中のデータを入れておく箱(fdごとに管理する)
    std::map<int, std::string> in_processing_data;

    struct in_process_t
    {
        std::string data;
        ParsingPhase phase;
        std::string start_line;
        std::string header;
        std::string body;
        size_t content_length;
    };

    std::map<int, in_process_t> in_process;

    HTTPRequestBuilder(void);
    ~HTTPRequestBuilder(void);

    void divide_data(int fd);

    void parse_start_line(int fd);
    void parse_header(int fd);
    void parse_body(int fd);

    void parse_data(int fd);

    // ボディまで読み終わったことを確認する
    bool is_received_up(int fd);

    // debug
    void show_request(int fd);

private:
};

#endif
