#include "HTTPResponseHandler.hpp"

// FIXME : 리스폰스에 URI 외에 더 다양한 아규먼트를 집어넣어야 하는데 어떤 형식으로 집어넣을지 고민 중입니다. 추후에 수정하겠습니다.
HTTPResponseHandler::HTTPResponseHandler(int connectionFd, const NginxConfig::ServerBlock& serverConf, const NginxConfig::NginxConfig& nginxConf) : HTTPHandler(connectionFd, serverConf, nginxConf) {
    _phase = PRE_STATUSCODE_CHECK;
    _file = NULL;
    _cgi = NULL;
}

HTTPResponseHandler::~HTTPResponseHandler() {
    delete _cgi;
    delete _file;
}

std::string HTTPResponseHandler::getIndexPage(const std::string& absPath, const std::vector<std::string>& serverIndexVec, const std::vector<std::string>& locIndexVec) {
    std::vector<std::string>::const_iterator iter;
    std::string serverIndex;
    for (iter = serverIndexVec.begin(); iter != serverIndexVec.end(); ++iter) {
        // std::cout << "[DEBUG] INDEX Page Search[ser]: " << absPath + *iter << std::endl;
        if (FileController::checkType(absPath + *iter) == FileController::FILE) {
            serverIndex = *iter;
            break ;
        }
    }
    std::string locIndex;
    if (!locIndexVec.empty()) {
        for (iter = locIndexVec.begin(); iter != locIndexVec.end(); ++iter) {
            if (FileController::checkType(absPath + *iter) == FileController::FILE) {
                // std::cout << "[DEBUG] INDEX Page Search[loc]: " << absPath + *iter << std::endl;
                locIndex = *iter;
                break ;
            }
        }
        if (iter == locIndexVec.end()) {
            locIndex.clear();
        }
    } else {
        locIndex = serverIndex;
    }
    return locIndex;
}

std::string HTTPResponseHandler::getErrorPage(const std::string& absPath, const std::vector<std::string>& serverErrorPageVec, const std::vector<std::string>& locErrorPageVec) {
    std::string serverErrorPage;
    std::vector<std::string>::const_iterator iter;
    if (!serverErrorPageVec.empty()) {
        iter = serverErrorPageVec.end() - 1;
        // std::cout << "[DEBUG] ERROR Page Search[ser]: " << absPath + *iter << std::endl;
        if (FileController::checkType(absPath + *iter) == FileController::FILE) {
           _errorPageList  = serverErrorPageVec;
            serverErrorPage = *iter;
        } else {
            _errorPageList.clear(); 
            serverErrorPage = "";
        }
    } else {
        _errorPageList.clear(); 
        serverErrorPage = "";
    }

    std::string locErrorPage;
    if (!locErrorPageVec.empty()) {     // loc error page가 있는 경우
        iter = locErrorPageVec.end() - 1;
        // std::cout << "[DEBUG] ERROR Page Search[loc]: " << absPath + *iter << std::endl;
        if (FileController::checkType(absPath + *iter) == FileController::FILE) {
            _errorPageList = locErrorPageVec;
            locErrorPage = *iter;
        } else {
            _errorPageList.clear();
            locErrorPage = "";
        }
    } else {                            // loc error page가 없는 경우
        if (!serverErrorPage.empty()) { // server error page가 있는 경우
            _errorPageList = serverErrorPageVec;
            locErrorPage = serverErrorPage;
        } else {
            _errorPageList.clear();
            locErrorPage = "";
        }
    }
    return locErrorPage;
}

bool HTTPResponseHandler::isErrorPageList(int statusCode, std::vector<std::string>& errorPageVec) {
    std::stringstream ssStatusCode;
    ssStatusCode << statusCode;
    if (find(errorPageVec.begin(), errorPageVec.end(), ssStatusCode.str()) != errorPageVec.end()) {
        return true;
    }
    return false;
}

