#include "ControlHeaderHTTP.hpp"
#include "RoutingParameters.hpp"

typedef HTTP::byte_string byte_string;
typedef HTTP::light_string light_string;

// [TransferCoding]

void HTTP::Term::TransferCoding::store_list_item(const parameter_key_type &key, const parameter_value_type &val) {
    parameters[key] = val;
    // DXOUT("\"" << key << "\" << \"" << parameters[key] << "\"");
}

HTTP::Term::TransferCoding HTTP::Term::TransferCoding::init() {
    HTTP::Term::TransferCoding o;
    o.quality_int = 0;
    return o;
}

bool HTTP::CH::TransferEncoding::empty() const {
    return transfer_codings.empty();
}

const HTTP::Term::TransferCoding &HTTP::CH::TransferEncoding::current_coding() const {
    return transfer_codings.back();
}

HTTP::byte_string HTTP::CH::TransferEncoding::normalize(const HTTP::byte_string &str) {
    return HTTP::Utils::downcase(str);
}

minor_error HTTP::CH::TransferEncoding::determine(const AHeaderHolder &holder) {
    // https://httpwg.org/specs/rfc7230.html#header.transfer-encoding
    // Transfer-Encoding  = 1#transfer-coding
    // transfer-coding    = "chunked"
    //                    / "compress"
    //                    / "deflate"
    //                    / "gzip"
    //                    / transfer-extension
    // transfer-extension = token *( OWS ";" OWS transfer-parameter )
    // transfer-parameter = token BWS "=" BWS ( token / quoted-string )

    // transfer-coding は大文字小文字関係ない.
    // https://www.rfc-editor.org/rfc/rfc9112#section-7
    // "All transfer-coding names are case-insensitive"

    const AHeaderHolder::value_list_type *tes = holder.get_vals(HeaderHTTP::transfer_encoding);
    if (!tes) {
        return minor_error::ok();
    }
    int i = 0;
    for (AHeaderHolder::value_list_type::const_iterator it = tes->begin(); it != tes->end(); ++it) {
        ++i;
        // VOUT(i);
        light_string val_lstr = light_string(*it);
        for (;;) {
            // QVOUT(val_lstr);
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            if (val_lstr.size() == 0) {
                DXOUT("away; sp only.");
                break;
            }
            light_string coding_str = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (coding_str.size() == 0) {
                DXOUT("away; no value.");
                break;
            }

            // 本体
            HTTP::Term::TransferCoding tc;
            tc.coding = normalize(coding_str.str());
            // QVOUT(tc.coding);
            this->transfer_codings.push_back(tc);
            // QVOUT(transfer_encoding.transfer_codings.back().coding);

            // 後続
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp, coding_str.size());
            if (val_lstr.size() == 0) {
                // DXOUT("away");
                break;
            }
            // cat(sp_end) が "," か ";" かによって分岐
            // QVOUT(val_lstr);
            if (val_lstr[0] == ';') {
                // parameterがはじまる
                val_lstr
                    = ARoutingParameters::decompose_semicoron_separated_kvlist(val_lstr, this->transfer_codings.back());
            }
            // QVOUT(val_lstr);
            if (val_lstr.size() > 0 && val_lstr[0] == ',') {
                // 次の要素
                val_lstr = val_lstr.substr(1);
            } else if (val_lstr.size() > 0 && val_lstr[0] == ';') {
                // parameterがはじまる
                val_lstr
                    = ARoutingParameters::decompose_semicoron_separated_kvlist(val_lstr, this->transfer_codings.back());
            } else {
                // 不正な要素
                DXOUT("[KO] unexpected state: \"" << val_lstr.substr(0) << "\"");
                break;
            }
        }
    }
    this->currently_chunked = !this->empty() && this->current_coding().coding == "chunked";
    // QVOUT(this->transfer_codings.back().coding);
    return minor_error::ok();
}

// [ContentLength]

HTTP::CH::ContentLength::ContentLength() : value(0) {}

bool HTTP::CH::ContentLength::empty() const {
    return lengths.size() == 0;
}

