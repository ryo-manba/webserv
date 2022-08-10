package error_page

import (
	"fmt"
	"http_test/client"
	"net/http"
	"reflect"
	"strings"
	"testing"
)

func TestErrorPage(t *testing.T) {
	tests := []struct {
		name       string
		request    string
		clientType string
		statusCode int
		body       []byte
	}{
		{

			name:       "Bad_Request",
			request:    "GET /index.html HTTP/1.1\r\n" + validHeader,
			clientType: "default",
			statusCode: http.StatusBadRequest,
			body:       badRequestHtml,
		},
		{

			name:       "Not_Found",
			request:    "GET /not_found_path HTTP/1.1\r\n" + validHeader,
			clientType: "default",
			statusCode: http.StatusNotFound,
			body:       notFoundhtml,
		},
		{
			// todo:allow method でないmethod
			name:       "Method_Not_Allowed",
			request:    "HEAD /index.html HTTP/1.1\r\n" + validHeader,
			clientType: "default",
			statusCode: http.StatusMethodNotAllowed,
			body:       methodNotAllowedhtml,
		},
		{
			// todo: 遅く投げるクライエントを作る
			name:       "Request_Timeout",
			request:    "HEAD /index.html HTTP/1.1\r\n" + validHeader,
			clientType: "slow",
			statusCode: http.StatusRequestTimeout,
			body:       requestTimeouthtml,
		},
		{
			name:       "Length_Required",
			request:    "POST /index.html HTTP/1.1\r\n" + validHeader + strings.Repeat("a", 24),
			clientType: "default",
			statusCode: http.StatusLengthRequired,
			body:       lengthRequiredhtml,
		},
		{
			name:       "Payload_Too_Large",
			request:    "POST /index.html HTTP/1.1\r\n" + "Content-Length: 24\r\n" + validHeader + strings.Repeat("a", 24),
			clientType: "default",
			statusCode: http.StatusRequestEntityTooLarge,
			body:       payloadTooLargehtml,
		},
		{
			name:       "Url_Too_Long",
			request:    fmt.Sprintf("GET /%s HTTP/1.1\r\n", strings.Repeat("a", 10000)) + validHeader,
			clientType: "default",
			statusCode: http.StatusRequestURITooLong,
			body:       urlTooLonghtml,
		},
		{
			// todo:読み込み権限のないファイル
			name:       "Internal_Server_Error",
			request:    "GET /index.html HTTP/1.1\r\n" + validHeader,
			clientType: "default",
			statusCode: http.StatusInternalServerError,
			body:       internalServerErrorhtml,
		},
		{
			name:       "Not_Implemented",
			request:    "get /index.html HTTP/1.1\r\n" + validHeader,
			clientType: "default",
			statusCode: http.StatusNotImplemented,
			body:       notImplementedhtml,
		},
		{
			name:       "HTTP_Version_Not_Supported",
			request:    "GET /index.html HTTP/2.0\r\n" + validHeader,
			clientType: "default",
			statusCode: http.StatusHTTPVersionNotSupported,
			body:       httpVersionNotSupportedhtml,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			c, err := client.NewClient(tt.clientType, tt.request, webservPort)
			if err != nil {
				t.Fatal(err)
			}
			res, err := c.Run()
			if err != nil {
				t.Fatal(err)
			}
			if res.StatusCode != tt.statusCode {
				t.Errorf("unexpected status code got = %d, want %d", res.StatusCode, tt.statusCode)
			}
			if !reflect.DeepEqual(res.Body, tt.body) {
				t.Errorf("unexpected body got = %s, want %s", string(res.Body), string(tt.body))
			}
		})
	}
}
