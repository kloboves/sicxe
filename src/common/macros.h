#ifndef COMMON_MACROS_H
#define COMMON_MACROS_H

#include <assert.h>
#include <type_traits>

// Mark unused variables with this macros to prevent compiler warnings.
#define unused(x) ((void)x)

// This macro deletes default move and copy constructors and assignment operators,
// thus preventing unintentional copy and move operations on the class.
#define DISALLOW_COPY_AND_MOVE(ClassName) \
  ClassName(const ClassName&) = delete; \
  ClassName(ClassName&&) = delete; \
  ClassName& operator=(const ClassName&) = delete; \
  ClassName& operator=(ClassName&&) = delete

// This macro deletes default constructors, thus preventing class instantiation.
// Useful for utility classes that only have static functions.
#define DISALLOW_INSTANTIATE(ClassName) \
  ClassName() = delete; \
  ClassName(const ClassName&) = delete; \
  ClassName(ClassName&&) = delete

namespace sicxe {

// isa<T>, cast<T>, dyn_cast<T> template RTTI mechanism adapted from LLVM
// (for more info see: http://llvm.org/docs/HowToSetUpLLVMStyleRTTI.html)

namespace cast_internal {

template<typename To, typename From, typename Enabler = void>
struct IsaImpl {
  static inline bool isa(const From& v) {
    return To::ClassOf(&v);
  }
};

template<typename To, typename From>
struct IsaImpl<
    To, From, typename std::enable_if<std::is_base_of<To, From>::value>::type> {
  static inline bool isa(const From&) {
    return true;
  }
};

template<typename To, typename From>
struct IsaImplWrapper {
  static inline bool isa(const From& v) {
    return IsaImpl<To, From>::isa(v);
  }
};

template<typename To, typename From>
struct IsaImplWrapper<To, From*> {
  static inline bool isa(const From* v) {
    assert(v != nullptr);
    return IsaImpl<To, From>::isa(*v);
  }
};

template<typename To, typename From>
struct IsaImplWrapper<To, From* const> {
  static inline bool isa(const From* v) {
    assert(v != nullptr);
    return IsaImpl<To, From>::isa(*v);
  }
};

template<typename To, typename From>
struct IsaImplWrapper<To, const From*> {
  static inline bool isa(const From* v) {
    assert(v != nullptr);
    return IsaImpl<To, From>::isa(*v);
  }
};

template<typename To, typename From>
struct IsaImplWrapper<To, const From* const> {
  static inline bool isa(const From* v) {
    assert(v != nullptr);
    return IsaImpl<To, From>::isa(*v);
  }
};

}  // namespace cast_internal

template<typename To, typename From>
inline bool isa(const From& v) {
  return cast_internal::IsaImplWrapper<To, const From>::isa(v);
}

namespace cast_internal {

template<typename To, typename From> struct CastReturnType {
  typedef To& Type;
};

template<typename To, typename From> struct CastReturnType<To, const From> {
  typedef const To& Type;
};

template<typename To, typename From> struct CastReturnType<To, From*> {
  typedef To* Type;
};

template<typename To, typename From> struct CastReturnType<To, const From*> {
  typedef const To* Type;
};

template<typename To, typename From> struct CastReturnType<To, const From* const> {
  typedef const To* Type;
};

}  // namespace cast_internal

template<typename To, typename From>
inline typename cast_internal::CastReturnType<To, From>::Type
cast(From& v) {
  assert(isa<To>(v));
  return static_cast<typename cast_internal::CastReturnType<To, From>::Type>(v);
}

template<typename To, typename From>
inline typename cast_internal::CastReturnType<To, From*>::Type
cast(From* v) {
  assert(isa<To>(v));
  return static_cast<typename cast_internal::CastReturnType<To, From*>::Type>(v);
}

template<typename To, typename From>
inline typename cast_internal::CastReturnType<To, From*>::Type
dyn_cast(From* v) {
  return isa<To>(v) ? cast<To>(v) : nullptr;
}

}  // namespace sicxe

#endif  // COMMON_MACROS_H
