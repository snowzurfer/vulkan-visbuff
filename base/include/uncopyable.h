#ifndef VKS_UNCOPYABLE
#define VKS_UNCOPYABLE
/**
 * @file uncopyable.h
 * @brief Base class used to disallow copying of other child classes;
 * see Item 39 of Effective C++ 3rd Ed, Meyers
 * @author Alberto Taiuti
 * @version 1.0
 * @date 2016-09-07
 */

namespace szt {

class Uncopyable {
protected:
  Uncopyable() {}
  ~Uncopyable() {}

private:
  Uncopyable(const Uncopyable &);
  Uncopyable &operator=(const Uncopyable &);

}; // class Uncopyable

} // namespace szt

#endif