minor_error HTTP::CH::ContentLength::determine(const AHeaderHolder &holder) {
    // https://www.rfc-editor.org/rfc/rfc9110.html#name-content-length
    // Content-Length = 1*DIGIT
    lengths.clear();
    merror                                    = minor_error::ok();
    const AHeaderHolder::value_list_type *les = holder.get_vals(HeaderHTTP::content_length);
    if (!les) {
        return minor_error::ok();
    }
    for (AHeaderHolder::value_list_type::const_iterator it = les->begin(); it != les->end(); ++it) {
        std::pair<bool, unsigned int> res = ParserHelper::str_to_u(*it);
        // VOUT(res.first);
        // VOUT(res.second);
        if (!res.first) {
            // 変換に失敗 -> そういうエラー, ただし外部には返さない
            merror = erroneous(merror, minor_error::make("non-numeric content-length", HTTP::STATUS_BAD_REQUEST));
            continue;
        }
        lengths.insert(res.second);
    }
    if (lengths.size() == 1) {
        // 集合 lengths のサイズがぴったり1の時
        // -> 唯一の要素を value にセット
        value = *(lengths.begin());
    } else if (lengths.size() > 1) {
        // lengths に複数の値がある時
        // -> そういうエラー, ただし外部には返さない
        merror = erroneous(merror, minor_error::make("multiple content-length", HTTP::STATUS_BAD_REQUEST));
    }
    // VOUT(value);
    // VOUT(merror);
    return minor_error::ok();
}

// [ContentType]

const HTTP::byte_string HTTP::CH::ContentType::default_value = HTTP::strfy("application/octet-stream");

void HTTP::CH::ContentType::store_list_item(const parameter_key_type &key, const parameter_value_type &val) {
    const parameter_key_type normalized_key = normalize(key);
    parameters[normalized_key]              = val;
}

minor_error HTTP::CH::ContentType::determine(const AHeaderHolder &holder) {
    // https://datatracker.ietf.org/doc/html/rfc3875#section-4.1.3
    // If the request includes a message-body, the CONTENT_TYPE variable is
    // set to the Internet Media Type [6] of the message-body.

    //     CONTENT_TYPE = "" | media-type
    //     media-type   = type "/" subtype *( ";" parameter )
    //     type         = token
    //     subtype      = token
    //     parameter    = attribute "=" value
    //     attribute    = token
    //     value        = token | quoted-string

    // The type, subtype and parameter attribute names are not
    // case-sensitive.  Parameter values may be case sensitive.  Media types
    // and their use in HTTP are described section 3.7 of the HTTP/1.1
    // specification [4].

    // HTTPにはOWSがあるが, CGIにはない.
    // -> OWSがあるのはparameter部分だけなので, そこに差異を押し込められそう.

    const byte_string *ct = holder.get_val(HeaderHTTP::content_type);
    if (!ct || ct->size() == 0) {
        this->value = HTTP::CH::ContentType::default_value;
        return minor_error::ok();
    }
    const light_string lct(*ct);
    // [`type`の捕捉]
    // type = token = 1*tchar
    light_string::size_type type_end = lct.find_first_not_of(HTTP::CharFilter::tchar);
    if (type_end == light_string::npos) {
        // `type`がスラッシュ'/'で終わっていない
        // DXOUT("[KO] no /: \"" << lct << "\"");
        return minor_error::ok();
    }
    // [`/`の捕捉]
    if (lct[type_end] != '/') {
        // スラッシュ'/'がない
        DXOUT("[KO] not separated by /: \"" << lct << "\"");
        return minor_error::ok();
    }
    light_string type_str(lct, 0, type_end);
    // [`subtype`の捕捉]
    // subtype = token = 1*char
    light_string::size_type subtype_end = lct.find_first_not_of(HTTP::CharFilter::tchar, type_end + 1);
    light_string subtype_str(lct, type_end + 1, subtype_end);
    if (subtype_str.size() == 0) {
        // スラッシュ'/'のあとに`subtype`がない
        // DXOUT("[KO] no subtype after /: \"" << lct << "\"");
        return minor_error::ok();
    }
    this->value = normalize(lct.substr(0, subtype_end).str());
    // QVOUT(value);
    // [`parameter`の捕捉]
    // parameter  = 1*tchar "=" ( 1*tchar / quoted-string )
    light_string parameters_str(lct, subtype_end);
    ARoutingParameters::decompose_semicoron_separated_kvlist(parameters_str, *this);

    // boundary の検出
    if (value == "multipart/form-data") {
        HTTP::IDictHolder::parameter_dict::const_iterator res = parameters.find(HTTP::strfy("boundary"));
        if (res != parameters.end() && res->second.size() > 0) {
            // マルチパートである?
            const HTTP::light_string boundary_candidate = res->second;
            // 文字数が 1 ~ 70かどうか
            const bool length_is_right = 1 <= boundary_candidate.size() && boundary_candidate.size() <= 70;
            // 使用可能文字のみかどうか
            const bool chars_right
                = boundary_candidate.find_first_not_of(HTTP::CharFilter::boundary_char) == light_string::npos;
            if (length_is_right && chars_right) {
                boundary = boundary_candidate;
            }
        }
    }
    // QVOUT(boundary);
    return minor_error::ok();
}

