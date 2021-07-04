#ifndef HTMLBODY_HPP
#define HTMLBODY_HPP

#include <string>
#include "FileController.hpp"
#include <sstream>
#include <iomanip>

class HTMLBody {
    public:

        static std::string getBasicHTMLBody(const std::string& statusMsg) {
            std::string serverName = "webserv/0.0.1";
            std::stringstream ret;
            ret << "<html>";
            ret << "<head><title>" << statusMsg << "</title></head>";
            ret << "<body>";
            ret << "<center><h1>" << statusMsg << "</h1></center>";
            ret << "<hr>";
            ret << "<center>" << serverName << "</center>";
            ret << "</body>";
            ret << "</html>";
            return (ret.str());
        }

        static std::string getAutoIndexBody(std::string root, std::string path) {
            FileController folder = FileController(root + path, FileController::READ);
            std::stringstream ret;
            ret << "<html>";
            ret << "<head><title>Index of " << path << "</title></head>";
            ret << "<body>";
            ret << "<h1>Index of " << path << "</h1><hr><pre>" << std::endl;
            for (int i = 0; i < folder.getFilesSize(); ++i) {
                if (folder.getFiles(i)->name == std::string(".")) {
                    continue ;
                }
                std::string fileName;
                if (folder.getFiles(i)->type == FileController::DIRECTORY) {
                    fileName = folder.getFiles(i)->name + std::string("/");
                } else {
                    fileName = folder.getFiles(i)->name;
                }
                ret << "<a href=\"" << fileName << "\">";
                ret << std::setw(53) << std::setfill(' ');
                ret << std::left << (fileName + std::string("</a>"));
                ret << std::right;
                ret << folder.getFiles(i)->generateTime;
                ret << std::setw(20) << std::setfill(' ');
                if (folder.getFiles(i)->type == FileController::DIRECTORY) {
                    ret << "-" << std::endl;
                } else {
                    ret << folder.getFiles(i)->size << std::endl;
                }
            }
            ret << "</pre><hr></body>";
            ret << "</html>";
            return (ret.str());
        }

        static std::string getStaticHTML(int statusCode, std::string root, std::string path) {
            std::string statusMsg;
            std::string ret;
            if (statusCode == 404) {
                statusMsg = "404 Not Found";
                ret = getBasicHTMLBody(statusMsg);
            } else if (statusCode == 403) {
                statusMsg = "403 Forbidden";
                ret = getBasicHTMLBody(statusMsg);
            } else if (statusCode == 200) {
                ret = getAutoIndexBody(root, path);
            }
            return (ret);
        }

};
#endif