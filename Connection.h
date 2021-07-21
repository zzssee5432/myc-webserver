#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include "Timer.h"
#include <iostream>
using namespace std;
class eventloop;
class Timer;
class Channel;

enum ProcessState {
  STATE_PARSE_URI = 1,
  STATE_PARSE_HEADERS,
  STATE_RECV_BODY,
  STATE_ANALYSIS,
  STATE_FINISH
};

enum URIState {
  PARSE_URI_AGAIN = 1,
  PARSE_URI_ERROR,
  PARSE_URI_SUCCESS,
};





enum HeaderState {
  PARSE_HEADER_SUCCESS = 1,
  PARSE_HEADER_AGAIN,
  PARSE_HEADER_ERROR
};

enum AnalysisState { ANALYSIS_SUCCESS = 1, ANALYSIS_ERROR };

enum ParseState {
  H_START = 0,
  H_KEY,
  H_COLON,
  H_SPACES_AFTER_COLON,
  H_VALUE,
  H_CR,
  H_LF,
  H_END_CR,
  H_END_LF
};

enum ConnectionState { H_CONNECTED = 0, H_DISCONNECTING, H_DISCONNECTED };

enum Method { METHOD_POST = 1, METHOD_GET, METHOD_HEAD };

enum Version { HTTP_10 = 1, HTTP_11 };

class request_type {
 private:
  static void init();
  static std::unordered_map<std::string, std::string> file_type;
  request_type();
  request_type(const request_type &m);

 public:
  static std::string gettype(const std::string &req_type);

 private:
  static pthread_once_t once_control;
};

class Connection : public std::enable_shared_from_this<Connection> {
 public:
 Connection(eventloop *loop, int connfd);
  ~Connection() { 
    cout<<"~Connection"<<endl;
    close(fd_); }
  void reset();
  void seperateTimer();
  void linkTimer(std::shared_ptr<Timer> mtimer) {
    timer_ = mtimer;
  }
  std::shared_ptr<Channel> getChannel() { return channel_; }
  eventloop *getLoop() { return loop_; }
  void handleClose();
  void newEvent();

 private:
  eventloop *loop_;
  std::shared_ptr<Channel> channel_;
  int fd_;
  std::string inBuffer_;
  std::string outBuffer_;
  bool error_;
  ConnectionState connectionState_;

  Method method_;
  Version HTTPVersion_;
  std::string fileName_;
  std::string path_;
  int nowReadPos_;
  ProcessState state_;
  ParseState hState_;
  bool keepAlive_;
  std::map<std::string, std::string> headers_;
  std::map<std::string, std::string> args_;
  std::weak_ptr<Timer> timer_;

  void handleRead();
  void handleWrite();
  void handleConn();
  void handleError(int fd, int err_num, std::string short_msg);
  URIState  parseURI();
  HeaderState parseHeaders();
  AnalysisState analysisRequest();
};