std::string HTTPResponseHandler::getMIME(const std::string& extension) {
    std::map<std::string, std::string> mime = _nginxConf._http.types.typeMap;
    if(extension == std::string("") || mime.find(extension) == mime.end()) {
        return (_nginxConf._http.dirMap["default_type"]);
    } else {
        return (mime[extension]);
    }
}

void HTTPResponseHandler::setGeneralHeader(int status) {
    std::string startLine;
    if (status == 200) {
        startLine = std::string("HTTP/1.1 200 OK");
    } else if (status == 301) {
        startLine = std::string("HTTP/1.1 302 Found");
    } else if (status == 302) {
        startLine = std::string("HTTP/1.1 301 Moved Permanently");
    } else if (status == 400) {
        startLine = std::string("HTTP/1.1 400 Bad Request");
    } else if (status == 403) {
        startLine = std::string("HTTP/1.1 403 Forbidden");
    } else if (status == 404) {
        startLine = std::string("HTTP/1.1 404 Not Found");
    } else if (status == 413) {
        startLine = std::string("HTTP/1.1 413 Payload Too Large");
    } else if (status == 500) {
        startLine = std::string("HTTP/1.1 500 Internal Server Error");
    } else {
        throw ErrorHandler("Error: invalid HTTP Status Code", ErrorHandler::ALERT, "HTTPResponseHandler::setGeneralHeader");   
    }

    time_t rawtime;
    std::time(&rawtime);
    struct tm* timeinfo = std::gmtime(&rawtime);
    static char timeBuffer[48];
    std::strftime(timeBuffer, 48, "%a, %d %b %Y %H:%M:%S %Z", timeinfo);

    _headerString = startLine;
    _headerString += "\r\n";
    _headers["Connection"] = std::string("close");
    _headers["Date"] = std::string(timeBuffer);
}

void HTTPResponseHandler::setHTMLHeader(const HTTPData& data) {
    std::stringstream ssLength;
    ssLength << data._resContentLength;
    _headers["Server"] = std::string("webserv/") + std::string(WEBSERV_VERSION);
    _headers["Content-Type"] = getMIME(data._URIExtension);
    _headers["Content-Length"] = ssLength.str();
    if (data._statusCode == 301) {
        _headers["Location"] = data._resAbsoluteFilePath;
    }
    convertHeaderMapToString();
}

void HTTPResponseHandler::showResponseInformation(HTTPData &data) {
    std::cout << "Response Information=============================" << std::endl;
    std::cout << "# root + Request URL: " << data._root + data._reqURI << std::endl;
    std::cout << "# Request URL: " << data._reqURI << std::endl;
    std::cout << "# Status Code: " << data._statusCode << std::endl;
    std::cout << "# Absolute File Path: " << data._resAbsoluteFilePath << std::endl;
    std::cout << "# URL Extension: " << data._URIExtension << std::endl;
    std::cout << "# Location Index Page File: " << _indexPage << std::endl;
    std::cout << "# Location Error Page File: " << _errorPage << std::endl;
    std::cout << "# Location Path: " << _locConf._locationPath << std::endl;

}

void HTTPResponseHandler::setCGIConfigMap() {
    for (std::size_t i = 0; i < _serverConf.location.size(); ++i) {
        std::string tmpExt = HTTPData::getExtension(_serverConf.location[i]._locationPath);
        NginxConfig::LocationBlock tmpLocBlock = _serverConf.location[i];
        if (tmpExt.empty()) {
            continue ;
        }
        _cgiConfMap[tmpExt] = tmpLocBlock.dirMap["cgi_pass"];
    }
}

