#ifndef API_H
#define API_H

#include <stdint.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include <iostream>

#ifdef _WIN32
#pragma comment(lib, "opengl32")
#pragma comment(lib, "glu32")
#include <Windows.h>
#endif // _WIN32

#include <GL/gl.h>
#include <GL/glu.h>

// basic datatypes

using u8 = uint8_t;
using i8 = int8_t;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

inline constexpr u8 operator""_u8(const unsigned long long value)
{
  return static_cast<u8>(value);
}
inline constexpr i8 operator""_i8(const unsigned long long value)
{
  return static_cast<i8>(value);
}
inline constexpr u16 operator""_u16(const unsigned long long value)
{
  return static_cast<u16>(value);
}
inline constexpr i16 operator""_i16(const unsigned long long value)
{
  return static_cast<i16>(value);
}
inline constexpr u32 operator""_u32(const unsigned long long value)
{
  return static_cast<u32>(value);
}
inline constexpr i32 operator""_i32(const unsigned long long value)
{
  return static_cast<i32>(value);
}
inline constexpr u64 operator""_u64(const unsigned long long value)
{
  return static_cast<u64>(value);
}
inline constexpr i64 operator""_i64(const unsigned long long value)
{
  return static_cast<i64>(value);
}

inline constexpr f32 operator""_f32(const long double value)
{
  return static_cast<f32>(value);
}
inline constexpr f64 operator""_f64(const long double value)
{
  return static_cast<f64>(value);
}

namespace U8
{
inline constexpr u8 max = UINT8_MAX;
}

namespace I8
{
inline constexpr i8 min = INT8_MIN;
inline constexpr i8 max = INT8_MAX;
}

namespace U16
{
inline constexpr u16 max = UINT16_MAX;
}

namespace I16
{
inline constexpr i16 min = INT16_MIN;
inline constexpr i16 max = INT16_MAX;
}

namespace U32
{
inline constexpr u32 max = UINT32_MAX;
}

namespace I32
{
inline constexpr i32 min = INT32_MIN;
inline constexpr i32 max = INT32_MAX;
}

namespace F32
{
inline constexpr f32 min = FLT_MIN;
inline constexpr f32 max = FLT_MAX;
inline constexpr f32 inf = HUGE_VALF;
inline constexpr f32 nan = NAN;
}

namespace F64
{
inline constexpr f64 min = DBL_MIN;
inline constexpr f64 max = DBL_MAX;
inline constexpr f64 inf = HUGE_VAL;
inline constexpr f64 nan = NAN;
}
// ~basic datatypes

// global constants
inline constexpr f32 PI = 3.14159265358979323846_f32;   // pi
inline constexpr f32 PI_2 = 1.57079632679489661923_f32;   // pi/2
inline constexpr f32 PI_4 = 0.785398163397448309616_f32;  // pi/4
inline constexpr f32 SQRT2 = 1.41421356237309504880_f32;   // sqrt(2)
inline constexpr f32 SQRT1_2 = 0.707106781186547524401_f32;  // 1/sqrt(2)
// ~global constants

// global usings
using Color = u32;
using EntityId = i32;
// ~global usings

// enum as flags
template<typename T>
inline constexpr typename std::underlying_type<T>::type to_underlying(T value) noexcept
{
    return static_cast<typename std::underlying_type<T>::type>(value);
}

#define BIT_FLAGS(T) ;                                                                                                                                                   \
  inline constexpr T operator~(T a) { return static_cast<T>(~static_cast<std::underlying_type<T>::type>(a) ); }                                                          \
  inline constexpr T operator|(T a, T b) { return static_cast<T>(static_cast<std::underlying_type<T>::type>(a) | static_cast<std::underlying_type<T>::type>(b)); }       \
  inline constexpr T operator&(T a, T b) { return static_cast<T>(static_cast<std::underlying_type<T>::type>(a) & static_cast<std::underlying_type<T>::type>(b)); }       \
  inline constexpr T operator^(T a, T b) { return static_cast<T>(static_cast<std::underlying_type<T>::type>(a) ^ static_cast<std::underlying_type<T>::type>(b)); }       \
  inline T& operator|=(T& a, T b) { return reinterpret_cast<T&>(reinterpret_cast<std::underlying_type<T>::type&>(a) |= static_cast<std::underlying_type<T>::type>(b)); } \
  inline T& operator&=(T& a, T b) { return reinterpret_cast<T&>(reinterpret_cast<std::underlying_type<T>::type&>(a) &= static_cast<std::underlying_type<T>::type>(b)); } \
  inline T& operator^=(T& a, T b) { return reinterpret_cast<T&>(reinterpret_cast<std::underlying_type<T>::type&>(a) ^= static_cast<std::underlying_type<T>::type>(b)); }

#define BIT_FLAGS_FRIEND(T) ;                                                                                                                                                   \
  friend inline constexpr T operator~(T a) { return static_cast<T>(~static_cast<std::underlying_type<T>::type>(a) ); }                                                          \
  friend inline constexpr T operator|(T a, T b) { return static_cast<T>(static_cast<std::underlying_type<T>::type>(a) | static_cast<std::underlying_type<T>::type>(b)); }       \
  friend inline constexpr T operator&(T a, T b) { return static_cast<T>(static_cast<std::underlying_type<T>::type>(a) & static_cast<std::underlying_type<T>::type>(b)); }       \
  friend inline constexpr T operator^(T a, T b) { return static_cast<T>(static_cast<std::underlying_type<T>::type>(a) ^ static_cast<std::underlying_type<T>::type>(b)); }       \
  friend inline T& operator|=(T& a, T b) { return reinterpret_cast<T&>(reinterpret_cast<std::underlying_type<T>::type&>(a) |= static_cast<std::underlying_type<T>::type>(b)); } \
  friend inline T& operator&=(T& a, T b) { return reinterpret_cast<T&>(reinterpret_cast<std::underlying_type<T>::type&>(a) &= static_cast<std::underlying_type<T>::type>(b)); } \
  friend inline T& operator^=(T& a, T b) { return reinterpret_cast<T&>(reinterpret_cast<std::underlying_type<T>::type&>(a) ^= static_cast<std::underlying_type<T>::type>(b)); }
// ~enum as flags

#define ASSERT(arg) assert(arg)

#ifdef __GNUC__
[[noreturn]] inline __attribute__((always_inline)) void unreachable() {__builtin_unreachable();}
#elif defined(_MSC_VER) // MSVC
[[noreturn]] __forceinline void unreachable() {__assume(false);}
#else // ???
inline void unreachable() {}
#endif


#define UNUSED(x) (void)x;

//#define ARRAY_SIZE(arr) sizeof(arr)/sizeof(arr[0])
//
//#define INST_DECL(name) struct name##InstanceData* i = nullptr; name()
//#define INST_INIT(name) name::name() : i(new name##InstanceData) {}
//
//#define KEYPRESS_TOGGLE [](bool input)->bool{static bool current;static bool lastInput;if(input&&!lastInput)current=!current;lastInput=input;return current;}
//#define KEYPRESS_ONCE [](bool input)->bool{bool current=false;static bool lastInput;if(input&&!lastInput)current=input;lastInput=input;return current;}
//
//#define FIRST_RUN static bool runNot;!runNot?runNot=true:false // usage: if (FIRST_RUN) {...}
//
//#define DEBUG_BREAK {volatile bool a=true;}
//
//#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
//#define DBG(x) std::cerr << '[' << __FILENAME__ << ':' << __LINE__ << " (" << __func__ << ")] " << #x << " = " << x << '\n';

#endif // API_H
