//#ifndef SZT_LOG
//#define SZT_LOG

//#include <fstream>
//#include <memory>
//
///**
// * @file log.h
// * @brief Logger class taken from
// http://www.drdobbs.com/cpp/a-lightweight-logger-for-c/240147505
// * @author Alberto Taiuti
// * @version 1.0
// * @date 2016-09-16
// */
//
// namespace szt {
//
// class ILogPolicy {
// public:
//  virtual void OpenOstream(const std::string& name) = 0;
//  virtual void SetOstream(std::ostream& o_stream) = 0;
//  virtual void CloseOstream() = 0;
//  virtual void Write(const std::string& msg) = 0;
//};
//
///*
// * Implementation which allows to write into a file
// */
// class FileLogPolicy: public ILogPolicy {
// private:
//  std::unique_ptr< std::ofstream > out_stream_;
//
// public:
//  FileLogPolicy() : out_stream_(new std::ofstream) {}
//  ~FileLogPolicy();
//
//  void OpenOstream(const std::string& name);
//  void SetOstream(std::ostream& o_stream) {};
//  void CloseOstream();
//  void Write(const std::string& msg);
//}; // class FileLogPolicy
//
///*
// * Implementation which allows to write into an already open ostream
// */
// class STDOutLogPolicy: public ILogPolicy {
// private:
//  std::ostream *out_stream_;
//
// public:
//  STDOutLogPolicy() : out_stream_() {}
//
//  void OpenOstream(const std::string& name) {};
//  void SetOstream(std::ostream& o_stream);
//  void CloseOstream() {};
//  void Write(const std::string& msg);
//}; // class STDOutLogPolicy
//
//} // namespace szt
//
//#endif
