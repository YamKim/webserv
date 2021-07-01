#include "HTTPHandler.hpp"

HTTPHandler::HTTPHandler(int connectionFd, NginxConfig::ServerBlock serverConf, const NginxConfig& nginxConf) : _serverConf(serverConf), _nginxConf(nginxConf) {
	_connectionFd = connectionFd;
	_headerString = std::string("");
}

HTTPHandler::~HTTPHandler() {}

std::map<std::string, std::string> HTTPHandler::getHeaders(void) const {
    return (_headers);
}

void HTTPHandler::setGeneralHeader(std::string status) {
    static char timeBuffer[48];
    time_t rawtime;

    std::time(&rawtime);
    struct tm* timeinfo = std::gmtime(&rawtime);
    std::strftime(timeBuffer, 48, "%a, %d %b %Y %H:%M:%S %Z", timeinfo);

    _headerString = status;
    _headerString += "\r\n";

    // NOTE https://developer.mozilla.org/ko/docs/Web/HTTP/Headers
    // 서버의 소프트웨어 정보
    _headers["Server"] = std::string("webserv/0.1");
    // HTTP 메시지가 만들어진 날짜와 시간
    _headers["Date"] = std::string(timeBuffer);
    // 전송이 완료된 후 네트워크 접속을 유지할지 말지
    // REVIEW 상황따라 keep-alive / close가 갈리긴 하지만 현재 구현상으로는 메시지 보내고 끊김. keep-alive 형태로 현재 연결된 fd를 유지시켜야 하나?
    _headers["Connection"] = std::string("close");
}

void HTTPHandler::setTypeHeader(std::string type) {
    _headers["Content-Type"] = type;
}

void HTTPHandler::setLengthHeader(long contentLength) {
    std::stringstream ssLength;

    ssLength << contentLength;
    _headers["Content-Length"] = ssLength.str();
}

void HTTPHandler::convertHeaderMapToString(bool isCGI) {
    std::map<std::string, std::string>::iterator iter;
    for (iter = _headers.begin(); iter != _headers.end(); ++iter) {
        _headerString += iter->first;
        _headerString += ": ";
        _headerString += iter->second;
        _headerString += "\r\n";
    }
    if (isCGI == false) {
        _headerString += "\r\n";
    }
}