HTTP::byte_string HTTP::CH::ContentType::normalize(const HTTP::byte_string &str) {
    return HTTP::Utils::downcase(str);
}

// [ContentDisposition]

void HTTP::CH::ContentDisposition::store_list_item(const parameter_key_type &key, const parameter_value_type &val) {
    parameters[key] = val;
    VOUT(key);
    VOUT(val);
}

minor_error HTTP::CH::ContentDisposition::determine(const AHeaderHolder &holder) {

    //  content-disposition = "Content-Disposition" ":"
    //                         disposition-type *( ";" disposition-parm )
    //  disposition-type    = "inline" | "attachment" | disp-ext-type
    //                      ; case-insensitive
    //  disp-ext-type       = token
    //  disposition-parm    = filename-parm | disp-ext-parm
    //  filename-parm       = "filename" "=" value
    //                      | "filename*" "=" ext-value
    //  disp-ext-parm       = token "=" value
    //                      | ext-token "=" ext-value
    //  ext-token           = <the characters in token, followed by "*">

    const byte_string *ct = holder.get_val(HeaderHTTP::content_disposition);
    if (!ct || ct->size() == 0) {
        return minor_error::ok();
    }
    const light_string lct(*ct);
    // disposition-type の補足
    VOUT(lct);
    light_string::size_type type_end = lct.find_first_not_of(HTTP::CharFilter::cd_token_char);
    if (type_end == light_string::npos) {
        //
        DXOUT("[KO] no type: \"" << lct << "\"");
        return minor_error::ok();
    }
    this->value = lct.substr(0, type_end).str();
    VOUT(this->value);
    // [`parameter`の捕捉]
    // parameter  = 1*tchar "=" ( 1*tchar / quoted-string )
    light_string parameters_str(lct, type_end);
    light_string continuation = ARoutingParameters::decompose_semicoron_separated_kvlist(parameters_str, *this);
    (void)continuation;
    return minor_error::ok();
}

// [TE]

