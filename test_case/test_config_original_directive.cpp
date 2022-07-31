#include "../../src/config/Context.hpp"
#include "../../src/config/File.hpp"
#include "../../src/config/Lexer.hpp"
#include "../../src/config/Parser.hpp"
#include "gtest/gtest.h"
#include <iostream>
#include <vector>

namespace {
class ConfigTest : public testing::Test {
protected:
    ConfigTest() {}
    virtual ~ConfigTest() {}
    virtual void SetUp(const std::string &data) {
        configs = parser.parse(data);
    }

    config::Parser parser;
    std::map<config::host_port_pair, std::vector<config::Config> > configs;
};

TEST_F(ConfigTest, GetExecCgi) {
    const std::string config_data = "\
http { \
    server { \
        listen 80; \
        location /on/ { \
            exec_cgi on; \
        } \
        location /off/ { \
            exec_cgi off; \
        } \
    } \
} \
";
    SetUp(config_data);
    const config::host_port_pair hp = std::make_pair("0.0.0.0", 80);
    const config::Config conf       = configs[hp].front();

    EXPECT_EQ(false, conf.get_exec_cgi("/"));
    EXPECT_EQ(true, conf.get_exec_cgi("/on/"));
    EXPECT_EQ(false, conf.get_exec_cgi("/off/"));
}

TEST_F(ConfigTest, GetExecDelete) {
    const std::string config_data = "\
http { \
    server { \
        listen 80; \
        location /on/ { \
            exec_delete on; \
        } \
        location /off/ { \
            exec_delete off; \
        } \
    } \
} \
";
    SetUp(config_data);
    const config::host_port_pair hp = std::make_pair("0.0.0.0", 80);
    const config::Config conf       = configs[hp].front();
    EXPECT_EQ(false, conf.get_exec_delete("/"));
    EXPECT_EQ(true, conf.get_exec_delete("/on/"));
    EXPECT_EQ(false, conf.get_exec_delete("/off/"));
}

TEST_F(ConfigTest, GetCgiPath) {
    const std::string config_data = "\
http { \
    server { \
        listen 80; \
        location /ruby/ { \
            exec_cgi on; \
            cgi_path .rb /usr/bin/ruby; \
        } \
        location /python/ { \
            exec_cgi on; \
            cgi_path .py /usr/bin/python; \
        } \
    } \
} \
";
    SetUp(config_data);
    const config::host_port_pair hp = std::make_pair("0.0.0.0", 80);
    const config::Config conf       = configs[hp].front();

    {
        const config::cgi_path_map expected;
        const config::cgi_path_map actual = conf.get_cgi_path("/");
        EXPECT_EQ(expected.empty(), actual.empty());
    }
    {
        config::cgi_path_map expected;
        const std::string extension = ".rb";
        expected[".rb"]             = "/usr/bin/ruby";
        config::cgi_path_map actual = conf.get_cgi_path("/ruby/");
        EXPECT_EQ(expected.size(), actual.size());
        EXPECT_EQ(expected[extension], actual[extension]);
    }
    {
        config::cgi_path_map expected;
        const std::string extension = ".py";
        expected[extension]         = "/usr/bin/python";
        config::cgi_path_map actual = conf.get_cgi_path("/python/");
        EXPECT_EQ(expected.size(), actual.size());
        EXPECT_EQ(expected[extension], actual[extension]);
    }
}

} // namespace