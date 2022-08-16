#ifndef ERROR_PAGE_GENERATOR_HPP
#define ERROR_PAGE_GENERATOR_HPP
#include "FileReader.hpp"

class ErrorPageGenerator : public FileReader {
public:
    typedef HTTP::byte_string byte_string;
    typedef HTTP::char_string char_string;
    typedef HTTP::light_string light_string;
    typedef byte_string::size_type size_type;

private:
    minor_error error;

    void generate_html();

public:
    ErrorPageGenerator(const minor_error &error, const RequestMatchingResult &match_result);
    ~ErrorPageGenerator();

    void start_origination(IObserver &observer);
    void leave();
    ResponseHTTP *respond(const RequestHTTP &request);
};

#endif