minor_error HTTP::CH::TE::determine(const AHeaderHolder &holder) {
    // https://triple-underscore.github.io/RFC7230-ja.html#header.te
    // TE        = #t-codings
    // t-codings = "trailers" / ( transfer-coding [ t-ranking ] )
    // t-ranking = OWS ";" OWS "q=" rank
    // rank      = ( "0" [ "." 0*3DIGIT ] )
    //           / ( "1" [ "." 0*3("0") ] )
    // transfer-coding    = "chunked"
    //                    / "compress"
    //                    / "deflate"
    //                    / "gzip"
    //                    / transfer-extension
    // transfer-extension = token *( OWS ";" OWS transfer-parameter )
    // transfer-parameter = token BWS "=" BWS ( token / quoted-string )

    const AHeaderHolder::value_list_type *tes = holder.get_vals(HeaderHTTP::te);
    if (!tes) {
        return minor_error::ok();
    }
    for (AHeaderHolder::value_list_type::const_iterator it = tes->begin(); it != tes->end(); ++it) {
        light_string val_lstr = light_string(*it);
        for (;;) {
            // TE: のvalueがスペースしかない
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            if (val_lstr.size() == 0) {
                DXOUT("away; sp only.");
                return minor_error::ok();
            }
            light_string tc_lstr = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (tc_lstr.size() == 0) {
                // TE: のvalueに有効な文字がない
                DXOUT("away; no value.");
                return minor_error::ok();
            }

            // 本体
            // DXOUT("tc_lstr: \"" << tc_lstr << "\"");
            HTTP::Term::TransferCoding tc = HTTP::Term::TransferCoding::init();
            tc.coding                     = tc_lstr.str();
            this->transfer_codings.push_back(tc);

            // 後続
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp, tc_lstr.size());
            if (val_lstr.size() == 0) {
                // 値の後にSP以外なにもない
                DXOUT("away");
                return minor_error::ok();
            }

            // DXOUT("val_lstr[0]: " << val_lstr[0]);
            HTTP::Term::TransferCoding &last_coding = this->transfer_codings.back();
            if (val_lstr[0] == ';') {
                // parameterがはじまる
                val_lstr = ARoutingParameters::decompose_semicoron_separated_kvlist(val_lstr, last_coding);
            }

            // rankがあるなら, それはセミコロン分割リストの一部として解釈されるはず -> q 値をチェック.
            if (!this->transfer_codings.empty()) {
                HTTP::IDictHolder::parameter_dict::iterator qit = last_coding.parameters.find(HTTP::strfy("q"));
                if (qit != last_coding.parameters.end()) {
                    // DXOUT("rank: \"" << qit->second << "\"");
                    if (HTTP::Validator::is_valid_rank(qit->second)) {
                        // DXOUT("rank is valid: " << qit->second);
                        // DXOUT("q: " << last_coding.quality_int);
                        last_coding.quality_int = ParserHelper::quality_to_u(qit->second);
                        // DXOUT("q -> " << last_coding.quality_int);
                    } else {
                        // DXOUT("rank is INVALID: " << qit->second);
                    }
                }
            }

            if (val_lstr.size() == 0) {
                DXOUT("away");
                break;
            }
            if (val_lstr[0] == ',') {
                // 次の要素
                val_lstr = val_lstr.substr(1);
            } else {
                // 不正な要素
                DXOUT("[KO] unexpected state: \"" << val_lstr.substr(0) << "\"");
                break;
            }
        }
    }
    return minor_error::ok();
}

// [Upgrade]

minor_error HTTP::CH::Upgrade::determine(const AHeaderHolder &holder) {
    // Upgrade          = 1#protocol
    // protocol         = protocol-name ["/" protocol-version]
    // protocol-name    = token
    // protocol-version = token
    const AHeaderHolder::value_list_type *elems = holder.get_vals(HeaderHTTP::upgrade);
    if (!elems) {
        return minor_error::ok();
    }
    for (AHeaderHolder::value_list_type::const_iterator it = elems->begin(); it != elems->end(); ++it) {
        light_string val_lstr = light_string(*it);
        for (; val_lstr.size() > 0;) {
            val_lstr          = val_lstr.substr_after(HTTP::CharFilter::sp);
            light_string name = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (name.size() == 0) {
                // 値なし
                DXOUT("[KO] no name");
                break;
            }
            HTTP::Term::Protocol protocol;
            protocol.name = name;
            val_lstr      = val_lstr.substr(name.size());
            if (val_lstr.size() > 0 && val_lstr[0] == '/') {
                // versionがある
                light_string version = val_lstr.substr_while(HTTP::CharFilter::tchar, 1);
                if (version.size() == 0) {
                    // 値なし
                    DXOUT("[KO] no version after slash");
                } else {
                    protocol.version = version;
                }
                val_lstr = val_lstr.substr(1 + version.size());
            }
            DXOUT("protocol: " << protocol.name.qstr() << " - " << protocol.version.qstr());
            this->protocols.push_back(protocol);
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            if (val_lstr.size() == 0 || val_lstr[0] != ',') {
                DXOUT("away");
                break;
            }
            val_lstr = val_lstr.substr(1);
        }
    }
    return minor_error::ok();
}

// [Via]

