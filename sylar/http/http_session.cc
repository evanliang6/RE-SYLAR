#include"http_session.h"
#include"http_parser.h"
namespace sylar {
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    :SocketStream(sock, owner) 
{}
HttpRequest::ptr HttpSession::recvRequest(){
    HttpRequestParser::ptr parser(new HttpRequestParser);
        uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    //uint64_t buff_size = 100;
    std::shared_ptr<char> buffer(
            new char[buff_size], [](char* ptr){
                delete[] ptr;
            });
    char* data = buffer.get();
    int offset = 0;
    do {
        int len = read(data + offset, buff_size - offset);// 从socket流中读取数据到data缓冲区.如果长度小于缓冲区大小,则继续读取
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;// 加上上一次剩的
        size_t nparse = parser->execute(data, len);// nparse表示已经解析的字节数
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparse;// 剩余字节数
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);
    int64_t content_length = parser->getContentLength();
    if (content_length > 0) {
        std::string body;
        body.resize(content_length);

        int copied = std::min((int64_t)offset, content_length);
        memcpy(&body[0], data, copied);

        int64_t remain = content_length - copied;
        if (remain > 0) {
            if (readFixSize(&body[copied], remain) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }
    std::string keep_alive = parser->getData()->getHeader("Connection");
    if(!strcasecmp(keep_alive.c_str(), "keep-alive")) {
        parser->getData()->setClose(false);
    }
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

}


}