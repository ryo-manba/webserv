#include "RequestMatcher.hpp"
#include "../communication/RequestHTTP.hpp"
#include "../config/Config.hpp"
#include "../utils/File.hpp"
#include "../utils/HTTPError.hpp"
#include "../utils/LightString.hpp"
#include "../utils/UtilsString.hpp"
#include <sys/stat.h>

RequestMatcher::RequestMatcher() {}
RequestMatcher::~RequestMatcher() {}

/**
 * - schemaのバリデーション
 * - 不正なリクエストターゲットのバリデーション
 * - リクエストメソッドのバリデーション
 * - リダイレクトされるかどうかの判定
 * - CGIか通常のファイルでそれぞれのルーティング処理
 */
RequestMatchingResult RequestMatcher::request_match(const std::vector<config::Config> &configs,
                                                    const IRequestMatchingParam &rp) {

    const config::Config &conf = get_config(configs, rp);

    check_routable(rp, conf);

    RequestMatchingResult res;
    const RequestTarget &target = rp.get_request_target();
    if (is_redirect(target, conf)) {
        RequestMatcher::redirect_pair pair = get_redirect(target, conf);
        res.status_code                    = pair.first;
        res.redirect_location              = pair.second;
        res.result_type                    = RequestMatchingResult::RT_EXTERNAL_REDIRECTION;
        return res;
    }
    res.client_max_body_size = get_client_max_body_size(target, conf);
    res.status_page_dict     = get_status_page_dict(target, conf);

    if (is_cgi(target, conf)) {
        return routing_cgi(res, target, conf);
    }
    return routing_default(res, target, rp.get_http_method(), conf);
}

RequestMatchingResult
RequestMatcher::routing_cgi(RequestMatchingResult res, const RequestTarget &target, const config::Config &conf) {
    cgi_resource_pair resource;
    resource              = get_cgi_resource(target, conf);
    res.path_local        = resource.first;
    res.path_after        = resource.second;
    res.path_cgi_executor = get_path_cgi_executor(target, conf, res.path_local);
    res.result_type       = RequestMatchingResult::RT_CGI;
    return res;
}

RequestMatchingResult RequestMatcher::routing_default(RequestMatchingResult res,
                                                      const RequestTarget &target,
                                                      const HTTP::t_method &method,
                                                      const config::Config &conf) {
    std::pair<HTTP::byte_string, bool> path = make_resource_path(target, conf);
    if (path.first.empty()) {
        throw http_error("file not found", HTTP::STATUS_NOT_FOUND);
    }
    res.path_local  = path.first;
    res.result_type = RequestMatchingResult::RT_FILE;
    if (path.second) {
        if (get_is_autoindex(target, conf)) {
            res.result_type = RequestMatchingResult::RT_AUTO_INDEX;
        } else {
            throw http_error("permission denied", HTTP::STATUS_FORBIDDEN);
        }
    } else if (get_is_executable(target, method, conf)) {
        switch (method) {
            case HTTP::METHOD_DELETE:
                res.result_type = RequestMatchingResult::RT_FILE_DELETE;
                break;
            case HTTP::METHOD_POST:
                res.result_type = RequestMatchingResult::RT_FILE_POST;
                break;
            case HTTP::METHOD_PUT:
                res.result_type = RequestMatchingResult::RT_FILE_PUT;
                break;
            default:
                break;
        }
    }
    return res;
}

bool RequestMatcher::get_is_executable(const RequestTarget &target,
                                       const HTTP::t_method &method,
                                       const config::Config &conf) {
    const std::string &path = HTTP::restrfy(target.path.str());

    switch (method) {
        case HTTP::METHOD_GET:
            return true;
        case HTTP::METHOD_POST:
            return conf.get_upload_store(path) != "";
        case HTTP::METHOD_DELETE:
            return conf.get_exec_delete(path);
        default:;
    }
    return false;
}