minor_error HTTP::CH::Via::determine(const AHeaderHolder &holder) {
    // Via = 1#( received-protocol RWS received-by [ RWS comment ] )
    // received-protocol = [ protocol-name "/" ] protocol-version
    // received-by       = ( uri-host [ ":" port ] ) / pseudonym
    // pseudonym         = token
    // protocol-name     = token
    // protocol-version  = token
    const AHeaderHolder::value_list_type *elems = holder.get_vals(HeaderHTTP::via);
    if (!elems) {
        return minor_error::ok();
    }
    for (AHeaderHolder::value_list_type::const_iterator it = elems->begin(); it != elems->end(); ++it) {
        light_string val_lstr = light_string(*it);
        for (;;) {
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            // QVOUT(val_lstr);
            if (val_lstr.size() == 0) {
                break;
            }
            const light_string p1 = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (p1.size() == 0) {
                DXOUT("[KO] zero-width token(1): " << val_lstr.qstr());
                break;
            }
            val_lstr = val_lstr.substr(p1.size());
            light_string p2;
            if (val_lstr.size() > 0 && val_lstr[0] != ' ') {
                if (val_lstr[0] != '/') {
                    DXOUT("[KO] invalid separator: " << val_lstr.qstr());
                    break;
                }
                p2       = val_lstr.substr_while(HTTP::CharFilter::tchar, 1);
                val_lstr = val_lstr.substr(1 + p2.size());
            }
            HTTP::Term::Received r;
            HTTP::Term::Protocol &p = r.protocol;
            if (p2.size() == 0) {
                p.version = p1;
            } else {
                p.name    = p1;
                p.version = p2;
            }
            DXOUT("protocol: name=" << p.name.qstr() << ", version=" << p.version.qstr());
            light_string rws = val_lstr.substr_while(HTTP::CharFilter::sp);
            if (rws.size() >= 1) {
                val_lstr        = val_lstr.substr(rws.size());
                light_string by = val_lstr.substr_before(HTTP::CharFilter::sp | ",");
                DXOUT("by: " << by.qstr());
                bool is_valid_as_uri_host = HTTP::Validator::is_uri_host(by);
                if (!is_valid_as_uri_host) {
                    DXOUT("[KO] not valid as host: " << by.qstr());
                    break;
                }
                ARoutingParameters::pack_host(r.host, by);
                val_lstr = val_lstr.substr(by.size());
            }
            this->receiveds.push_back(r);
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);

            // const light_string comment = extract_comment(val_lstr);
            // QVOUT(comment);
            // QVOUT(val_lstr);

            if (val_lstr.size() > 0 && val_lstr[0] == ',') {
                val_lstr = val_lstr.substr(1);
                continue;
            }
            break;
        }
    }
    return minor_error::ok();
}

// [Date]
minor_error HTTP::CH::Date::determine(const AHeaderHolder &holder) {
    // https://www.rfc-editor.org/rfc/rfc9110.html#name-date
    // https://www.rfc-editor.org/rfc/rfc9110.html#name-date-time-formats
    //
    //   Date         = HTTP-date
    // (See ParserHelpher::http_date)

    merror                                     = minor_error::ok();
    value                                      = 0;
    const AHeaderHolder::value_list_type *vals = holder.get_vals(HeaderHTTP::date);
    VOUT(vals);
    if (!vals) {
        return minor_error::ok();
    }
    std::set<t_time_epoch_ms> ts;
    for (AHeaderHolder::value_list_type::const_iterator it = vals->begin(); it != vals->end(); ++it) {
        std::pair<bool, t_time_epoch_ms> res = ParserHelper::str_to_http_date(*it);
        if (res.first) {
            ts.insert(res.second);
        }
    }
    VOUT(ts.size());
    if (ts.size() == 1) {
        value = *(ts.begin());
    } else if (ts.size() > 1) {
        merror = minor_error::make("multiple valid dates", HTTP::STATUS_BAD_REQUEST);
    }
    VOUT(merror);
    VOUT(value);
    return minor_error::ok();
}

// [Location]