NginxConfig::LocationBlock HTTPResponseHandler::getMatchingLocationConfiguration(const HTTPData& data) {
    NginxConfig::LocationBlock ret;
    bool isLocFlag = false;
    std::size_t matchLen = 0; 
    for (std::size_t i = 0; i < _serverConf.location.size(); ++i) {
        std::string tmpLocPath = _serverConf.location[i]._locationPath;
        std::size_t j = 0; // 1인 경우는 / 일 때이므로
        for (; j < data._URIFilePath.size(); ++j) {
            if (tmpLocPath[j] != data._URIFilePath[j]) {
                break ;
            }
        }
        if (j == 1) { 
            if (tmpLocPath == "/") { // 하나만 일치하는 경우, tmpLocPath가 "/"가 아니면 일치하는 location 블록이 없는 것.
                isLocFlag = true;
                matchLen = j;
                ret = _serverConf.location[i];
            }
        } else {
            if (matchLen < j && (data._URIFilePath.size() >= tmpLocPath.size())) { // 더 많은 글자가 일치하는 location 블록이 있는 경우
                isLocFlag = true;
                matchLen = j;
                ret = _serverConf.location[i];
            }
        }
    }
    return ret;
}

HTTPResponseHandler::Phase HTTPResponseHandler::setInformation(HTTPData& data, int statusCode, const std::string& absPath) {
    data._statusCode = statusCode;
    setGeneralHeader(data._statusCode);
    data._resAbsoluteFilePath = absPath;
    data._URIExtension = "html";
    if (data._statusCode == 200) {
        data._URIExtension = HTTPData::getExtension(data._resAbsoluteFilePath);
        // 인덱스 파일이 cgi 파일인지 판별
        if (_cgiConfMap.find(data._URIExtension) != _cgiConfMap.end()) {
            data._CGIBinary = _cgiConfMap[data._URIExtension];
            return CGI_RUN;
        } else {
            return GET_FILE;
        }
    }
    if (!_errorPage.empty() && isErrorPageList(data._statusCode, _errorPageList)) {
        return setInformation(data, 200, data._resAbsoluteFilePath + _errorPage);
    }
    return GET_STATIC_HTML;
}


