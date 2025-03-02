#ifndef HTTP_HEADER_H
#define HTTP_HEADER_H

#include <sstream>
#include <string>
#include <regex>
#include <map>
#include <unordered_map>
#include "logger.h"
enum class HttpStatus {
    OK = 200,
    NOT_FOUND = 404,
    SWITCHING_PROTOCOLS = 101,
};
std::map<HttpStatus, std::string> status_phrase_map = {
    {HttpStatus::OK, "OK"},
    {HttpStatus::NOT_FOUND, "Not Found"},
    {HttpStatus::SWITCHING_PROTOCOLS, "Switching Protocols"}
};

enum class HttpMethod {
    UNKNOWN = -1,
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH,
    TRACE,
    CONNECT,
};

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    HttpStatus status;
    std::string http_version;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpParser {
public:
/*
    static HttpRequest parseHttpRequest(const std::string& request) {
        HttpRequest httpRequest;
        size_t end_header = request.find("\r\n\r\n");
        
        if (end_header != std::string::npos) {
            std::string request_line = request.substr(0, request.find("\r\n"));
            std::string headers_str = request.substr(request.find("\r\n") + 2, end_header - request.find("\r\n") - 2);
            httpRequest.body = request.substr(end_header + 4);
            
            std::regex request_regex("^(GET|POST|PUT|DELETE|HEAD|OPTIONS|PATCH|TRACE|CONNECT) (\\S+) (HTTP/1\\.\\d)");
            std::smatch match;
            if (std::regex_search(request_line, match, request_regex)) {
                httpRequest.method = strToHttpMethod(match[1]);
                httpRequest.path = match[2];
                httpRequest.http_version = match[3];
            }
            
            size_t header_start = 0;
            while (header_start < headers_str.size()) {
                size_t header_end = headers_str.find("\r\n", header_start);
                std::string header;
                if (header_end == std::string::npos) 
                    header = headers_str.substr(header_start);
                else{
                    header = headers_str.substr(header_start, header_end - header_start);
                }
                //std::string header = headers_str.substr(header_start, header_end - header_start);
                size_t colon_pos = header.find(": ");
                if (colon_pos != std::string::npos) {
                    std::cout<<"Header: "<<header.substr(0, colon_pos)<<" Value: "<<header.substr(colon_pos + 2)<<std::endl;
                    httpRequest.headers[header.substr(0, colon_pos)] = header.substr(colon_pos + 2);
                }
                header_start = header_end + 2;
            }
        }
        return httpRequest;
    }
    */
    static HttpRequest parseHttpRequest(const std::string &request)
    {
        HttpRequest httpRequest;
        size_t end_header = request.find("\r\n\r\n");

        if (end_header != std::string::npos)
        {
            std::string request_line = request.substr(0, request.find("\r\n"));
            std::string headers_str = request.substr(request.find("\r\n") + 2, end_header - request.find("\r\n") - 2);
            httpRequest.body = request.substr(end_header + 4);

            std::regex request_regex("^(GET|POST|PUT|DELETE|HEAD|OPTIONS|PATCH|TRACE|CONNECT) (\\S+) (HTTP/1\\.\\d)");
            std::smatch match;
            if (std::regex_search(request_line, match, request_regex))
            {
                httpRequest.method = strToHttpMethod(match[1]); // 示例中简化处理
                httpRequest.path = match[2];
                httpRequest.http_version = match[3];
            }

            size_t header_start = 0;
            while (header_start < headers_str.size())
            {
                size_t header_end = headers_str.find("\r\n", header_start);
                std::string header;
                if (header_end == std::string::npos)
                {
                    header = headers_str.substr(header_start);
                }
                else
                {
                    header = headers_str.substr(header_start, header_end - header_start);
                }
                size_t colon_pos = header.find(": ");
                if (colon_pos != std::string::npos)
                {
                    httpRequest.headers[header.substr(0, colon_pos)] = header.substr(colon_pos + 2);
                }
                if (header_end == std::string::npos)
                {
                    break;
                }
                header_start = header_end + 2;
            }
        }
        return httpRequest;
    }

    static HttpResponse parseHttpResponse(const std::string& response) {
        HttpResponse httpResponse;
        size_t end_header = response.find("\r\n\r\n");
        
        if (end_header != std::string::npos) {
            std::string status_line = response.substr(0, response.find("\r\n"));
            std::string headers_str = response.substr(response.find("\r\n") + 2, end_header - response.find("\r\n") - 2);
            httpResponse.body = response.substr(end_header + 4);
            
            std::regex status_regex("^(HTTP/1\\.\\d) (\\d{3})");
            std::smatch match;
            if (std::regex_search(status_line, match, status_regex)) {
                httpResponse.http_version = match[1];
                httpResponse.status = static_cast<HttpStatus>(std::stoi(match[2]));
            }
            
            size_t header_start = 0;
            while (header_start < headers_str.size()) {
                size_t header_end = headers_str.find("\r\n", header_start);
                if (header_end == std::string::npos) break;
                std::string header = headers_str.substr(header_start, header_end - header_start);
                size_t colon_pos = header.find(": ");
                if (colon_pos != std::string::npos) {
                    
                    httpResponse.headers[header.substr(0, colon_pos)] = header.substr(colon_pos + 2);
                }
                header_start = header_end + 2;
            }
        }
        return httpResponse;
    }
    
    
    //暂未使用
    
    static std::string createHttpRequest(const std::string& method, const std::string& path,  const std::map<std::string, std::string>& headers, const std::string& body) {
        std::ostringstream request_stream;
        request_stream << method << " " << path << " " << "HTTP/1.1" << "\r\n";
        for (const auto& header : headers) {
            request_stream << header.first << ": " << header.second << "\r\n";
        }
        request_stream << "\r\n\r\n" << body;
        return request_stream.str();
    }
    static std::map<std::string, std::string> createDefaultHeaders(const std::string& body) {
        std::map<std::string, std::string> headers;
        headers["Access-Control-Allow-Origin"] = "*";
        headers["Access-Control-Allow-Methods"] = "POST, GET, OPTIONS";
        headers["Access-Control-Allow-Headers"] = "*";
        headers["Connection"]="close";
        if(body.size()>0)
            headers["Content-Length"] = std::to_string(body.length());
        return headers;
    }
    
    static std::string createHttpRequestResponse(HttpStatus status, const std::string& body, const std::map<std::string, std::string>& extraHeaders = {}) {
        std::ostringstream response_stream;
        response_stream << "HTTP/1.1 " << static_cast<int>(status) << " " << status_phrase_map.at(status)<< "\r\n";
    
        std::map<std::string, std::string> headers = createDefaultHeaders(body);

        //添加和覆盖HttpHeaders
        for (const auto& header : extraHeaders) {
            headers[header.first] = header.second;
        }
    
        for (const auto& header : headers) {
            response_stream << header.first << ": " << header.second << "\r\n";
        }
    
        response_stream << "\r\n" << body;
        return response_stream.str();
    }
    static std::string createHttpRequestResponse_1(HttpStatus status,const std::string& Key,const std::string &body)
    {
        std::ostringstream response_stream;
        response_stream << "HTTP/1.1" << " " << static_cast<int>(status) << " " << status_phrase_map.at(status) << "\r\n";
        response_stream<< "Upgrade: websocket"<<"\r\n";
        response_stream<< "Connection: Upgrade"<<"\r\n";
        response_stream<< "Sec-WebSocket-Accept: "<<Key<<"\r\n";
        
        response_stream << "\r\n"<<body;
        //LOG_INFO("createHttpRequestResponse_1:\n"+response_stream.str());
        return response_stream.str();
    }
    /*
    static std::string createHttpResponse(HttpStatus status, const std::string& content_type, const std::string& body) {
        std::ostringstream response_stream;
        response_stream << "HTTP/1.1 " << static_cast<int>(status) << " \r\n";
        response_stream << "Content-Type: " << content_type << "\r\n";
        response_stream << "Access-Control-Allow-Origin: *\r\n"
                           "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                           "Access-Control-Allow-Headers: *\r\n";
                           
        response_stream << "Content-Length: " << body.length() << "\r\n\r\n";
        response_stream << body;
        return response_stream.str();
    }
    */
    static void printHttpRequest(const HttpRequest& request) {
        std::cout << "HTTP Request:\n";
        std::cout << "Method: " << static_cast<int>(request.method) << "\n";
        std::cout << "Path: " << request.path << "\n";
        std::cout << "Version: " << request.http_version << "\n";
        std::cout << "Headers:\n";
        for (const auto& header : request.headers) {
            std::cout << "  " << header.first << ": " << header.second << "\n";
        }
        std::cout << "Body:\n" << request.body << "\n";
    }

    static void printHttpResponse(const HttpResponse& response) {
        std::cout << "HTTP Response:\n";
        std::cout << "Version: " << response.http_version << "\n";
        std::cout << "Status: " << static_cast<int>(response.status) << "\n";
        std::cout << "Headers:\n";
        for (const auto& header : response.headers) {
            std::cout << "  " << header.first << ": " << header.second << "\n";
        }
        std::cout << "Body:\n" << response.body << "\n";
    }
private:
    static HttpMethod strToHttpMethod(const std::string& method) {
        static const std::unordered_map<std::string, HttpMethod> method_map = {
            {"GET", HttpMethod::GET}, {"POST", HttpMethod::POST}, {"PUT", HttpMethod::PUT},
            {"DELETE", HttpMethod::DELETE}, {"HEAD", HttpMethod::HEAD}, {"OPTIONS", HttpMethod::OPTIONS},
            {"PATCH", HttpMethod::PATCH}, {"TRACE", HttpMethod::TRACE}, {"CONNECT", HttpMethod::CONNECT}
        };
        auto it = method_map.find(method);
        return it != method_map.end() ? it->second : HttpMethod::UNKNOWN;
    }
};

#endif // HTTP_HEADER_H