minor_error HTTP::CH::Location::determine(const AHeaderHolder &holder) {
    // https://datatracker.ietf.org/doc/html/rfc3875#section-6.3.2
    // The Location header field is used to specify to the server that the
    // script is returning a reference to a document rather than an actual
    // document (see sections 6.2.3 and 6.2.4).  It is either an absolute
    // URI (optionally with a fragment identifier), indicating that the
    // client is to fetch the referenced document, or a local URI path
    // (optionally with a query string), indicating that the server is to
    // fetch the referenced document and return it to the client as the
    // response.

    //     Location        = local-Location | client-Location
    //     client-Location = "Location:" fragment-URI NL
    //     fragment-URI    = absoluteURI [ "#" fragment ]
    //     fragment        = *uric
    //     local-Location  = "Location:" local-pathquery NL
    //     local-pathquery = abs-path [ "?" query-string ]
    //     abs-path        = "/" path-segments
    //     path-segments   = segment *( "/" segment )
    //     segment         = *pchar
    //     pchar           = unreserved | escaped | extra
    //     extra           = ":" | "@" | "&" | "=" | "+" | "$" | ","
    //     escaped         = "%" hex hex

    const byte_string *ct = holder.get_val(HeaderHTTP::location);
    if (!ct || ct->size() == 0) {
        this->value.clear();
        this->is_local = false;
        return minor_error::ok();
    }
    this->value = *ct;
    light_string lct(*ct);
    if (lct[0] == '/') {
        // local-Location?
        // `local-pathquery`
        // = abs-path [ "?" query-string ]
        // = "/" path-segments [ "?" *uric ]
        // = "/" segment *( "/" segment ) [ "?" *(reserved | unreserved | escaped) ]
        // = "/" *(unreserved | escaped | extra) *( "/" *(unreserved | escaped | extra) ) [ "?" *(reserved | unreserved
        // | escaped) ]

        // `abs-path`と`query_string`(あれば)に分ける
        const light_string abs_path = lct.substr_before("?");
        const light_string query_string
            = abs_path.size() < lct.size() ? lct.substr(abs_path.size() + 1) : HTTP::strfy("");
        QVOUT(abs_path);
        // `abs_path`のvalidityを検証する
        {
            const HTTP::CharFilter unreserved_extra = HTTP::CharFilter::cgi_unreserved | HTTP::CharFilter::cgi_extra;
            std::vector<light_string> segments      = abs_path.split("/");
            if (!HTTP::Validator::is_uri_path(abs_path, unreserved_extra)) {
                return minor_error::make("invalid segment", HTTP::STATUS_INTERNAL_SERVER_ERROR);
            }
            this->abs_path = abs_path;
        }
        QVOUT(query_string);
        // `query_string`のvalidityを検証する
        {
            const HTTP::CharFilter unreserved_reserved
                = HTTP::CharFilter::cgi_unreserved | HTTP::CharFilter::cgi_reserved;
            if (!HTTP::Validator::is_segment(query_string, unreserved_reserved)) {
                return minor_error::make("invalid query_string", HTTP::STATUS_INTERNAL_SERVER_ERROR);
            }
            DXOUT("query_string is valid");
            this->query_string = query_string;
        }

    } else {
        // client-Location?
        // `fragment-URI`
        // = absoluteURI [ "#" fragment ]
        // absolute-URI  = scheme ":" hier-part [ "?" query ]

        // `scheme`の捕捉
        // scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
        light_string tmp;
        HTTP::CharFilter ftr_scheme = HTTP::CharFilter::alpha | HTTP::CharFilter::digit | "+-.";
        const light_string scheme   = lct.substr_while(ftr_scheme);
        if (scheme.size() < 1 || !HTTP::CharFilter::alpha.includes(scheme[0])) {
            QVOUT(scheme);
            return minor_error::make("invalid scheme", HTTP::STATUS_INTERNAL_SERVER_ERROR);
        }
        // `scheme`の直後が`:`であることの確認
        if (scheme.size() >= lct.size() || lct[scheme.size()] != ':') {
            return minor_error::make("invalid scheme separator", HTTP::STATUS_INTERNAL_SERVER_ERROR);
        }
        // `hier-part`の捕捉
        // hier-part   = "//" authority path-abempty
        //     / path-absolute
        //     / path-rootless
        //     / path-empty
        tmp                                 = lct.substr(scheme.size() + 1);
        const light_string bang_fragment    = tmp.substr_from("#");
        const light_string fragment         = bang_fragment.substr(1);
        const light_string without_fragment = tmp.substr_before("#");
        const light_string question_query   = without_fragment.substr_from("?");
        const light_string query            = question_query.substr(1);
        const light_string hier_part        = without_fragment.substr_before("?");
        QVOUT(hier_part);
        QVOUT(query);
        QVOUT(fragment);
        const HTTP::CharFilter unreserved_extra = HTTP::CharFilter::cgi_unreserved | HTTP::CharFilter::cgi_extra;
        if (hier_part.size() >= 2 && hier_part.substr(0, 2) == "//") {
            // hier-part    = "//" authority path-abempty
            // authority    = [ userinfo "@" ] host [ ":" port ]
            // host         = IP-literal / IPv4address / reg-name
            // port         = *DIGIT
            light_string rest = hier_part.substr(2);
            QVOUT(rest);
            const light_string authority = rest.substr_before("/");
            QVOUT(authority);
            if (!HTTP::Validator::is_uri_authority(authority)) {
                return minor_error::make("invalid authority", HTTP::STATUS_INTERNAL_SERVER_ERROR);
            }
            this->authority = authority;
            // validate `path_abempty`
            // path-abempty = *( "/" segment )
            // segment      = *(unreserved | escaped | extra)
            const light_string path_abempty = rest.substr(authority.size());
            QVOUT(path_abempty);
            {
                if (!HTTP::Validator::is_uri_path(path_abempty, unreserved_extra)) {
                    return minor_error::make("invalid path-abempty", HTTP::STATUS_INTERNAL_SERVER_ERROR);
                }
                DXOUT("hier_part is path-abempty");
            }
        } else if (hier_part.size() > 0) {
            // hier-part     = path-absolute | path-rootless
            // path-absolute = "/" [ segment-nz *( "/" segment ) ]
            // segment-nz    = 1*pchar
            // (segment-nz は長さ1以上のsegment)
            // hier-part   = path-rootless
            // path-rootless = segment-nz *( "/" segment )
            light_string tmp = hier_part[0] == '/' ? hier_part.substr(1) : hier_part;
            if (tmp.substr_before("/").size() == 0) { // 実はチェックしなくていい ("//"で始まってないはずなので)
                return minor_error::make("first segment is empty", HTTP::STATUS_INTERNAL_SERVER_ERROR);
            }
            if (!HTTP::Validator::is_uri_path(tmp, unreserved_extra)) {
                return minor_error::make("invalid path-absolute or path-rootless", HTTP::STATUS_INTERNAL_SERVER_ERROR);
            }
            DXOUT("hier_part is path-absolute or path-rootless");
        } else {
            // hier-part   = path-empty
            // path-empty = "" (空文字列)
            DXOUT("hier_part is path-empty");
        }

        const HTTP::CharFilter uric = HTTP::CharFilter::cgi_reserved | HTTP::CharFilter::cgi_unreserved;
        // `query`の捕捉
        {
            //     QUERY_STRING    = query-string
            //     query-string    = *uric
            //     uric            = reserved | unreserved | escaped
            if (!HTTP::Validator::is_segment(query, uric)) {
                return minor_error::make("invalid query", HTTP::STATUS_INTERNAL_SERVER_ERROR);
            }
            DXOUT("query is valid");
            this->query_string = query;
        }

        // `fragment`の捕捉
        {
            // fragment = *(reserved | unreserved | escaped)
            if (!HTTP::Validator::is_segment(fragment, uric)) {
                return minor_error::make("invalid fragment", HTTP::STATUS_INTERNAL_SERVER_ERROR);
            }
            DXOUT("fragment is valid");
            this->fragment = fragment;
        }
    }
    QVOUT(this->value);
    VOUT(this->is_local);
    QVOUT(this->abs_path);
    QVOUT(this->fragment);
    QVOUT(this->query_string);
    return minor_error::ok();
}

