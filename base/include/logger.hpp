#ifndef SZT_LOGGER
#define SZT_LOGGER

//#include <log.h>
#include <cstdint>
#include <cstring>
#include <eastl_streams.h>
#include <sstream>
#include <string>

namespace szt {

// template <typename LogPolicy>
// class Logger {
// public:
//  Logger(const std::string& name);
//  Logger(std::ostream& o_stream);
//  ~Logger();
//
//  template <SeverityType severity , typename...Args >
//  void Print(Args...args);
//
// private:
//  uint32_t log_line_number_;
//  std::stringstream log_stream_;
//  LogPolicy *policy_;
//  std::mutex write_mutex_;
//
//  std::string GetTime();
//  std::string GetLoglineHeader();
//
//  //Core printing functionality
//  void PrintImpl();
//  template <typename First, typename...Rest>
//  void PrintImpl(First parm1, Rest...parm);
//};
//
//
// template <typename LogPolicy>
// void Logger<LogPolicy>::PrintImpl() {
//  policy_->Write(GetLoglineHeader() + log_stream_.str());
//  log_stream_.str("");
//}
//
// template <typename LogPolicy>
// template <typename First, typename...Rest >
// void Logger<LogPolicy>::PrintImpl(First parm1, Rest...parm) {
//  log_stream_ << parm1;
//  PrintImpl(parm...);
//}
//
// template <typename LogPolicy>
// template <SeverityType severity, typename...Args>
// void Logger<LogPolicy>::Print(Args...args) {
//  write_mutex_.lock();
//  switch(severity) {
//    case SeverityType::DEBUG:
//         log_stream_ << "<DEBUG> :";
//         break;
//    case SeverityType::WARNING:
//         log_stream_ << "<WARNING> :";
//         break;
//    case SeverityType::ERROR:
//         log_stream_ << "<ERROR> :";
//         break;
//  };
//
//  PrintImpl(args...);
//  write_mutex_.unlock();
//}
//
// template <typename LogPolicy>
// std::string Logger<LogPolicy>::GetTime() {
//  std::string time_str;
//  time_t raw_time;
//  time(&raw_time);
//  time_str = ctime(&raw_time);
//  //without the newline character
//  return time_str.substr(0, time_str.size() - 1);
//}
//
// template <typename LogPolicy>
// std::string Logger<LogPolicy>::GetLoglineHeader() {
//  std::stringstream header;
//  header.str("");
//  header.fill('0');
//  header.width(5);
//  header << log_line_number_++ << " < " << GetTime() << " - ";
//  header.fill('0');
//  header.width(5);
//  header << clock() <<" > ~ ";
//  return header.str();
//}
//
// template <typename LogPolicy>
// Logger<LogPolicy>::Logger(const std::string& name)
//    : log_line_number_(0U),
//      log_stream_(),
//      policy_( new LogPolicy()),
//      write_mutex_() {
//  if (!policy_) {
//    throw std::runtime_error("LOGGER: Unable to create the logger instance");
//  }
//
//  policy_->OpenOstream(name);
//}
//
// template <typename LogPolicy>
// Logger<LogPolicy>::Logger(std::ostream& o_stream)
//    : log_line_number_(0U),
//      log_stream_(),
//      policy_( new LogPolicy()),
//      write_mutex_() {
//  if (!policy_) {
//    throw std::runtime_error("LOGGER: Unable to create the logger instance");
//  }
//
//  policy_->SetOstream(o_stream);
//}
//
// template <typename LogPolicy>
// Logger<LogPolicy>::~Logger() {
//  if(policy_) {
//    policy_->CloseOstream();
//    delete policy_;
//    policy_ = nullptr;
//  }
//}
//
//#include <log.h>
//#include <string.h>

#define __FILENAME__                                                           \
  (std::strrchr(__FILE__, '/') ? std::strrchr(__FILE__, '/') + 1 : __FILE__)

// Pre-define logging levels depending on type of build
#ifndef NDEBUG
#define LOGGING_LEVEL_1
#define LOGGING_LEVEL_2
#else
#define LOGGING_LEVEL_2
#endif

// static Logger<FileLogPolicy> file_log_inst("execution.log");
// static Logger<STDOutLogPolicy> stdout_log_inst(std::cout);

//#ifdef LOGGING_LEVEL_1
//#ifdef FILE_LOGGING
//#define LOG_FILE szt::file_log_inst.Print<szt::SeverityType::DEBUG>
//#define LOG_FILE_ERR szt::file_log_inst.Print<szt::SeverityType::ERROR>
//#define LOG_FILE_WARN szt::file_log_inst.Print<szt::SeverityType::WARNING>
//#endif
//#define LOG szt::stdout_log_inst.Print<szt::SeverityType::DEBUG>
//#define LOG_ERR szt::stdout_log_inst.Print<szt::SeverityType::ERROR>
//#define LOG_WARN szt::stdout_log_inst.Print<szt::SeverityType::WARNING>
//#define LOG_FILE(...)
//#define LOG_FILE_ERR(...)
//#define LOG_FILE_WARN(...)
//#else
//#define LOG(...)
//#define LOG_ERR(...)
//#define LOG_WARN(...)
//#define LOG_FILE(...)
//#define LOG_FILE_ERR(...)
//#define LOG_FILE_WARN(...)
//#endif

//#ifdef LOGGING_LEVEL_2
//#ifdef FILE_LOGGING
//#define ELOG_FILE szt::file_log_inst.Print<szt::SeverityType::DEBUG>
//#define ELOG_FILE_ERR szt::file_log_inst.Print<szt::SeverityType::ERROR>
//#define ELOG_FILE_WARN szt::file_log_inst.Print<szt::SeverityType::WARNING>
//#endif
//#define ELOG szt::stdout_log_inst.Print<szt::SeverityType::DEBUG>
//#define ELOG_ERR szt::stdout_log_inst.Print<szt::SeverityType::ERROR>
//#define ELOG_WARN szt::stdout_log_inst.Print<szt::SeverityType::WARNING>
//#define ELOG_FILE(...)
//#define ELOG_FILE_ERR(...)
//#define ELOG_FILE_WARN(...)
//#else
//#define ELOG(...)
//#define ELOG_ERR(...)
//#define ELOG_WARN(...)
//#define ELOG_FILE(...)
//#define ELOG_FILE_ERR(...)
//#define ELOG_FILE_WARN(...)
//#endif

#ifdef LOGGING_LEVEL_1
#ifdef FILE_LOGGING
#define LOG_FILE szt::file_log_inst.Print<szt::SeverityType::DEBUG>
#define LOG_FILE_ERR szt::file_log_inst.Print<szt::SeverityType::ERROR>
#define LOG_FILE_WARN szt::file_log_inst.Print<szt::SeverityType::WARNING>
#endif
#define LOG(msg)                                                               \
  (std::cout << __FILENAME__ << ":" << __LINE__ << " <DEBUG>: " << msg         \
             << std::endl)
#define LOG_ERR(msg)                                                           \
  (std::cout << __FILENAME__ << ":" << __LINE__ << " <ERROR>: " << msg         \
             << std::endl)
#define LOG_WARN(msg)                                                          \
  (std::cout << __FILENAME__ << ":" << __LINE__ << " <WARNING>: " << msg       \
             << std::endl)
#define LOG_FILE(...)
#define LOG_FILE_ERR(...)
#define LOG_FILE_WARN(...)
#else
#define LOG(...)
#define LOG_ERR(...)
#define LOG_WARN(...)
#define LOG_FILE(...)
#define LOG_FILE_ERR(...)
#define LOG_FILE_WARN(...)
#endif

#ifdef LOGGING_LEVEL_2
#ifdef FILE_LOGGING
#define ELOG_FILE szt::file_log_inst.Print<szt::SeverityType::DEBUG>
#define ELOG_FILE_ERR szt::file_log_inst.Print<szt::SeverityType::ERROR>
#define ELOG_FILE_WARN szt::file_log_inst.Print<szt::SeverityType::WARNING>
#endif
#define ELOG(msg)                                                              \
  (std::cout << __FILENAME__ << ":" << __LINE__ << " <DEBUG>: " << msg         \
             << std::endl)
#define ELOG_ERR(msg)                                                          \
  (std::cout << __FILENAME__ << ":" << __LINE__ << " <ERROR>: " << msg         \
             << std::endl)
#define ELOG_WARN(msg)                                                         \
  (std::cout << __FILENAME__ << ":" << __LINE__ << " <WARNING>: " << msg       \
             << std::endl)
#define ELOG_FILE(...)
#define ELOG_FILE_ERR(...)
#define ELOG_FILE_WARN(...)
#else
#define ELOG(...)
#define ELOG_ERR(...)
#define ELOG_WARN(...)
#define ELOG_FILE(...)
#define ELOG_FILE_ERR(...)
#define ELOG_FILE_WARN(...)
#endif

} // namespace szt

#endif
