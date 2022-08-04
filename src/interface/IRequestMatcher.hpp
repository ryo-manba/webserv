#ifndef IREQUESTMATCHER_HPP
#define IREQUESTMATCHER_HPP
#include "../communication/RequestHTTP.hpp"
#include "../config/Config.hpp"
#include "../utils/LightString.hpp"
#include "../utils/http.hpp"
#include <map>

struct RequestMatchingResult {

    typedef std::map<HTTP::t_status, HTTP::byte_string> status_dict_type;
    enum ResultType { RT_FILE, RT_EXTERNAL_REDIRECTION, RT_CGI, RT_AUTO_INDEX, RT_ECHO };

    const RequestTarget *target;

    // 種別
    ResultType result_type;

    bool is_cgi;
    bool is_autoindex;
    bool is_redirect;

    std::pair<int, HTTP::byte_string> redirect;

    // メソッドが実行できるか(できない場合はgetとして扱う)
    bool is_executable;

    // リクエストターゲットにマッチしたローカルファイルのパス
    // CGIでいう`SCRIPT_NAME`に相当
    HTTP::byte_string path_local;

    // リクエストターゲットのうちファイルパスにマッチするパートより後の部分
    // CGIでいう`PATH_INFO`に相当
    HTTP::byte_string path_after;

    // リクエスト先がCGIである場合、かつエグゼキュータが特定できた場合、エグゼキュータのパスが入る。
    HTTP::byte_string path_cgi_executor;

    // あるHTTPステータスの時はこのファイルの中身を返す, という時の
    // ステータスとファイルパスの辞書.
    status_dict_type status_page_dict;

    // 必要な場合, HTTPステータスコードが入る
    HTTP::t_status status_code;

    // 外部リダイレクトの行先
    HTTP::byte_string redirect_location;

    // confから決まる「HTTPリクエストのボディのサイズ上限」
    HTTP::byte_string::size_type max_body_size;

    long client_max_body_size;
};
class IRequestMatcher {
public:
    ~IRequestMatcher() {}

    virtual RequestMatchingResult request_match(const std::vector<config::Config> &configs,
                                                const IRequestMatchingParam &rp)
        = 0;
};

#endif
