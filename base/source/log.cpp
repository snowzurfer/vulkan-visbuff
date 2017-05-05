//#include <log.h>
//
//namespace szt {
//
//void FileLogPolicy::OpenOstream(const std::string& name) {
//  out_stream_->open(name.c_str(), std::ios_base::binary|std::ios_base::out);
//  if (!out_stream_->is_open()) {
//    throw(std::runtime_error("LOGGER: Unable to open an output stream"));
//  }
//}
//
//void FileLogPolicy::CloseOstream() {
//  if (out_stream_) {
//    out_stream_->close();
//  }
//}
//
//void FileLogPolicy::Write(const std::string& msg) {
//  (*out_stream_) << msg << std::endl;
//}
//
//FileLogPolicy::~FileLogPolicy() {
//  if (out_stream_) {
//    CloseOstream();
//  }
//}
//
//void STDOutLogPolicy::SetOstream(std::ostream& o_stream) {
//  out_stream_ = &o_stream;
//}
//
//void STDOutLogPolicy::Write(const std::string& msg) {
//  (*out_stream_) << msg << std::endl;
//}
//
//} // namespace szt
