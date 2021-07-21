#include "Connection.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include "channel.h"
#include "eventloop.h"
#include "util.h"
#include "time.h"
#include <string.h>

using namespace std;
pthread_once_t request_type::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> request_type::file_type;
const int DEFAULT_EXPIRED_TIME = 5000;             
const int DEFAULT_KEEP_ALIVE_TIME = 5000;  //5s
const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;

void request_type::init()
{
 file_type[".html"] = "text/html";
  file_type[".c"] = "text/plain";
 file_type[".doc"] = "application/msword";
  file_type[".gz"] = "application/x-gzip";
 file_type[".htm"] = "text/html";
  file_type[".ico"] = "image/x-icon";
  file_type[".jpg"] = "image/jpeg";
  file_type[".png"] = "image/png";
  file_type[".txt"] = "text/plain";
  file_type["default"] = "text/html";
}

std::string request_type::gettype(const std::string &req_type)
{
     pthread_once(&once_control, request_type::init);
  if(file_type.find(req_type) == file_type.end())
    return file_type["default"];
  else
    return file_type[req_type];
}


 Connection::Connection(eventloop *loop, int connfd):
 loop_(loop),
 fd_(connfd),
 error_(false),
 channel_(new Channel(loop,connfd)),
 connectionState_(H_CONNECTED),
 method_(METHOD_GET),
 HTTPVersion_(HTTP_11),
 state_(STATE_PARSE_URI),
 hState_(H_START),
 keepAlive_(false),
 nowReadPos_(0)
 {

   cout<<"Connection"<<endl;
  channel_->setReadHandler(bind(&Connection::handleRead, this));
  channel_->setWriteHandler(bind(&Connection::handleWrite, this));
  channel_->setConnHandler(bind(&Connection::handleConn, this));
 }


void Connection::reset()
{
 fileName_.clear();
  path_.clear();
  nowReadPos_ = 0;
  state_ = STATE_PARSE_URI;
  hState_ = H_START;
  headers_.clear();

 seperateTimer();
}


void Connection::seperateTimer()
{
  if(timer_.lock())
  {
    shared_ptr <Timer> this_timer(timer_.lock());
    this_timer->clearReq();
    timer_.reset();
  }
}


 void Connection::handleRead() {
  __uint32_t &events_ = channel_->getEvents();
  
  do {
    bool zero = false;
    
    int read_num = readn(fd_, inBuffer_, zero);
    if (connectionState_ == H_DISCONNECTING) {
      inBuffer_.clear();
      break;
    }
    if (read_num < 0) {
      perror("1");
      error_ = true;
      handleError(fd_, 400, "Bad Request");
      break;
    }
    
    else if (zero) {
      connectionState_ = H_DISCONNECTING;
      if (read_num == 0) {
        break;
      }
    }

    if (state_ == STATE_PARSE_URI) {
      URIState flag = this->parseURI();
      if (flag == PARSE_URI_AGAIN)
        break;
      else if (flag == PARSE_URI_ERROR) {
        perror("2");
        inBuffer_.clear();
        error_ = true;
        handleError(fd_, 400, "Bad Request");
        break;
      } else
        state_ = STATE_PARSE_HEADERS;
    }
    if (state_ == STATE_PARSE_HEADERS) {
      HeaderState flag = this->parseHeaders();
      if (flag == PARSE_HEADER_AGAIN)
        break;
      else if (flag == PARSE_HEADER_ERROR) {
        perror("3");
        error_ = true;
        handleError(fd_, 400, "Bad Request");
        break;
      }
      if (method_ == METHOD_POST) {
        state_ = STATE_RECV_BODY;
      } else {
        state_ = STATE_ANALYSIS;
      }
    }
    if (state_ == STATE_RECV_BODY) {
      int content_length = -1;
      if (headers_.find("Content-length") != headers_.end()) {
        content_length = stoi(headers_["Content-length"]);
        cout<<content_length<<endl;
      } else {
        error_ = true;
        handleError(fd_, 400, "Bad Request: Lack of argument (Content-length)");
        break;
      }
      if (static_cast<int>(inBuffer_.size()) < content_length) 
      {
          cout<<inBuffer_.size()<<endl;
        break;
      }
      inBuffer_=inBuffer_.substr(content_length);
      state_ = STATE_ANALYSIS;
    }
    if (state_ == STATE_ANALYSIS) {
      AnalysisState flag = this->analysisRequest();
      if (flag == ANALYSIS_SUCCESS) {
        state_ = STATE_FINISH;
        break;
      } else {
        error_ = true;
        break;
      }
    }
  } while (false);
  cout<<"error:"<<error_<<endl;
  if (!error_) {
    if (outBuffer_.size() > 0) {
      handleWrite();
    }

    if (!error_ && state_ == STATE_FINISH) {
      this->reset();
      if (inBuffer_.size() > 0) {
    
        if (connectionState_ != H_DISCONNECTING) handleRead();
      }
    } else if (!error_ && connectionState_ != H_DISCONNECTED)
      events_ |= EPOLLIN;
  }
}