bool RequestMatcher::get_is_autoindex(const RequestTarget &target, const config::Config &conf) {
    const std::string &path = HTTP::restrfy(target.path.str());
    return conf.get_autoindex(path);
}

config::Config RequestMatcher::get_config(const std::vector<config::Config> &configs, const IRequestMatchingParam &rp) {
    const HTTP::CH::Host host_data = rp.get_host();
    const std::string host         = HTTP::restrfy(host_data.host);

    // host_nameが一致した場合
    for (std::vector<config::Config>::const_iterator it = configs.begin(); it != configs.end(); ++it) {
        if (it->get_host() == host) {
            return *it;
        }
    }
    // default_serverの指定があった場合
    for (std::vector<config::Config>::const_iterator it = configs.begin(); it != configs.end(); ++it) {
        if (it->get_default_server()) {
            return *it;
        }
    }
    // 該当しない場合は先頭のconfigを使う
    return configs.front();
}

void RequestMatcher::check_routable(const IRequestMatchingParam &rp, const config::Config &conf) {
    const RequestTarget &target = rp.get_request_target();

    if (!is_valid_scheme(target)) {
        throw http_error("invalid scheme", HTTP::STATUS_BAD_REQUEST);
    }
    if (!is_valid_path(target)) {
        throw http_error("invalid url target", HTTP::STATUS_BAD_REQUEST);
    }
    if (!is_valid_request_method(target, rp.get_http_method(), conf)) {
        throw http_error("method not allowed", HTTP::STATUS_METHOD_NOT_ALLOWED);
    }
}

bool RequestMatcher::is_valid_scheme(const RequestTarget &target) {
    return target.scheme == "http" || target.scheme == "";
}

bool RequestMatcher::is_valid_path(const RequestTarget &target) {
    const std::vector<light_string> &splitted = target.path.split("/");

    int depth = 0;
    for (std::vector<light_string>::const_iterator it = splitted.begin(); it != splitted.end(); ++it) {
        if (*it == "..") {
            depth -= 1;
        } else if (*it != "" && *it != ".") {
            depth += 1;
        }
        if (depth < 0) {
            return false;
        }
    }
    return true;
}

bool RequestMatcher::is_valid_request_method(const RequestTarget &target,
                                             const HTTP::t_method &method,
                                             const config::Config &conf) {
    const std::string &path                   = HTTP::restrfy(target.path.str());
    const std::set<enum config::Methods> &lmt = conf.get_limit_except(path);

    switch (method) {
        // limit_except自体が定義されていない or limit_exceptに対応するメソッドが定義されていたら対応可能
        case HTTP::METHOD_GET:
            return (lmt.empty() || lmt.find(config::GET) != lmt.end());
        case HTTP::METHOD_DELETE:
            return (lmt.empty() || lmt.find(config::DELETE) != lmt.end());
        case HTTP::METHOD_POST:
            return (lmt.empty() || lmt.find(config::POST) != lmt.end());
        default:;
    }
    return false;
}

bool RequestMatcher::is_redirect(const RequestTarget &target, const config::Config &conf) {
    const std::string &path                         = HTTP::restrfy(target.path.str());
    std::pair<HTTP::t_status, std::string> redirect = conf.get_redirect(path);
    return redirect.first != HTTP::STATUS_REDIRECT_INIT;
}

bool RequestMatcher::is_cgi(const RequestTarget &target, const config::Config &conf) {
    const std::string &path = HTTP::restrfy(target.path.str());
    return conf.get_exec_cgi(path);
}

RequestMatcher::redirect_pair RequestMatcher::get_redirect(const RequestTarget &target,
                                                           const config::Config &conf) const {
    const std::string &path                         = HTTP::restrfy(target.path.str());
    std::pair<HTTP::t_status, std::string> redirect = conf.get_redirect(path);
    return std::make_pair(redirect.first, HTTP::strfy(redirect.second));
}

