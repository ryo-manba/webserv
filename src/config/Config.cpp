#include "Config.hpp"
#include <stack>

namespace config {

Config::Config(const ContextServer &srv) : ctx_server_(srv){};
Config::~Config(void){};

void Config::set_host_port(const host_port_pair &hp) {
    host_port_ = hp;
}

bool Config::get_autoindex(const std::string &target) const {
    const ContextLocation &loc = longest_prefix_match_location(ctx_server_, target);
    return loc.autoindex;
}

std::map<int, std::string> Config::get_error_page(const std::string &target) const {
    const ContextLocation &loc = longest_prefix_match_location(ctx_server_, target);
    return loc.error_pages;
}

std::vector<std::string> Config::get_index(const std::string &target) const {
    const ContextLocation &loc = longest_prefix_match_location(ctx_server_, target);

    // indexが指定されていない場合は, index.htmlを設定する
    if (loc.indexes.empty()) {
        std::vector<std::string> res;
        res.push_back("index.html");
        return res;
    }
    return loc.indexes;
}

std::string Config::get_root(const std::string &target) const {
    const ContextLocation &loc = longest_prefix_match_location(ctx_server_, target);
    return loc.root;
}

long Config::get_client_max_body_size(const std::string &target) const {
    const ContextLocation &loc = longest_prefix_match_location(ctx_server_, target);
    return loc.client_max_body_size;
}

std::string Config::get_host(const std::string &target) const {
    (void)target;
    return host_port_.first;
}

int Config::get_port(const std::string &target) const {
    (void)target;
    return host_port_.second;
}

std::pair<int, std::string> Config::get_redirect(const std::string &target) const {
    const ContextLocation &loc = longest_prefix_match_location(ctx_server_, target);
    return loc.redirect;
}

std::vector<std::string> Config::get_server_name(const std::string &target) const {
    (void)target;
    return ctx_server_.server_names;
}

std::string Config::get_upload_store(const std::string &target) const {
    (void)target;
    return ctx_server_.upload_store;
}

bool Config::get_default_server(const std::string &target) const {
    (void)target;
    return ctx_server_.default_server;
}

/**
 * サーバーコンテキストの中で最長前方一致するロケーションを返す
 * 一致しない場合はサーバーコンテキストの情報をそのまま継承したロケーションを返す
 */
ContextLocation Config::longest_prefix_match_location(const ContextServer &srv, const std::string &path) const {
    ContextLocation longest(srv);

    std::stack<ContextLocation> sta;
    for (std::vector<ContextLocation>::const_iterator it = srv.locations.begin(); it != srv.locations.end(); ++it) {
        sta.push(*it);
    }

    while (!sta.empty()) {
        ContextLocation cur = sta.top();
        sta.pop();
        // 一致していたら子要素をstackに積む
        if (path.find(cur.path) == 0) {
            for (std::vector<ContextLocation>::const_iterator it = cur.locations.begin(); it != cur.locations.end();
                 ++it) {
                sta.push(*it);
            }
            // マッチしてる部分が長い場合は更新する
            if (longest.path.size() < cur.path.size()) {
                longest = cur;
            }
        }
    }
    return longest;
}

} // namespace config
