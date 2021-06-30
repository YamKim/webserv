#include "ConnectionSocket.hpp"

#include <iostream>


ConnectionSocket::ConnectionSocket(int listeningSocketFd) : Socket(-1) {
    struct sockaddr_in myAddr;
    this->_socket = accept(listeningSocketFd, (struct sockaddr *) &this->_socketAddr, &this->_socketLen);
    if (this->_socket == -1) {
        throw ErrorHandler("Error: connection socket error.", ErrorHandler::ALERT, "ConnectionSocket::ConnectionSocket");
    }
    if (getsockname(this->_socket, (struct sockaddr *) &myAddr, &this->_socketLen) == -1) {
        throw ErrorHandler("Error: getsockname error.", ErrorHandler::ALERT, "ConnectionSocket::ConnectionSocket");
    }
    // _data.setConnectionData(_socketAddr, myAddr);
    _data._clientIP = std::string(inet_ntoa(_socketAddr.sin_addr));
    _data._clientPort = ntohs(_socketAddr.sin_port);
    _data._hostIP = std::string(inet_ntoa(myAddr.sin_addr));
    _data._hostPort = ntohs(myAddr.sin_port);
    _req = new HTTPRequestHandler(_socket);
    _res = NULL;
}

ConnectionSocket::~ConnectionSocket(){
    delete _req;
    delete _res;
}

HTTPRequestHandler::Phase ConnectionSocket::HTTPRequestProcess(void) {
    HTTPRequestHandler::Phase phase;
    try {
        phase = _req->process(_data);
        if (phase == HTTPRequestHandler::FINISH) {
            _res = new HTTPResponseHandler(_socket);
        }
    } catch (const std::exception &error) {
        std::cout << error.what() << std::endl;
        _data._statusCode = 400; // Bad Request
        // TODO: response에서 StatusCode를 인식해서 동작하게 해야 함.
        phase = HTTPRequestHandler::FINISH;
    }
    return (phase);
}

HTTPResponseHandler::Phase ConnectionSocket::HTTPResponseProcess(void) {
    HTTPResponseHandler::Phase phase;
    phase = _res->process(_data);
    return (phase);
}

int ConnectionSocket::runSocket() {
    return (0);
}

int ConnectionSocket::getCGIfd(void) {
    return (_res->getCGIfd());
}