void Connection::handleWrite() {
  cout<<"begin write"<<endl;
  if (!error_ && connectionState_ != H_DISCONNECTED) {
    __uint32_t &events_ = channel_->getEvents();
    if (writen(fd_, outBuffer_) < 0) {
      perror("writen");
      events_ = 0;
      error_ = true;
    }
    if (outBuffer_.size() > 0) events_ |= EPOLLOUT;
  }
}


void Connection::handleConn() {
  seperateTimer();
  __uint32_t &events_ = channel_->getEvents();
  if (!error_ && connectionState_ == H_CONNECTED) {
    if (events_ != 0) {
      cout<<"not over"<<endl;
      int timeout = DEFAULT_EXPIRED_TIME;
      if (keepAlive_) timeout = DEFAULT_KEEP_ALIVE_TIME;
      if ((events_ & EPOLLIN) && (events_ & EPOLLOUT)) {
        events_ = __uint32_t(0);
        events_ |= EPOLLOUT;
      }
      events_ |= EPOLLET;
      loop_->updatePoller(channel_, timeout);

    } else if (keepAlive_) {
      cout<<"keep alive"<<endl;
      events_ |= (EPOLLIN | EPOLLET);
      int timeout = DEFAULT_KEEP_ALIVE_TIME;
      loop_->updatePoller(channel_, timeout);
    } else {
      cout<<"shutdown"<<endl;
      loop_->shutdown(channel_);
       loop_->runInLoop(bind(&Connection::handleClose, shared_from_this()));
    }
  } else if (!error_ && connectionState_ == H_DISCONNECTING &&
             (events_ & EPOLLOUT)) {
               cout<<"continue write"<<endl;
    events_ = (EPOLLOUT | EPOLLET);
  } else {
    loop_->runInLoop(bind(&Connection::handleClose, shared_from_this()));
  }
}