HTTPResponseHandler::Phase HTTPResponseHandler::process(HTTPData& data, long bufferSize) {
    // TODO: 해당하는 로케이션의 설정에서 status 코드 적용시키기
    if (_phase == PRE_STATUSCODE_CHECK) {
        if (data._statusCode != 200) {
            setGeneralHeader(data._statusCode);
            data._URIExtension = "html";
            _phase = GET_STATIC_HTML;
        } else {
            _phase = FIND_RESOURCE; 
        }
    }
    
    if (_phase == FIND_RESOURCE) {
        // 1
        data._serverName = _serverConf.dirMap["server_name"];
        data._root = _serverConf.dirMap["root"].empty() ? DEFAULT_ROOT : _serverConf.dirMap["root"];

        // 2
        setCGIConfigMap();

        // 3
        _locConf = getMatchingLocationConfiguration(data);
        // TODO: root에 따라서 변하는 경우, 처리할지 말지 고민: location block 내에도 root가 올 수 있음
        if (!_locConf._locationPath.empty()) { // 4
            // index page 세팅 및 error page 세팅
            std::string tmpAbsPath = _locConf._locationPath == "/" ? "/" : _locConf._locationPath + "/";
            _indexPage = getIndexPage(data._root + tmpAbsPath, _serverConf.index, _locConf.index);
            _errorPage = getErrorPage(data._root + tmpAbsPath, _serverConf.error_page, _locConf.error_page);
            data._root = _locConf.dirMap["root"].empty() ? data._root : _locConf.dirMap["root"];
            _type = FileController::checkType(data._root + data._URIFilePath);
            if (_type == FileController::DIRECTORY) {
                if (data._URIFilePath[data._URIFilePath.size() - 1] != '/') {
                    _phase = setInformation(data, 301, data._URIFilePath + "/");
                } else {
                    if (!_locConf._return.empty()) { // redirection 시키는 것이 가장 우선순위가 높음
                        _phase = setInformation(data, atoi(_locConf._return[0].c_str()), _locConf._return[1]);
                    } else { 
                        if (!_indexPage.empty()) { // index file이 어떻게든 있는 경우
                            // index 파일이 서버 컴퓨터에 있는지 판별
                            FileController::Type indexType = FileController::checkType(data._root + _locConf._locationPath + "/" + _indexPage);
                            if (indexType == FileController::FILE) {
                                _phase = setInformation(data, 200, data._root + _locConf._locationPath + "/" + _indexPage);
                            } else {
                                _phase = setInformation(data, 403, data._root + _locConf._locationPath + "/");
                            }
                        } else {                  // index file이 어디에도 설정되지 않은 경우
                            _locConf.dirMap["autoindex"] = _locConf.dirMap["autoindex"].empty()
                                                        ? _serverConf.dirMap["autoindex"]
                                                        : _locConf.dirMap["autoindex"];
                            if (_locConf.dirMap["autoindex"] == "on") {
                                data._statusCode = 200;
                                setGeneralHeader(data._statusCode);
                                data._URIExtension = "html";
                                _phase = GET_STATIC_HTML;
                            } else {
                                _phase = setInformation(data, 403, data._root + _locConf._locationPath + "/");
                            }
                        }
                    }
                }
            } else if (_type == FileController::FILE) {
                _phase = setInformation(data, 200, data._root + data._URIFilePath);
            } else {
                _phase = setInformation(data, 404, data._root + _locConf._locationPath + "/");
            }
        } else {
            _phase = setInformation(data, 404, data._root + _locConf._locationPath + "/");
        }
        showResponseInformation(data);
    }

    if (_phase == GET_STATIC_HTML) {
        _staticHtml = HTMLBody::getStaticHTML(data);
        data._resContentLength = _staticHtml.length();
        setHTMLHeader(data);
        send(_connectionFd, _headerString.data(), _headerString.length(), 0);
        _phase = DATA_SEND_LOOP;
    } else if (_phase == GET_FILE) {
        _file = new FileController(data._resAbsoluteFilePath, FileController::READ);
        data._resContentLength = _file->length();
        setHTMLHeader(data);
        send(_connectionFd, _headerString.data(), _headerString.length(), 0);
        _phase = DATA_SEND_LOOP;
    }

    if (_phase == DATA_SEND_LOOP) {
        size_t writtenLengthOnBuf;
        size_t buflen = _staticHtml.empty() ? bufferSize : data._resContentLength + 1;
        Buffer buf(buflen);
        if (_staticHtml.empty()) {
            writtenLengthOnBuf = read(_file->getFd(), *buf, buflen);
            _phase = writtenLengthOnBuf == 0 ? FINISH : DATA_SEND_LOOP;
        } else {
            (*buf)[buflen - 1] = '\0';
            writtenLengthOnBuf = strlcpy(*buf, _staticHtml.c_str(), buflen);
            _phase = FINISH;
        }
        size_t writtenLengthOnSocket = send(_connectionFd, *buf, writtenLengthOnBuf, 0);
        if (writtenLengthOnSocket != writtenLengthOnBuf) {
            throw ErrorHandler("Error: send error.", ErrorHandler::ALERT, "HTTPResponseHandler::process");
        }
    } 

    if (_phase == CGI_RUN) {
        // FIXME[yekim]: _cgi를 처음에 절대 경로와 함께 생성하는데, 이를 그대로 사용하면 cgi가 동작하지 않습니다 ㅠㅠ
        delete _cgi;
        _cgi = new CGISession(data);
        _cgi->makeCGIProcess();
        //fcntl(_cgi->getOutputStream(), F_SETFL, O_NONBLOCK);
        _phase = CGI_REQ;
    }
    
    if (_phase == CGI_REQ) {
        if (data._postFilePath.empty()) {
            _phase = CGI_RECV_HEAD_LOOP;
        } else {
            _file = new FileController(data._postFilePath, FileController::READ);
            _phase = CGI_SEND_LOOP;
        }
    }
    
    if (_phase == CGI_SEND_LOOP) {
        Buffer buf(bufferSize);
        int length = read(_file->getFd(), *buf, bufferSize);
        if (length != 0) {
            int writeLength = write(_cgi->getInputStream(), *buf, length);
            if (writeLength != length) {
                throw ErrorHandler("Error: send error.", ErrorHandler::ALERT, "HTTPResponseHandler::process");
            }
        } else {
            _file->del();
            _phase = CGI_RECV_HEAD_LOOP;
        }
    }
    
    if (_phase == CGI_RECV_HEAD_LOOP) {
        bool sendHeader = false;
        Buffer buf(bufferSize);
        int length = read(_cgi->getOutputStream(), *buf, bufferSize);
        std::cout << "length : " << length << std::endl;
        try { // 500 internal error 감지
            // NOTE: stdio는 라인바이라인으로 버퍼가 넘어가는데 여기서 eof(len = 0)가 오면 500 error임.
            if (length <= 0) {
                throw ErrorHandler("Error: CGI HTTP Header Error", ErrorHandler::NORMAL, "HTTPResponseHandler::process");
            } else {
                _CGIReceive += std::string(*buf, length);
                size_t spliter = _CGIReceive.find("\r\n\r\n");
                std::string header;
                if (spliter != std::string::npos) {
                    header = _CGIReceive.substr(0, spliter);
                    _CGIReceive = _CGIReceive.substr(spliter + 4);
                    std::size_t pos = 0;
                    while (header.length() > pos) {
                        _headers.insert(getHTTPHeader(header, pos));
                    }
                    if (_headers.find("Status") == _headers.end()) {
                        data._statusCode = 200;
                        setGeneralHeader(data._statusCode);
                    } else {
                        // TODO[joopark]: setGeneralHeader 중복 세팅에 대한 부분 고려한 후 처리
                        data._statusCode = std::atoi(_headers["Status"].c_str());
                        setGeneralHeader(data._statusCode);
                        _headers.erase("Status");
                    }
                    sendHeader = true;
                }
            }
        } catch (const std::exception& error) {
            std::cerr << error.what() << std::endl;
            data._statusCode = 500;
            setGeneralHeader(data._statusCode);
            data._URIExtension = "html";
            _phase = GET_STATIC_HTML;
        }
        if (sendHeader == true) {
            convertHeaderMapToString();
            size_t writeLength = send(_connectionFd, _headerString.data(), _headerString.length(), 0);
            if (writeLength != _headerString.length()) {
                throw ErrorHandler("Error: send error.", ErrorHandler::ALERT, "HTTPResponseHandler::process");
            }
            _phase = CGI_RECV_BODY_LOOP;
        }
    }
    
    if (_phase == CGI_RECV_BODY_LOOP) {
        Buffer buf(bufferSize);
        if (_CGIReceive.empty()) {
            ssize_t length = read(_cgi->getOutputStream(), *buf, bufferSize);
            if (length == 0) {
                _phase = FINISH;
            } else if (length < 0) {
                throw ErrorHandler("Error: read error.", ErrorHandler::ALERT, "HTTPResponseHandler::process");
            } else {
                ssize_t writeLength = send(_connectionFd, *buf, length, 0);
                if (writeLength != length) {
                    throw ErrorHandler("Error: send error.", ErrorHandler::ALERT, "HTTPResponseHandler::process");
                }
            }
        } else {
            size_t writeLength = send(_connectionFd, _CGIReceive.c_str(), _CGIReceive.length(), 0);
            if (writeLength != _CGIReceive.length()) {
                throw ErrorHandler("Error: send error.", ErrorHandler::ALERT, "HTTPResponseHandler::process");
            } else {
                _CGIReceive.clear();
            }
        }
    }
    return (_phase);
}

int HTTPResponseHandler::getCGIfd(void) {
    return (_cgi->getOutputStream());
}