// [Connection]

minor_error HTTP::CH::Connection::determine(const AHeaderHolder &holder) {
    // Connection        = 1#connection-option
    // connection-option = token
    const AHeaderHolder::value_list_type *cons = holder.get_vals(HeaderHTTP::connection);
    if (!cons) {
        return minor_error::ok();
    }
    if (cons->size() == 0) {
        DXOUT("[KO] list exists, but it's empty.");
        return minor_error::ok();
    }
    for (AHeaderHolder::value_list_type::const_iterator it = cons->begin(); it != cons->end(); ++it) {
        light_string val_lstr = light_string(*it);
        for (;;) {
            // DXOUT("val_lstr: \"" << val_lstr << "\"");
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            if (val_lstr.size() == 0) {
                DXOUT("away; sp only.");
                break;
            }
            light_string target_lstr = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (target_lstr.size() == 0) {
                DXOUT("away; no value.");
                break;
            }

            // 本体
            byte_string target_str = HTTP::Utils::downcase(target_lstr.str());
            // DXOUT("target_str: \"" << target_str << "\"");
            this->connection_options.push_back(target_str);
            if (target_str == "close") {
                this->close_ = true;
            } else if (target_str == "keep-alive") {
                this->keep_alive_ = true;
            }

            // 後続
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp, target_lstr.size());
            if (val_lstr.size() == 0) {
                // DXOUT("away");
                break;
            }
            // cat(sp_end) が "," か ";" かによって分岐
            // DXOUT("val_lstr[0]: " << val_lstr[0]);
            if (val_lstr[0] == ',') {
                // 次の要素
                val_lstr = val_lstr.substr(1);
            } else {
                // 不正な要素
                DXOUT("[KO] unexpected state: \"" << val_lstr.substr(0) << "\"");
                break;
            }
        }
    }
    DXOUT("close: " << this->will_close() << ", keep-alive: " << this->will_keep_alive());
    return minor_error::ok();
}