// ファイルの権限を順番に見ていき、ファイルが存在した時点で、分割する
RequestMatcher::cgi_resource_pair RequestMatcher::get_cgi_resource(const RequestTarget &target,
                                                                   const config::Config &conf) const {
    HTTP::byte_string resource_path;
    HTTP::byte_string path_info;
    const std::string root = conf.get_root(HTTP::restrfy(target.path.str()));
    // ルートとパスをくっつける
    HTTP::byte_string full_path = HTTP::Utils::join_path(HTTP::strfy(root), target.path.str());
    light_string path           = full_path;
    // ルート部分の末尾から見ていく
    for (size_t i = root.size();; i = path.find("/", i)) {
        HTTP::byte_string cur = path.substr(0, i).str();
        if (file::is_file(HTTP::restrfy(cur))) {
            resource_path = cur;
            if (i != HTTP::npos) {
                path_info = path.substr(i, path.size()).str();
            }
            break;
        }
        if (i == HTTP::npos) {
            break;
        }
        i += 1;
    }
    if (resource_path.empty()) {
        throw http_error("file not found", HTTP::STATUS_NOT_FOUND);
    }
    return std::make_pair(resource_path, path_info);
}

RequestMatchingResult::status_dict_type RequestMatcher::get_status_page_dict(const RequestTarget &target,
                                                                             const config::Config &conf) const {
    const std::string &path = HTTP::restrfy(target.path.str());

    const std::map<HTTP::t_status, std::string> error_pages = conf.get_error_page(path);
    RequestMatchingResult::status_dict_type res;
    for (std::map<HTTP::t_status, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end();
         ++it) {
        res[static_cast<HTTP::t_status>(it->first)] = HTTP::strfy(it->second);
    }
    return res;
}

long RequestMatcher::get_client_max_body_size(const RequestTarget &target, const config::Config &conf) const {
    const std::string &path = HTTP::restrfy(target.path.str());
    return conf.get_client_max_body_size(path);
}

// 対応するrootを連結する + indexに対応するファイルを探す
std::pair<HTTP::byte_string, bool> RequestMatcher::make_resource_path(const RequestTarget &target,
                                                                      const config::Config &conf) const {
    const std::string &path = HTTP::restrfy(target.path.str());

    const std::string root                   = conf.get_root(path);
    const HTTP::byte_string resource_path_bs = HTTP::Utils::join_path(HTTP::strfy(root), target.path);
    const std::string resource_path          = HTTP::restrfy(resource_path_bs);
    if (!file::is_dir(resource_path)) {
        if (file::is_file(resource_path)) {
            return std::make_pair(resource_path_bs, false);
        }
        return std::make_pair(HTTP::strfy(""), false);
    }

    std::vector<std::string> indexes = conf.get_index(path);
    for (std::vector<std::string>::iterator it = indexes.begin(); it != indexes.end(); ++it) {
        const HTTP::byte_string cur_bs = HTTP::Utils::join_path(resource_path_bs, HTTP::strfy(*it));
        const std::string cur          = HTTP::restrfy(cur_bs);
        if (file::is_file(cur)) {
            return std::make_pair(cur_bs, false);
        }
    }
    return std::make_pair(resource_path_bs, true);
}

HTTP::byte_string RequestMatcher::get_path_cgi_executor(const RequestTarget &target,
                                                        const config::Config &conf,
                                                        const HTTP::byte_string &cgi_path) const {
    const std::string s = HTTP::restrfy(cgi_path);
    const size_t pos    = s.rfind(".");
    if (pos == std::string::npos) {
        return HTTP::strfy("");
    }
    const std::string &extension           = s.substr(pos + 1);
    const std::string &path                = HTTP::restrfy(target.path.str());
    config::cgi_executer_map cgi_executers = conf.get_cgi_path(path);
    return HTTP::strfy(cgi_executers[extension]);
}
