#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest()
    : start_line(),
      header(),
      body()
{
}

HTTPRequest::~HTTPRequest()
{
}