bool HTTP::CH::Connection::will_close() const {
    return close_;
}

bool HTTP::CH::Connection::will_keep_alive() const {
    return !close_ && keep_alive_;
}

// [Status]

minor_error CGIP::CH::Status::determine(const AHeaderHolder &holder) {
    // https://datatracker.ietf.org/doc/html/rfc3875#section-6.3.3
    // The Status header field contains a 3-digit integer result code that
    // indicates the level of success of the script's attempt to handle the
    // request.

    //     Status         = "Status:" status-code SP reason-phrase NL
    //     status-code    = "200" | "302" | "400" | "501" | extension-code
    //     extension-code = 3digit
    //     reason-phrase  = *TEXT
    //     TEXT           = <any printable character>

    const byte_string *ct = holder.get_val(HeaderHTTP::status);
    if (!ct || ct->size() == 0) {
        this->code = HTTP::STATUS_UNSPECIFIED;
        return minor_error::ok();
    }
    light_string lct(*ct);
    // `status-code`の捕捉
    const light_string code_ = lct.substr_while(HTTP::CharFilter::digit);
    if (code_.size() != 3) {
        QVOUT(code_);
        return minor_error::make("invalid status code", HTTP::STATUS_INTERNAL_SERVER_ERROR);
    }
    std::stringstream ss;
    ss << code_.str();
    int scode;
    ss >> scode;
    this->code = scode;
    lct        = lct.substr(code_.size());
    VOUT(code_);
    VOUT(this->code);
    // `sp`の捕捉
    // 1つ以上を許容しておく
    const light_string sps = lct.substr_while(HTTP::CharFilter::cgi_sp);
    if (sps.size() == 0) {
        return minor_error::make("not enough spaces", HTTP::STATUS_INTERNAL_SERVER_ERROR);
    }
    lct = lct.substr(sps.size());
    // reason-phraseの捕捉
    lct = lct.rtrim(HTTP::CharFilter::sp);
    if (lct.substr_while(HTTP::CharFilter::printables).size() != lct.size()) {
        QVOUT(lct);
        return minor_error::make("invalid reason-phrase", HTTP::STATUS_INTERNAL_SERVER_ERROR);
    }
    this->reason = lct.str();
    return minor_error::ok();
}