URIState Connection::parseURI()
{
    string &str = inBuffer_;
  string cop = str;
  size_t pos = str.find('\r', nowReadPos_);
  if (pos < 0) {
    return PARSE_URI_AGAIN;
  }
  string request_line = str.substr(0, pos);
  if (str.size() > pos + 1)
    str = str.substr(pos + 1);
  else
    str.clear();
  int posGet = request_line.find("GET");
  int posPost = request_line.find("POST");
  int posHead = request_line.find("HEAD");

  if (posGet >= 0) {
    pos = posGet;
    method_ = METHOD_GET;
  } else if (posPost >= 0) {
    pos = posPost;
    method_ = METHOD_POST;
  } else if (posHead >= 0) {
    pos = posHead;
    method_ = METHOD_HEAD;
  } else {
    return PARSE_URI_ERROR;
  }

  pos = request_line.find("/", pos);
  if (pos < 0) {
    fileName_ = "index.html";
    HTTPVersion_ = HTTP_11;
    return PARSE_URI_SUCCESS;
  } else {
    size_t _pos = request_line.find(' ', pos);
    if (_pos < 0)
      return PARSE_URI_ERROR;
    else {
      if (_pos - pos > 1) {
        fileName_ = request_line.substr(pos + 1, _pos - pos - 1);
        size_t __pos = fileName_.find('?');
        if (__pos >= 0) {
          fileName_ = fileName_.substr(0, __pos);
        }
      }

      else
        fileName_ = "index.html";
    }
    pos = _pos;
  }

  pos = request_line.find("/", pos);
  if (pos < 0)
    return PARSE_URI_ERROR;
  else {
    if (request_line.size() - pos <= 3)
      return PARSE_URI_ERROR;
    else {
      string ver = request_line.substr(pos + 1, 3);
      if (ver == "1.0")
        HTTPVersion_ = HTTP_10;
      else if (ver == "1.1")
        HTTPVersion_ = HTTP_11;
      else
        return PARSE_URI_ERROR;
    }
  }
  return PARSE_URI_SUCCESS;
}

