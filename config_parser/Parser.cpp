#include "Parser.hpp"
#include "Lexer.hpp"
#include "Validator.hpp"
#include "test_common.hpp"
#include <iostream>
#include <map>
#include <sstream>
#include <utility>

Parser::Parser() {}
Parser::~Parser() {}

const std::string Parser::None = "";

std::string syntax_error(const std::string &message, const size_t &line = 0) {
    std::ostringstream oss;

    oss << "config: ";
    oss << message << " in line: " << line;
    return oss.str();
}

std::vector<Directive> Parser::Parse(std::string filename) {
    // トークンごとに分割する
    lexer_.lex(filename);

    std::string err;
    if ((err = brace_balanced()) != None) {
        throw ConfigValidationException(err);
    }
    std::vector<Directive> parsed = parse();

    return parsed;
}

std::vector<std::string> Parser::enter_block_ctx(Directive dire, std::vector<std::string> ctx) {
    if (ctx.size() > 0 && ctx[0] == "http" && dire.directive == "location") {
        ctx.clear();
        ctx.push_back("http");
        ctx.push_back("location");
        return ctx;
    }

    // location以外はネストすることができないので追加する
    ctx.push_back(dire.directive);
    return ctx;
}

std::string Parser::brace_balanced(void) {
    int depth = 0;
    int line  = 0;

    Lexer::wsToken *tok;
    while ((tok = lexer_.read()) != NULL) {
        line = tok->line;
        if (tok->value == "}" && !tok->is_quoted) {
            depth -= 1;
        } else if (tok->value == "{" && !tok->is_quoted) {
            depth += 1;
        }
        if (depth < 0) {
            return syntax_error("unexpected \"}\"", line);
        }
    }
    if (depth > 0) {
        return syntax_error("unexpected end of file, expecting \"}\"", line);
    }
    lexer_.reset_read_idx();
    return None;
}

/**
 * @brief lexerによって分割されたtokenを解析する
 * 1. "}"のチェック
 * 2. 引数のチェック
 * 3. 構文解析
 *  - ディレクティブが正しいか
 *  - 引数の数が正しいか
 * 4. ブロックディレクティブの場合は再帰的にパースする
 */
std::vector<Directive> Parser::parse(std::vector<std::string> ctx) {
    std::vector<Directive> parsed;

    Lexer::wsToken *cur;
    while (1) {
        cur = lexer_.read();
        if (cur == NULL) {
            return parsed;
        }

        if (cur->value == "}" && !cur->is_quoted) {
            break;
        }

        Directive dire(cur->value, cur->line);

        cur = lexer_.read();
        if (cur == NULL) {
            return parsed;
        }

        // 引数のパース(特殊文字が来るまで足し続ける)
        // TODO: is_specialに置き換える
        while (cur->is_quoted || (cur->value != "{" && cur->value != ";" && cur->value != "}")) {
            dire.args.push_back(cur->value);
            cur = lexer_.read();
            if (cur == NULL) {
                return parsed;
            }
        }
        std::string err;
        if ((err = validate(dire, cur->value, ctx)) != None) {
            throw ConfigValidationException(err);
        }

        // "{" で終わってた場合はcontextを調べる
        std::vector<std::string> inner;
        if (cur->value == "{" && !cur->is_quoted) {
            inner      = enter_block_ctx(dire, ctx); // get context for block
            dire.block = parse(inner);
        }
        parsed.push_back(dire);
    }

    return parsed;
}

/*************************************************************/
// debug
void print(std::vector<Directive> d, bool is_block, std::string before) {
    std::string dir;
    if (is_block) {
        dir = "block";
    } else {
        dir = "Dire";
    }
    for (size_t i = 0; i < d.size(); i++) {
        std::cout << before << dir << "[" << i << "].dire   : " << d[i].directive << std::endl;

        if (d[i].args.size() == 0) {
            std::cout << before << dir << "[" << i << "].args   : {}" << std::endl;
        } else {
            for (size_t j = 0; j < d[i].args.size(); j++) {
                std::cout << before << dir << "[" << i << "].args[" << j << "]: " << d[i].args[j] << std::endl;
            }
        }
        if (d[i].block.size() == 0) {
            std::cout << before << dir << "[" << i << "].block  : {}" << std::endl;
        } else {
            std::string b = before + dir + "[" + std::to_string(i) + "]" + ".";
            print(d[i].block, true, b);
        }
    }
}
/*************************************************************/