HeaderState Connection::parseHeaders() {

  string &str = inBuffer_;
  int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
  int now_read_line_begin = 0;
  bool notFinish = true;
  size_t i = 0;
  for (; i < str.size() && notFinish; ++i) {
    switch (hState_) {
      case H_START: {
        if (str[i] == '\n' || str[i] == '\r') break;
        hState_ = H_KEY;
        key_start = i;
        now_read_line_begin = i;
        break;
      }
      case H_KEY: {
        if (str[i] == ':') {
          key_end = i;
          if (key_end - key_start <= 0) return PARSE_HEADER_ERROR;
          hState_ = H_COLON;
        } else if (str[i] == '\n' || str[i] == '\r')
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_COLON: {
        if (str[i] == ' ') {
          hState_ = H_SPACES_AFTER_COLON;
        } else
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_SPACES_AFTER_COLON: {
        hState_ = H_VALUE;
        value_start = i;
        break;
      }
      case H_VALUE: {
        if (str[i] == '\r') {
          hState_ = H_CR;
          value_end = i;
          if (value_end - value_start <= 0) return PARSE_HEADER_ERROR;
        } else if (i - value_start > 255)
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_CR: {
        if (str[i] == '\n') {
          hState_ = H_LF;
          string key(str.begin() + key_start, str.begin() + key_end);
          string value(str.begin() + value_start, str.begin() + value_end);
          headers_[key] = value;
          now_read_line_begin = i;
        } else
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_LF: {
        if (str[i] == '\r') {
          hState_ = H_END_CR;
        } else {
          key_start = i;
          hState_ = H_KEY;
        }
        break;
      }
      case H_END_CR: {
        if (str[i] == '\n') {
          hState_ = H_END_LF;
        } else
          return PARSE_HEADER_ERROR;
        break;
      }
      case H_END_LF: {
        notFinish = false;
        key_start = i;
        now_read_line_begin = i;
        break;
      }
    }
  }
  if (hState_ == H_END_LF) {
    if(str.size()>0)
      str = str.substr(now_read_line_begin);
    return PARSE_HEADER_SUCCESS;
  }
  str = str.substr(now_read_line_begin);
  return PARSE_HEADER_AGAIN;
}


AnalysisState Connection::analysisRequest() {
  if (method_ == METHOD_POST) {
    
     string header;
     header += string("HTTP/1.1 200 OK\r\n");
    if(headers_.find("Connection") != headers_.end() &&
     headers_["Connection"] == "Keep-Alive")
    {
        keepAlive_ = true;
       header += string("Connection: Keep-Alive\r\n") + "Keep-Alive:  timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
  } 
    string body_buff="I have received it!";
    outBuffer_+=header;
      outBuffer_ +="Content-type: text/plain\r\nContent-Length: "+to_string(body_buff.size()) +"\r\nServer: ZJ's Web Server\r\n\r\n"+body_buff;

      return ANALYSIS_SUCCESS;
  }
  else if (method_ == METHOD_GET || method_ == METHOD_HEAD) {
    string header;
    header += "HTTP/1.1 200 OK\r\n";
    if (headers_.find("Connection") != headers_.end() &&
        (headers_["Connection"] == "Keep-Alive" ||
         headers_["Connection"] == "keep-alive")) {
      keepAlive_ = true;
      header += string("Connection: Keep-Alive\r\n") + "Keep-Alive: timeout=" +
                to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
    }
    int dot_pos = fileName_.find('.');
    string filetype;
    if (dot_pos < 0)
      filetype =request_type::gettype("default");
    else
      filetype = request_type::gettype(fileName_.substr(dot_pos));

    // echo test
    if (fileName_ == "hello") {
      outBuffer_+=header;
      string body_buff="Hello World";
      outBuffer_ +=
          "Content-type: text/plain\r\nContent-Length: "+to_string(body_buff.size()) +"\r\nServer: ZJ's Web Server\r\n\r\n"+body_buff;
             
      return ANALYSIS_SUCCESS;
    }
   

    struct stat sbuf;
    if (stat(fileName_.c_str(), &sbuf) < 0) {
      header.clear();
      handleError(fd_, 404, "Not Found!");
      return ANALYSIS_ERROR;
    }
    header += "Content-Type: " + filetype + "\r\n";
    header += "Content-Length: " + to_string(sbuf.st_size) + "\r\n";
    header += "Server: ZJ's Web Server\r\n";
    // 头部结束
    header += "\r\n";
    outBuffer_ += header;

    if (method_ == METHOD_HEAD) return ANALYSIS_SUCCESS;

    int src_fd = open(fileName_.c_str(), O_RDONLY, 0);
    if (src_fd < 0) {
      outBuffer_.clear();
      handleError(fd_, 404, "Not Found!");
      return ANALYSIS_ERROR;
    }
    void *mmapRet = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    close(src_fd);
    if (mmapRet == (void *)-1) {
      munmap(mmapRet, sbuf.st_size);
      outBuffer_.clear();
      handleError(fd_, 404, "Not Found!");
      return ANALYSIS_ERROR;
    }
    char *src_addr = static_cast<char *>(mmapRet);
    outBuffer_ += string(src_addr, src_addr + sbuf.st_size);
    ;
    munmap(mmapRet, sbuf.st_size);
    return ANALYSIS_SUCCESS;
  }
  return ANALYSIS_ERROR;
}

void Connection::newEvent() {
  channel_->setEvents(DEFAULT_EVENT);
  loop_->addToPoller(channel_, DEFAULT_EXPIRED_TIME);
}

void Connection::handleError(int fd, int err_num, string short_msg) {
  short_msg = " " + short_msg;
  char send_buff[4096];
  string body_buff, header_buff;
  body_buff += "<html><title>出错了</title>";
  body_buff += "<body bgcolor=\"ffffff\">";
  body_buff += to_string(err_num) + short_msg;
  body_buff += "<hr><em> ZJ's Web Server</em>\n</body></html>";

  header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
  header_buff += "Content-Type: text/html\r\n";
  header_buff += "Connection: Close\r\n";
  header_buff += "Content-Length: " + to_string(body_buff.size()) + "\r\n";
  header_buff += "Server: ZJ's Web Server\r\n";
  ;
  header_buff += "\r\n";
  sprintf(send_buff, "%s", header_buff.c_str());
  writen(fd, send_buff, strlen(send_buff));
  sprintf(send_buff, "%s", body_buff.c_str());
  writen(fd, send_buff, strlen(send_buff));
}

void Connection::handleClose() {
  //cout<<"close"<<endl;
  connectionState_ = H_DISCONNECTED;
  shared_ptr<Connection> guard(shared_from_this());
  loop_->removeFromPoller(channel_);
}