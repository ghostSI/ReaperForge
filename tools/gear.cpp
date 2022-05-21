
#pragma warning(disable:4996)

#include <assert.h>
#include <filesystem>
#include <float.h>
#include <iostream>
#include <math.h>
#include <regex>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

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


template<typename T>
inline constexpr typename std::underlying_type<T>::type to_underlying(T value) noexcept {
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


namespace Json
{
  enum Type : u64 {
    type_string,
    type_number,
    type_object,
    type_array,
    type_true,
    type_false,
    type_null
  };

  struct value {
    void* payload;
    Type type;
  };

  struct string {
    const char* string;
    u64 string_size;
  };

  struct number {
    const char* number;
    u64 number_size;
  };

  struct object_element {
    struct string* name;
    struct value* value;
    struct object_element* next;
  };

  struct object {
    struct object_element* start;
    u64 length;
  };

  struct array_element {
    struct value* value;
    struct array_element* next;
  };

  struct array {
    struct array_element* start;
    u64 length;
  };

  value* parse(const void* src, u64 src_size);
}

enum struct ParseFlags : u64 {
  default_ = 0,
  allow_trailing_comma = 0x1,
  allow_unquoted_keys = 0x2,
  allow_global_object = 0x4,
  allow_equals_in_object = 0x8,
  allow_no_commas = 0x10,
  allow_c_style_comments = 0x20,
  deprecated = 0x40,
  allow_location_information = 0x80,
  allow_single_quoted_strings = 0x100,
  allow_hexadecimal_numbers = 0x200,
  allow_leading_plus_sign = 0x400,
  allow_leading_or_trailing_decimal_point = 0x800,
  allow_inf_and_nan = 0x1000,
  allow_multi_line_strings = 0x2000,
  allow_simplified_json =
  (allow_trailing_comma |
    allow_unquoted_keys |
    allow_global_object |
    allow_equals_in_object |
    allow_no_commas),
  allow_json5 =
  (allow_trailing_comma |
    allow_unquoted_keys |
    allow_c_style_comments |
    allow_single_quoted_strings |
    allow_hexadecimal_numbers |
    allow_leading_plus_sign |
    allow_leading_or_trailing_decimal_point |
    allow_inf_and_nan |
    allow_multi_line_strings)
}BIT_FLAGS(ParseFlags);

enum struct ParseError {
  none = 0,
  expected_comma_or_closing_bracket,
  expected_colon,
  expected_opening_quote,
  invalid_string_escape_sequence,
  invalid_number_format,
  invalid_value,
  premature_end_of_buffer,
  invalid_string,
  allocator_failed,
  unexpected_trailing_characters,
  unknown
};

struct string_ex {
  struct Json::string string;
  u64 offset;
  u64 line_no;
  u64 row_no;
};

struct value_ex {
  struct Json::value value;
  u64 offset;
  u64 line_no;
  u64 row_no;
};

struct parse_result_s {
  ParseError error;
  u64 error_offset;
  u64 error_line_no;
  u64 error_row_no;
};

struct parse_state_s {
  const char* src;
  u64 size;
  u64 offset;
  ParseFlags flags_bitset;
  char* data;
  char* dom;
  u64 dom_size;
  u64 data_size;
  u64 line_no;
  u64 line_offset;
  ParseError error;
};

struct extract_state_s {
  char* dom;
  char* data;
};

struct extract_result_s {
  u64 dom_size;
  u64 data_size;
};

static i32 get_object_size(struct parse_state_s* state, i32 is_global_object);
static void parse_value(struct parse_state_s* state, i32 is_global_object, struct Json::value* value);
static i32 get_array_size(struct parse_state_s* state);
static extract_result_s extract_get_array_size(const Json::array* const array);
static extract_result_s extract_get_object_size(const Json::object* const object);



static i32 hexadecimal_digit(const char c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  }
  if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  }
  if ('A' <= c && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

static i32 hexadecimal_value(const char* c, const u32 size, u32* result) {
  const char* p;
  i32 digit;

  if (size > sizeof(u32) * 2) {
    return 0;
  }

  *result = 0;
  for (p = c; (u32)(p - c) < size; ++p) {
    *result <<= 4;
    digit = hexadecimal_digit(*p);
    if (digit < 0 || digit > 15) {
      return 0;
    }
    *result |= (u8)digit;
  }
  return 1;
}

static i32 skip_whitespace(parse_state_s* state) {
  u64 offset = state->offset;
  const u64 size = state->size;
  const char* const src = state->src;

  switch (src[offset]) {
  default:
    return 0;
  case ' ':
  case '\r':
  case '\t':
  case '\n':
    break;
  }

  do {
    switch (src[offset]) {
    default:
      state->offset = offset;
      return 1;
    case ' ':
    case '\r':
    case '\t':
      break;
    case '\n':
      state->line_no++;
      state->line_offset = offset;
      break;
    }

    offset++;
  } while (offset < size);

  state->offset = offset;
  return 1;
}

static i32 skip_c_style_comments(parse_state_s* state) {
  if ('/' == state->src[state->offset]) {
    state->offset++;

    if ('/' == state->src[state->offset]) {
      state->offset++;

      while (state->offset < state->size) {
        switch (state->src[state->offset]) {
        default:
          state->offset++;
          break;
        case '\n':
          state->offset++;
          state->line_no++;
          state->line_offset = state->offset;
          return 1;
        }
      }

      return 1;
    }
    else if ('*' == state->src[state->offset]) {
      state->offset++;

      while (state->offset + 1 < state->size) {
        if (('*' == state->src[state->offset]) &&
          ('/' == state->src[state->offset + 1])) {
          state->offset += 2;
          return 1;
        }
        else if ('\n' == state->src[state->offset]) {
          state->line_no++;
          state->line_offset = state->offset;
        }

        state->offset++;
      }

      return 1;
    }
  }

  return 0;
}

static i32 skip_all_skippables(parse_state_s* state) {
  i32 did_consume = 0;
  const u64 size = state->size;

  if (to_underlying(ParseFlags::allow_c_style_comments & state->flags_bitset)) {
    do {
      if (state->offset == size) {
        state->error = ParseError::premature_end_of_buffer;
        return 1;
      }

      did_consume = skip_whitespace(state);

      if (state->offset == size) {
        state->error = ParseError::premature_end_of_buffer;
        return 1;
      }

      did_consume |= skip_c_style_comments(state);
    } while (0 != did_consume);
  }
  else {
    do {
      if (state->offset == size) {
        state->error = ParseError::premature_end_of_buffer;
        return 1;
      }

      did_consume = skip_whitespace(state);
    } while (0 != did_consume);
  }

  if (state->offset == size) {
    state->error = ParseError::premature_end_of_buffer;
    return 1;
  }

  return 0;
}

static i32 get_string_size(parse_state_s* state, u64 is_key) {
  u64 offset = state->offset;
  const u64 size = state->size;
  u64 data_size = 0;
  const char* const src = state->src;
  const i32 is_single_quote = '\'' == src[offset];
  const char quote_to_use = is_single_quote ? '\'' : '"';
  const ParseFlags flags_bitset = state->flags_bitset;
  u32 codepoint;
  u32 high_surrogate = 0;

  if (to_underlying(ParseFlags::allow_location_information & flags_bitset) != 0 &&
    is_key != 0) {
    state->dom_size += sizeof(string_ex);
  }
  else {
    state->dom_size += sizeof(Json::string);
  }

  if ('"' != src[offset]) {
    if (!(to_underlying(ParseFlags::allow_single_quoted_strings & flags_bitset) &&
      is_single_quote)) {
      state->error = ParseError::expected_opening_quote;
      state->offset = offset;
      return 1;
    }
  }

  offset++;

  while ((offset < size) && (quote_to_use != src[offset])) {
    data_size++;

    switch (src[offset]) {
    default:
      break;
    case '\0':
    case '\t':
      state->error = ParseError::invalid_string;
      state->offset = offset;
      return 1;
    }

    if ('\\' == src[offset]) {
      offset++;

      if (offset == size) {
        state->error = ParseError::premature_end_of_buffer;
        state->offset = offset;
        return 1;
      }

      switch (src[offset]) {
      default:
        state->error = ParseError::invalid_string_escape_sequence;
        state->offset = offset;
        return 1;
      case '"':
      case '\\':
      case '/':
      case 'b':
      case 'f':
      case 'n':
      case 'r':
      case 't':
        offset++;
        break;
      case 'u':
        if (!(offset + 5 < size)) {
          state->error = ParseError::invalid_string_escape_sequence;
          state->offset = offset;
          return 1;
        }

        codepoint = 0;
        if (!hexadecimal_value(&src[offset + 1], 4, &codepoint)) {
          state->error = ParseError::invalid_string_escape_sequence;
          state->offset = offset;
          return 1;
        }

        if (high_surrogate != 0) {
          if (codepoint >= 0xdc00 &&
            codepoint <= 0xdfff) {
            data_size += 3;
            high_surrogate = 0;
          }
          else {
            state->error = ParseError::invalid_string_escape_sequence;
            state->offset = offset;
            return 1;
          }
        }
        else if (codepoint <= 0x7f) {
          data_size += 0;
        }
        else if (codepoint <= 0x7ff) {
          data_size += 1;
        }
        else if (codepoint >= 0xd800 &&
          codepoint <= 0xdbff) {
          if (offset + 11 > size || '\\' != src[offset + 5] ||
            'u' != src[offset + 6]) {
            state->error = ParseError::invalid_string_escape_sequence;
            state->offset = offset;
            return 1;
          }
          high_surrogate = codepoint;
        }
        else if (codepoint >= 0xd800 &&
          codepoint <= 0xdfff) {
          state->error = ParseError::invalid_string_escape_sequence;
          state->offset = offset;
          return 1;
        }
        else {
          data_size += 2;
        }
        offset += 5;
        break;
      }
    }
    else if (('\r' == src[offset]) || ('\n' == src[offset])) {
      if (!to_underlying(ParseFlags::allow_multi_line_strings & flags_bitset)) {
        state->error = ParseError::invalid_string_escape_sequence;
        state->offset = offset;
        return 1;
      }

      offset++;
    }
    else {
      offset++;
    }
  }

  if (offset == size) {
    state->error = ParseError::premature_end_of_buffer;
    state->offset = offset - 1;
    return 1;
  }

  offset++;

  state->data_size += data_size;

  state->data_size++;

  state->offset = offset;

  return 0;
}

static i32 is_valid_unquoted_key_char(const char c) {
  return (('0' <= c && c <= '9') || ('a' <= c && c <= 'z') ||
    ('A' <= c && c <= 'Z') || ('_' == c));
}

static i32 get_key_size(parse_state_s* state) {
  const ParseFlags flags_bitset = state->flags_bitset;

  if (to_underlying(ParseFlags::allow_unquoted_keys & flags_bitset)) {
    u64 offset = state->offset;
    const u64 size = state->size;
    const char* const src = state->src;
    u64 data_size = state->data_size;

    if ('"' == src[offset]) {
      return get_string_size(state, 1);
    }
    else if (to_underlying(ParseFlags::allow_single_quoted_strings & flags_bitset) &&
      ('\'' == src[offset])) {
      return get_string_size(state, 1);
    }
    else {
      while ((offset < size) && is_valid_unquoted_key_char(src[offset])) {
        offset++;
        data_size++;
      }

      data_size++;

      if (to_underlying(ParseFlags::allow_location_information & flags_bitset)) {
        state->dom_size += sizeof(string_ex);
      }
      else {
        state->dom_size += sizeof(Json::string);
      }

      state->offset = offset;

      state->data_size = data_size;

      return 0;
    }
  }
  else {
    return get_string_size(state, 1);
  }
}

static i32 get_number_size(parse_state_s* state) {
  const ParseFlags flags_bitset = state->flags_bitset;
  u64 offset = state->offset;
  const u64 size = state->size;
  i32 had_leading_digits = 0;
  const char* const src = state->src;

  state->dom_size += sizeof(Json::number);

  if (to_underlying(ParseFlags::allow_hexadecimal_numbers & flags_bitset) &&
    (offset + 1 < size) && ('0' == src[offset]) &&
    (('x' == src[offset + 1]) || ('X' == src[offset + 1]))) {
    offset += 2;

    while ((offset < size) && (('0' <= src[offset] && src[offset] <= '9') ||
      ('a' <= src[offset] && src[offset] <= 'f') ||
      ('A' <= src[offset] && src[offset] <= 'F'))) {
      offset++;
    }
  }
  else {
    i32 found_sign = 0;
    i32 inf_or_nan = 0;

    if ((offset < size) &&
      (('-' == src[offset]) ||
        (to_underlying(ParseFlags::allow_leading_plus_sign & flags_bitset) &&
          ('+' == src[offset])))) {
      offset++;

      found_sign = 1;
    }

    if (to_underlying(ParseFlags::allow_inf_and_nan & flags_bitset)) {
      const char inf[9] = "Infinity";
      const u64 inf_strlen = sizeof(inf) - 1;
      const char nan[4] = "NaN";
      const u64 nan_strlen = sizeof(nan) - 1;

      if (offset + inf_strlen < size) {
        i32 found = 1;
        u64 i;
        for (i = 0; i < inf_strlen; i++) {
          if (inf[i] != src[offset + i]) {
            found = 0;
            break;
          }
        }

        if (found) {
          offset += inf_strlen;

          inf_or_nan = 1;
        }
      }

      if (offset + nan_strlen < size) {
        i32 found = 1;
        u64 i;
        for (i = 0; i < nan_strlen; i++) {
          if (nan[i] != src[offset + i]) {
            found = 0;
            break;
          }
        }

        if (found) {
          offset += nan_strlen;

          inf_or_nan = 1;
        }
      }
    }

    if (found_sign && !inf_or_nan && (offset < size) &&
      !('0' <= src[offset] && src[offset] <= '9')) {
      if (!to_underlying(ParseFlags::allow_leading_or_trailing_decimal_point &
        flags_bitset) ||
        ('.' != src[offset])) {
        state->error = ParseError::invalid_number_format;
        state->offset = offset;
        return 1;
      }
    }

    if ((offset < size) && ('0' == src[offset])) {
      offset++;

      had_leading_digits = 1;

      if ((offset < size) && ('0' <= src[offset] && src[offset] <= '9')) {
        state->error = ParseError::invalid_number_format;
        state->offset = offset;
        return 1;
      }
    }

    while ((offset < size) && ('0' <= src[offset] && src[offset] <= '9')) {
      offset++;

      had_leading_digits = 1;
    }

    if ((offset < size) && ('.' == src[offset])) {
      offset++;

      if (!('0' <= src[offset] && src[offset] <= '9')) {
        if (!to_underlying(ParseFlags::allow_leading_or_trailing_decimal_point &
          flags_bitset) ||
          !had_leading_digits) {
          state->error = ParseError::invalid_number_format;
          state->offset = offset;
          return 1;
        }
      }

      while ((offset < size) && ('0' <= src[offset] && src[offset] <= '9')) {
        offset++;
      }
    }

    if ((offset < size) && ('e' == src[offset] || 'E' == src[offset])) {
      offset++;

      if ((offset < size) && ('-' == src[offset] || '+' == src[offset])) {
        offset++;
      }

      if ((offset < size) && !('0' <= src[offset] && src[offset] <= '9')) {
        state->error = ParseError::invalid_number_format;
        state->offset = offset;
        return 1;
      }

      do {
        offset++;
      } while ((offset < size) && ('0' <= src[offset] && src[offset] <= '9'));
    }
  }

  if (offset < size) {
    switch (src[offset]) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
    case '}':
    case ',':
    case ']':
      break;
    case '=':
      if (to_underlying(ParseFlags::allow_equals_in_object & flags_bitset)) {
        break;
      }

      state->error = ParseError::invalid_number_format;
      state->offset = offset;
      return 1;
    default:
      state->error = ParseError::invalid_number_format;
      state->offset = offset;
      return 1;
    }
  }

  state->data_size += offset - state->offset;

  state->data_size++;

  state->offset = offset;

  return 0;
}

static i32 get_value_size(parse_state_s* state, i32 is_global_object) {
  const ParseFlags flags_bitset = state->flags_bitset;
  const char* const src = state->src;
  u64 offset;
  const u64 size = state->size;

  if (to_underlying(ParseFlags::allow_location_information & flags_bitset)) {
    state->dom_size += sizeof(value_ex);
  }
  else {
    state->dom_size += sizeof(Json::value);
  }

  if (is_global_object) {
    return get_object_size(state, 1);
  }
  else {
    if (skip_all_skippables(state)) {
      state->error = ParseError::premature_end_of_buffer;
      return 1;
    }

    offset = state->offset;

    switch (src[offset]) {
    case '"':
      return get_string_size(state, 0);
    case '\'':
      if (to_underlying(ParseFlags::allow_single_quoted_strings & flags_bitset)) {
        return get_string_size(state, 0);
      }
      else {
        state->error = ParseError::invalid_value;
        return 1;
      }
    case '{':
      return get_object_size(state, 0);
    case '[':
      return get_array_size(state);
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return get_number_size(state);
    case '+':
      if (to_underlying(ParseFlags::allow_leading_plus_sign & flags_bitset)) {
        return get_number_size(state);
      }
      else {
        state->error = ParseError::invalid_number_format;
        return 1;
      }
    case '.':
      if (to_underlying(ParseFlags::allow_leading_or_trailing_decimal_point &
        flags_bitset)) {
        return get_number_size(state);
      }
      else {
        state->error = ParseError::invalid_number_format;
        return 1;
      }
    default:
      if ((offset + 4) <= size && 't' == src[offset + 0] &&
        'r' == src[offset + 1] && 'u' == src[offset + 2] &&
        'e' == src[offset + 3]) {
        state->offset += 4;
        return 0;
      }
      else if ((offset + 5) <= size && 'f' == src[offset + 0] &&
        'a' == src[offset + 1] && 'l' == src[offset + 2] &&
        's' == src[offset + 3] && 'e' == src[offset + 4]) {
        state->offset += 5;
        return 0;
      }
      else if ((offset + 4) <= size && 'n' == state->src[offset + 0] &&
        'u' == state->src[offset + 1] &&
        'l' == state->src[offset + 2] &&
        'l' == state->src[offset + 3]) {
        state->offset += 4;
        return 0;
      }
      else if (to_underlying(ParseFlags::allow_inf_and_nan & flags_bitset) &&
        (offset + 3) <= size && 'N' == src[offset + 0] &&
        'a' == src[offset + 1] && 'N' == src[offset + 2]) {
        return get_number_size(state);
      }
      else if (to_underlying(ParseFlags::allow_inf_and_nan & flags_bitset) &&
        (offset + 8) <= size && 'I' == src[offset + 0] &&
        'n' == src[offset + 1] && 'f' == src[offset + 2] &&
        'i' == src[offset + 3] && 'n' == src[offset + 4] &&
        'i' == src[offset + 5] && 't' == src[offset + 6] &&
        'y' == src[offset + 7]) {
        return get_number_size(state);
      }

      state->error = ParseError::invalid_value;
      return 1;
    }
  }
}

static i32 get_object_size(parse_state_s* state, i32 is_global_object) {
  const ParseFlags flags_bitset = state->flags_bitset;
  const char* const src = state->src;
  const u64 size = state->size;
  u64 elements = 0;
  i32 allow_comma = 0;
  i32 found_closing_brace = 0;

  if (is_global_object) {
    if (!skip_all_skippables(state) && '{' == state->src[state->offset]) {
      is_global_object = 0;
    }
  }

  if (!is_global_object) {
    if ('{' != src[state->offset]) {
      state->error = ParseError::unknown;
      return 1;
    }

    state->offset++;
  }

  state->dom_size += sizeof(Json::object);

  if ((state->offset == size) && !is_global_object) {
    state->error = ParseError::premature_end_of_buffer;
    return 1;
  }

  do {
    if (!is_global_object) {
      if (skip_all_skippables(state)) {
        state->error = ParseError::premature_end_of_buffer;
        return 1;
      }

      if ('}' == src[state->offset]) {
        state->offset++;

        found_closing_brace = 1;
        break;
      }
    }
    else {
      if (skip_all_skippables(state)) {
        break;
      }
    }

    if (allow_comma) {
      if (',' == src[state->offset]) {
        state->offset++;
        allow_comma = 0;
      }
      else if (to_underlying(ParseFlags::allow_no_commas & flags_bitset)) {
        allow_comma = 0;
      }
      else {
        state->error = ParseError::expected_comma_or_closing_bracket;
        return 1;
      }

      if (to_underlying(ParseFlags::allow_trailing_comma & flags_bitset)) {
        continue;
      }
      else {
        if (skip_all_skippables(state)) {
          state->error = ParseError::premature_end_of_buffer;
          return 1;
        }
      }
    }

    if (get_key_size(state)) {
      state->error = ParseError::invalid_string;
      return 1;
    }

    if (skip_all_skippables(state)) {
      state->error = ParseError::premature_end_of_buffer;
      return 1;
    }

    if (to_underlying(ParseFlags::allow_equals_in_object & flags_bitset)) {
      const char current = src[state->offset];
      if ((':' != current) && ('=' != current)) {
        state->error = ParseError::expected_colon;
        return 1;
      }
    }
    else {
      if (':' != src[state->offset]) {
        state->error = ParseError::expected_colon;
        return 1;
      }
    }

    state->offset++;

    if (skip_all_skippables(state)) {
      state->error = ParseError::premature_end_of_buffer;
      return 1;
    }

    if (get_value_size(state, 0)) {
      return 1;
    }

    elements++;
    allow_comma = 1;
  } while (state->offset < size);

  if ((state->offset == size) && !is_global_object && !found_closing_brace) {
    state->error = ParseError::premature_end_of_buffer;
    return 1;
  }

  state->dom_size += sizeof(Json::object_element) * elements;

  return 0;
}

static i32 get_array_size(parse_state_s* state) {
  const ParseFlags flags_bitset = state->flags_bitset;
  u64 elements = 0;
  i32 allow_comma = 0;
  const char* const src = state->src;
  const u64 size = state->size;

  if ('[' != src[state->offset]) {
    state->error = ParseError::unknown;
    return 1;
  }

  state->offset++;

  state->dom_size += sizeof(Json::array);

  while (state->offset < size) {
    if (skip_all_skippables(state)) {
      state->error = ParseError::premature_end_of_buffer;
      return 1;
    }

    if (']' == src[state->offset]) {
      state->offset++;

      state->dom_size += sizeof(Json::array_element) * elements;

      return 0;
    }

    if (allow_comma) {
      if (',' == src[state->offset]) {
        state->offset++;
        allow_comma = 0;
      }
      else if (!to_underlying(ParseFlags::allow_no_commas & flags_bitset)) {
        state->error = ParseError::expected_comma_or_closing_bracket;
        return 1;
      }

      if (to_underlying(ParseFlags::allow_trailing_comma & flags_bitset)) {
        allow_comma = 0;
        continue;
      }
      else {
        if (skip_all_skippables(state)) {
          state->error = ParseError::premature_end_of_buffer;
          return 1;
        }
      }
    }

    if (get_value_size(state, 0)) {
      return 1;
    }

    elements++;
    allow_comma = 1;
  }

  state->error = ParseError::premature_end_of_buffer;
  return 1;
}

static void parse_string(parse_state_s* state, Json::string* string) {
  u64 offset = state->offset;
  u64 bytes_written = 0;
  const char* const src = state->src;
  const char quote_to_use = '\'' == src[offset] ? '\'' : '"';
  char* data = state->data;
  u32 high_surrogate = 0;
  u32 codepoint;

  string->string = data;

  offset++;

  while (quote_to_use != src[offset]) {
    if ('\\' == src[offset]) {
      offset++;

      switch (src[offset++]) {
      default:
        return;
      case 'u': {
        codepoint = 0;
        if (!hexadecimal_value(&src[offset], 4, &codepoint)) {
          return;
        }

        offset += 4;

        if (codepoint <= 0x7fu) {
          data[bytes_written++] = (char)codepoint;
        }
        else if (codepoint <= 0x7ffu) {
          data[bytes_written++] =
            (char)(0xc0u | (codepoint >> 6));
          data[bytes_written++] =
            (char)(0x80u | (codepoint & 0x3fu));
        }
        else if (codepoint >= 0xd800 &&
          codepoint <= 0xdbff) {
          high_surrogate = codepoint;
          continue;
        }
        else if (codepoint >= 0xdc00 &&
          codepoint <= 0xdfff) {
          const u32 surrogate_offset =
            0x10000u - (0xD800u << 10) - 0xDC00u;
          codepoint = (high_surrogate << 10) + codepoint + surrogate_offset;
          high_surrogate = 0;
          data[bytes_written++] =
            (char)(0xF0u | (codepoint >> 18));
          data[bytes_written++] =
            (char)(0x80u | ((codepoint >> 12) & 0x3fu));
          data[bytes_written++] =
            (char)(0x80u | ((codepoint >> 6) & 0x3fu));
          data[bytes_written++] =
            (char)(0x80u | (codepoint & 0x3fu));
        }
        else {
          data[bytes_written++] =
            (char)(0xe0u | (codepoint >> 12));
          data[bytes_written++] =
            (char)(0x80u | ((codepoint >> 6) & 0x3fu));
          data[bytes_written++] =
            (char)(0x80u | (codepoint & 0x3fu));
        }
      } break;
      case '"':
        data[bytes_written++] = '"';
        break;
      case '\\':
        data[bytes_written++] = '\\';
        break;
      case '/':
        data[bytes_written++] = '/';
        break;
      case 'b':
        data[bytes_written++] = '\b';
        break;
      case 'f':
        data[bytes_written++] = '\f';
        break;
      case 'n':
        data[bytes_written++] = '\n';
        break;
      case 'r':
        data[bytes_written++] = '\r';
        break;
      case 't':
        data[bytes_written++] = '\t';
        break;
      case '\r':
        data[bytes_written++] = '\r';

        if ('\n' == src[offset]) {
          data[bytes_written++] = '\n';
          offset++;
        }

        break;
      case '\n':
        data[bytes_written++] = '\n';
        break;
      }
    }
    else {
      data[bytes_written++] = src[offset++];
    }
  }

  offset++;

  string->string_size = bytes_written;

  data[bytes_written++] = '\0';

  state->data += bytes_written;

  state->offset = offset;
}

static void parse_key(parse_state_s* state, Json::string* string) {
  if (to_underlying(ParseFlags::allow_unquoted_keys & state->flags_bitset)) {
    const char* const src = state->src;
    char* const data = state->data;
    u64 offset = state->offset;

    if (('"' == src[offset]) || ('\'' == src[offset])) {
      parse_string(state, string);
    }
    else {
      u64 size = 0;

      string->string = state->data;

      while (is_valid_unquoted_key_char(src[offset])) {
        data[size++] = src[offset++];
      }

      data[size] = '\0';
      string->string_size = size++;
      state->data += size;
      state->offset = offset;
    }
  }
  else {
    parse_string(state, string);
  }
}

static void parse_object(parse_state_s* state, i32 is_global_object, Json::object* object) {
  const ParseFlags flags_bitset = state->flags_bitset;
  const u64 size = state->size;
  const char* const src = state->src;
  u64 elements = 0;
  i32 allow_comma = 0;
  Json::object_element* previous = nullptr;

  if (is_global_object) {
    if ('{' == src[state->offset]) {
      is_global_object = 0;
    }
  }

  if (!is_global_object) {
    state->offset++;
  }

  (void)skip_all_skippables(state);

  elements = 0;

  while (state->offset < size) {
    Json::object_element* element = nullptr;
    Json::string* string = nullptr;
    Json::value* value = nullptr;

    if (!is_global_object) {
      (void)skip_all_skippables(state);

      if ('}' == src[state->offset]) {
        state->offset++;
        break;
      }
    }
    else {
      if (skip_all_skippables(state)) {
        break;
      }
    }

    if (allow_comma) {
      if (',' == src[state->offset]) {
        state->offset++;
        allow_comma = 0;
        continue;
      }
    }

    element = (Json::object_element*)state->dom;

    state->dom += sizeof(Json::object_element);

    if (nullptr == previous) {
      object->start = element;
    }
    else {
      previous->next = element;
    }

    previous = element;

    if (to_underlying(ParseFlags::allow_location_information & flags_bitset)) {
      string_ex* string_ex_ =
        (string_ex*)state->dom;
      state->dom += sizeof(string_ex);

      string_ex_->offset = state->offset;
      string_ex_->line_no = state->line_no;
      string_ex_->row_no = state->offset - state->line_offset;

      string = &(string_ex_->string);
    }
    else {
      string = (Json::string*)state->dom;
      state->dom += sizeof(Json::string);
    }

    element->name = string;

    (void)parse_key(state, string);

    (void)skip_all_skippables(state);

    state->offset++;

    (void)skip_all_skippables(state);

    if (to_underlying(ParseFlags::allow_location_information & flags_bitset)) {
      value_ex* value_ex_ = (value_ex*)state->dom;
      state->dom += sizeof(value_ex_);

      value_ex_->offset = state->offset;
      value_ex_->line_no = state->line_no;
      value_ex_->row_no = state->offset - state->line_offset;

      value = &(value_ex_->value);
    }
    else {
      value = (Json::value*)state->dom;
      state->dom += sizeof(Json::value);
    }

    element->value = value;

    parse_value(state, 0, value);

    elements++;
    allow_comma = 1;
  }

  if (previous) {
    previous->next = nullptr;
  }

  if (0 == elements) {
    object->start = nullptr;
  }

  object->length = elements;
}

static void parse_array(parse_state_s* state, Json::array* array) {
  const char* const src = state->src;
  const u64 size = state->size;
  u64 elements = 0;
  i32 allow_comma = 0;
  Json::array_element* previous = nullptr;

  state->offset++;

  (void)skip_all_skippables(state);

  elements = 0;

  do {
    Json::array_element* element = nullptr;
    Json::value* value = nullptr;

    (void)skip_all_skippables(state);

    if (']' == src[state->offset]) {
      state->offset++;
      break;
    }

    if (allow_comma) {
      if (',' == src[state->offset]) {
        /* skip comma. */
        state->offset++;
        allow_comma = 0;
        continue;
      }
    }

    element = (Json::array_element*)state->dom;

    state->dom += sizeof(Json::array_element);

    if (nullptr == previous) {
      array->start = element;
    }
    else {
      previous->next = element;
    }

    previous = element;

    if (to_underlying(ParseFlags::allow_location_information & state->flags_bitset)) {
      value_ex* value_ex_ = (value_ex*)state->dom;
      state->dom += sizeof(value_ex);

      value_ex_->offset = state->offset;
      value_ex_->line_no = state->line_no;
      value_ex_->row_no = state->offset - state->line_offset;

      value = &(value_ex_->value);
    }
    else {
      value = (Json::value*)state->dom;
      state->dom += sizeof(Json::value);
    }

    element->value = value;

    parse_value(state, 0, value);

    elements++;
    allow_comma = 1;
  } while (state->offset < size);

  if (previous) {
    previous->next = nullptr;
  }

  if (0 == elements) {
    array->start = nullptr;
  }

  array->length = elements;
}

static void parse_number(parse_state_s* state, Json::number* number) {
  const ParseFlags flags_bitset = state->flags_bitset;
  u64 offset = state->offset;
  const u64 size = state->size;
  u64 bytes_written = 0;
  const char* const src = state->src;
  char* data = state->data;

  number->number = data;

  if (to_underlying(ParseFlags::allow_hexadecimal_numbers & flags_bitset)) {
    if (('0' == src[offset]) &&
      (('x' == src[offset + 1]) || ('X' == src[offset + 1]))) {
      while ((offset < size) &&
        (('0' <= src[offset] && src[offset] <= '9') ||
          ('a' <= src[offset] && src[offset] <= 'f') ||
          ('A' <= src[offset] && src[offset] <= 'F') ||
          ('x' == src[offset]) || ('X' == src[offset]))) {
        data[bytes_written++] = src[offset++];
      }
    }
  }

  while (offset < size) {
    i32 end = 0;

    switch (src[offset]) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '.':
    case 'e':
    case 'E':
    case '+':
    case '-':
      data[bytes_written++] = src[offset++];
      break;
    default:
      end = 1;
      break;
    }

    if (0 != end) {
      break;
    }
  }

  if (to_underlying(ParseFlags::allow_inf_and_nan & flags_bitset)) {
    const u64 inf_strlen = 8;
    const u64 nan_strlen = 3;

    if (offset + inf_strlen < size) {
      if ('I' == src[offset]) {
        u64 i;
        for (i = 0; i < inf_strlen; i++) {
          data[bytes_written++] = src[offset++];
        }
      }
    }

    if (offset + nan_strlen < size) {
      if ('N' == src[offset]) {
        u64 i;
        for (i = 0; i < nan_strlen; i++) {
          data[bytes_written++] = src[offset++];
        }
      }
    }
  }

  number->number_size = bytes_written;
  data[bytes_written++] = '\0';
  state->data += bytes_written;
  state->offset = offset;
}

static void parse_value(parse_state_s* state, i32 is_global_object, Json::value* value) {
  const ParseFlags flags_bitset = state->flags_bitset;
  const char* const src = state->src;
  const u64 size = state->size;
  u64 offset;

  (void)skip_all_skippables(state);

  offset = state->offset;

  if (is_global_object) {
    value->type = Json::type_object;
    value->payload = state->dom;
    state->dom += sizeof(Json::object);
    parse_object(state, 1,
      (Json::object*)value->payload);
  }
  else {
    switch (src[offset]) {
    case '"':
    case '\'':
      value->type = Json::type_string;
      value->payload = state->dom;
      state->dom += sizeof(Json::string);
      parse_string(state, (Json::string*)value->payload);
      break;
    case '{':
      value->type = Json::type_object;
      value->payload = state->dom;
      state->dom += sizeof(Json::object);
      parse_object(state, 0,
        (Json::object*)value->payload);
      break;
    case '[':
      value->type = Json::type_array;
      value->payload = state->dom;
      state->dom += sizeof(Json::array);
      parse_array(state, (Json::array*)value->payload);
      break;
    case '-':
    case '+':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '.':
      value->type = Json::type_number;
      value->payload = state->dom;
      state->dom += sizeof(Json::number);
      parse_number(state, (Json::number*)value->payload);
      break;
    default:
      if ((offset + 4) <= size && 't' == src[offset + 0] &&
        'r' == src[offset + 1] && 'u' == src[offset + 2] &&
        'e' == src[offset + 3]) {
        value->type = Json::type_true;
        value->payload = nullptr;
        state->offset += 4;
      }
      else if ((offset + 5) <= size && 'f' == src[offset + 0] &&
        'a' == src[offset + 1] && 'l' == src[offset + 2] &&
        's' == src[offset + 3] && 'e' == src[offset + 4]) {
        value->type = Json::type_false;
        value->payload = nullptr;
        state->offset += 5;
      }
      else if ((offset + 4) <= size && 'n' == src[offset + 0] &&
        'u' == src[offset + 1] && 'l' == src[offset + 2] &&
        'l' == src[offset + 3]) {
        value->type = Json::type_null;
        value->payload = nullptr;
        state->offset += 4;
      }
      else if (to_underlying(ParseFlags::allow_inf_and_nan & flags_bitset) &&
        (offset + 3) <= size && 'N' == src[offset + 0] &&
        'a' == src[offset + 1] && 'N' == src[offset + 2]) {
        value->type = Json::type_number;
        value->payload = state->dom;
        state->dom += sizeof(Json::number);
        parse_number(state, (Json::number*)value->payload);
      }
      else if (to_underlying(ParseFlags::allow_inf_and_nan & flags_bitset) &&
        (offset + 8) <= size && 'I' == src[offset + 0] &&
        'n' == src[offset + 1] && 'f' == src[offset + 2] &&
        'i' == src[offset + 3] && 'n' == src[offset + 4] &&
        'i' == src[offset + 5] && 't' == src[offset + 6] &&
        'y' == src[offset + 7]) {
        value->type = Json::type_number;
        value->payload = state->dom;
        state->dom += sizeof(Json::number);
        parse_number(state, (Json::number*)value->payload);
      }
      break;
    }
  }
}

static extract_result_s extract_get_number_size(const Json::number* const number) {
  extract_result_s result;
  result.dom_size = sizeof(Json::number);
  result.data_size = number->number_size;
  return result;
}

static extract_result_s extract_get_string_size(const Json::string* const string) {
  extract_result_s result;
  result.dom_size = sizeof(Json::string);
  result.data_size = string->string_size + 1;
  return result;
}

static extract_result_s extract_get_value_size(const Json::value* const value) {
  extract_result_s result = { 0, 0 };

  switch (value->type) {
  default:
    break;
  case Json::type_object:
    result = extract_get_object_size(
      (const Json::object*)value->payload);
    break;
  case Json::type_array:
    result = extract_get_array_size(
      (const Json::array*)value->payload);
    break;
  case Json::type_number:
    result = extract_get_number_size(
      (const Json::number*)value->payload);
    break;
  case Json::type_string:
    result = extract_get_string_size(
      (const Json::string*)value->payload);
    break;
  }

  result.dom_size += sizeof(Json::value);

  return result;
}

static extract_result_s extract_get_object_size(const Json::object* const object) {
  extract_result_s result;
  u64 i;
  const Json::object_element* element = object->start;

  result.dom_size = sizeof(Json::object) +
    (sizeof(Json::object_element) * object->length);
  result.data_size = 0;

  for (i = 0; i < object->length; i++) {
    const extract_result_s string_result =
      extract_get_string_size(element->name);
    const extract_result_s value_result =
      extract_get_value_size(element->value);

    result.dom_size += string_result.dom_size;
    result.data_size += string_result.data_size;

    result.dom_size += value_result.dom_size;
    result.data_size += value_result.data_size;

    element = element->next;
  }

  return result;
}

static extract_result_s extract_get_array_size(const Json::array* const array) {
  extract_result_s result;
  u64 i;
  const Json::array_element* element = array->start;

  result.dom_size = sizeof(Json::array) +
    (sizeof(Json::array_element) * array->length);
  result.data_size = 0;

  for (i = 0; i < array->length; i++) {
    const extract_result_s value_result =
      extract_get_value_size(element->value);

    result.dom_size += value_result.dom_size;
    result.data_size += value_result.data_size;

    element = element->next;
  }

  return result;
}

static void extract_copy_value(extract_state_s* const state, const Json::value* const value) {
  Json::string* string;
  Json::number* number;
  Json::object* object;
  Json::array* array;
  Json::value* new_value;

  memcpy(state->dom, value, sizeof(Json::value));
  new_value = (Json::value*)state->dom;
  state->dom += sizeof(Json::value);
  new_value->payload = state->dom;

  if (Json::type_string == value->type) {
    memcpy(state->dom, value->payload, sizeof(Json::string));
    string = (Json::string*)state->dom;
    state->dom += sizeof(Json::string);

    memcpy(state->data, string->string, string->string_size + 1);
    string->string = state->data;
    state->data += string->string_size + 1;
  }
  else if (Json::type_number == value->type) {
    memcpy(state->dom, value->payload, sizeof(Json::number));
    number = (Json::number*)state->dom;
    state->dom += sizeof(Json::number);

    memcpy(state->data, number->number, number->number_size);
    number->number = state->data;
    state->data += number->number_size;
  }
  else if (Json::type_object == value->type) {
    Json::object_element* element;
    u64 i;

    memcpy(state->dom, value->payload, sizeof(Json::object));
    object = (Json::object*)state->dom;
    state->dom += sizeof(Json::object);

    element = object->start;
    object->start = (Json::object_element*)state->dom;

    for (i = 0; i < object->length; i++) {
      Json::value* previous_value;
      Json::object_element* previous_element;

      memcpy(state->dom, element, sizeof(Json::object_element));
      element = (Json::object_element*)state->dom;
      state->dom += sizeof(Json::object_element);

      string = element->name;
      memcpy(state->dom, string, sizeof(Json::string));
      string = (Json::string*)state->dom;
      state->dom += sizeof(Json::string);
      element->name = string;

      memcpy(state->data, string->string, string->string_size + 1);
      string->string = state->data;
      state->data += string->string_size + 1;

      previous_value = element->value;
      element->value = (Json::value*)state->dom;
      extract_copy_value(state, previous_value);

      previous_element = element;
      element = element->next;

      if (element) {
        previous_element->next = (Json::object_element*)state->dom;
      }
    }
  }
  else if (Json::type_array == value->type) {
    Json::array_element* element;
    u64 i;

    memcpy(state->dom, value->payload, sizeof(Json::array));
    array = (Json::array*)state->dom;
    state->dom += sizeof(Json::array);

    element = array->start;
    array->start = (Json::array_element*)state->dom;

    for (i = 0; i < array->length; i++) {
      Json::value* previous_value;
      Json::array_element* previous_element;

      memcpy(state->dom, element, sizeof(Json::array_element));
      element = (Json::array_element*)state->dom;
      state->dom += sizeof(Json::array_element);

      previous_value = element->value;
      element->value = (Json::value*)state->dom;
      extract_copy_value(state, previous_value);

      previous_element = element;
      element = element->next;

      if (element) {
        previous_element->next = (Json::array_element*)state->dom;
      }
    }
  }
}

static Json::value* parse_ex(const void* src, u64 src_size, ParseFlags flags_bitset, void* (*alloc_func_ptr)(void* user_data, u64 size), void* user_data, parse_result_s* result) {
  parse_state_s state;
  void* allocation;
  Json::value* value;
  u64 total_size;
  i32 input_error;

  if (result) {
    result->error = ParseError::none;
    result->error_offset = 0;
    result->error_line_no = 0;
    result->error_row_no = 0;
  }

  if (nullptr == src) {
    return nullptr;
  }

  state.src = (const char*)src;
  state.size = src_size;
  state.offset = 0;
  state.line_no = 1;
  state.line_offset = 0;
  state.error = ParseError::none;
  state.dom_size = 0;
  state.data_size = 0;
  state.flags_bitset = flags_bitset;

  input_error = get_value_size(
    &state, (i32)(ParseFlags::allow_global_object & state.flags_bitset));

  if (0 == input_error) {
    skip_all_skippables(&state);

    if (state.offset != state.size) {
      state.error = ParseError::unexpected_trailing_characters;
      input_error = 1;
    }
  }

  if (input_error) {
    if (result) {
      result->error = state.error;
      result->error_offset = state.offset;
      result->error_line_no = state.line_no;
      result->error_row_no = state.offset - state.line_offset;
    }
    return nullptr;
  }

  total_size = state.dom_size + state.data_size;

  if (nullptr == alloc_func_ptr) {
    allocation = malloc(total_size);
  }
  else {
    allocation = alloc_func_ptr(user_data, total_size);
  }

  if (nullptr == allocation) {
    if (result) {
      result->error = ParseError::allocator_failed;
      result->error_offset = 0;
      result->error_line_no = 0;
      result->error_row_no = 0;
    }

    return nullptr;
  }

  state.offset = 0;
  state.line_no = 1;
  state.line_offset = 0;

  state.dom = (char*)allocation;
  state.data = state.dom + state.dom_size;

  if (to_underlying(ParseFlags::allow_location_information & state.flags_bitset)) {
    value_ex* value_ex_ = (value_ex*)state.dom;
    state.dom += sizeof(value_ex);

    value_ex_->offset = state.offset;
    value_ex_->line_no = state.line_no;
    value_ex_->row_no = state.offset - state.line_offset;

    value = &(value_ex_->value);
  }
  else {
    value = (Json::value*)state.dom;
    state.dom += sizeof(Json::value);
  }

  parse_value(
    &state, (i32)(ParseFlags::allow_global_object & state.flags_bitset),
    value);

  return (Json::value*)allocation;
}

Json::value* Json::parse(const void* src, u64 src_size) {
  return parse_ex(src, src_size, ParseFlags::default_, nullptr,
    nullptr, nullptr);
}



struct Gear
{
  struct Knob
  {
    struct EnumValue
    {
      std::string key;
      std::string value;
    };

    std::string rtpc;
    std::string type;
    std::string name;
    std::string unittype;
    std::string defaultValue;
    std::string minValue;
    std::string maxValue;
    std::string valueStep;
    std::string ring;
    std::vector<EnumValue> enumValues;
    std::string index;
  };

  std::string associatedCabKey;
  std::string category;
  std::string key;
  std::vector<Knob> knobs;
  std::string name;
  std::string soundBank;
  std::string type;
  std::string persistentId;
};

static void readKnob(Json::object_element* it, Gear::Knob& knob)
{
  if (0 == strcmp(it->name->string, "RTPC"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "Base"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "Type"))
  {
    assert(it->value->type == Json::type_number);
    return;
  }
  if (0 == strcmp(it->name->string, "PosX"))
  {
    assert(it->value->type == Json::type_number);
    return;
  }
  if (0 == strcmp(it->name->string, "PosY"))
  {
    assert(it->value->type == Json::type_number);
    return;
  }
  if (0 == strcmp(it->name->string, "Name"))
  {
    assert(it->value->type == Json::type_string);
    knob.name = ((Json::string*)it->value->payload)->string;
    return;
  }
  if (0 == strcmp(it->name->string, "UnitType"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "DefaultValue"))
  {
    assert(it->value->type == Json::type_number);
    knob.defaultValue = ((Json::string*)it->value->payload)->string;
    return;
  }
  if (0 == strcmp(it->name->string, "MinValue"))
  {
    assert(it->value->type == Json::type_number);
    knob.minValue = ((Json::string*)it->value->payload)->string;
    return;
  }
  if (0 == strcmp(it->name->string, "MaxValue"))
  {
    assert(it->value->type == Json::type_number);
    knob.maxValue = ((Json::string*)it->value->payload)->string;
    return;
  }
  if (0 == strcmp(it->name->string, "ValueStep"))
  {
    assert(it->value->type == Json::type_number);
    knob.valueStep = ((Json::string*)it->value->payload)->string;
    return;
  }
  if (0 == strcmp(it->name->string, "Ring"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "EnumValues"))
  {
    assert(it->value->type == Json::type_object);

    Json::value* arrValue = it->value;
    assert(arrValue->type == Json::type_object);

    Json::object* object = (Json::object*)arrValue->payload;

    Json::object_element* it2 = object->start;

    do
    {
      Gear::Knob::EnumValue enumValue;
      enumValue.key = it2->name->string;
      enumValue.value = ((Json::string*)it2->value->payload)->string;
      knob.enumValues.push_back(enumValue);
    } while (it2 = it2->next);

    return;
  }
  if (0 == strcmp(it->name->string, "Index"))
  {
    assert(it->value->type == Json::type_number);
    return;
  }

  assert(false);
}

static void readKnobs(Json::array_element* it, std::vector<Gear::Knob>& knobs)
{
  Json::value* arrValue = it->value;
  assert(arrValue->type == Json::type_object);

  Json::object* object = (Json::object*)arrValue->payload;

  Json::object_element* it2 = object->start;

  Gear::Knob knob;
  do
  {
    readKnob(it2, knob);
  } while (it2 = it2->next);
  if (knob.valueStep.empty()) // a few knobs are missing the valueStep
    knob.valueStep = "1.0";
  if (!knob.name.empty()) // a few knobs are not valid
  {
    assert(!knob.minValue.empty());
    assert(!knob.maxValue.empty());
    assert(!knob.valueStep.empty());

    knob.name = std::regex_replace(knob.name, std::regex("\\\""), "\\\""); // fix names with " in them

    knobs.push_back(knob);
  }

  return;
}

static void readAttribute(Json::object_element* it, Gear& gear)
{
  if (0 == strcmp(it->name->string, "3DArtAsset"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "AssociatedCabKey"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    gear.associatedCabKey = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "Category"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    gear.category = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "DefaultSkin"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "Description"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "DLC"))
  {
    assert(it->value->type == Json::type_true || it->value->type == Json::type_false);
    return;
  }
  if (0 == strcmp(it->name->string, "Effects"))
  {
    assert(it->value->type == Json::type_array);
    return;
  }
  if (0 == strcmp(it->name->string, "Key"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    gear.key = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "Knobs"))
  {
    assert(it->value->type == Json::type_array);
    Json::array* arr = (Json::array*)it->value->payload;
    Json::array_element* it2 = arr->start;

    do
    {
      readKnobs(it2, gear.knobs);
    } while (it2 = it2->next);
    return;

    return;
  }
  if (0 == strcmp(it->name->string, "ManifestUrn"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "Name"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    gear.name = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "PersonalityColor"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "PersonalityParticle"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "Skins"))
  {
    assert(it->value->type == Json::type_array);
    return;
  }
  if (0 == strcmp(it->name->string, "Skins3D"))
  {
    assert(it->value->type == Json::type_array);
    return;
  }
  if (0 == strcmp(it->name->string, "SoundBank"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    gear.soundBank = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "SpawnPoint"))
  {
    assert(it->value->type == Json::type_string);
    return;
  }
  if (0 == strcmp(it->name->string, "Type"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    gear.type = string->string;
    return;
  }
  if (0 == strcmp(it->name->string, "PersistentID"))
  {
    assert(it->value->type == Json::type_string);
    Json::string* string = (Json::string*)it->value->payload;
    gear.persistentId = string->string;
    return;
  }

  assert(false);
}

int main(int argc, char* argv[])
{
  std::vector<Gear> gearList;

  for (const auto& file : std::filesystem::directory_iterator{ std::filesystem::path(argv[1]) })
  {
    if (file.path().extension().string() != ".json")
      continue;

    FILE* inFile = fopen(file.path().string().c_str(), "rb");
    fseek(inFile, 0L, SEEK_END);
    const size_t lSize = ftell(inFile);

    std::vector<u8> fileData(lSize);

    rewind(inFile);
    fread(fileData.data(), lSize, 1, inFile);
    fclose(inFile);

    // there is a ',' in the file gear_pedal_octaveup.json that should not be there. The json parser has a problem with it.
    if (file.path().string().ends_with("gear_pedal_octaveup.json"))
    {
      const i32 fixPosition = 1349;
      assert(fileData.size() > fixPosition);
      assert(fileData[fixPosition] == ',');
      fileData[fixPosition] = ' ';
    }

    Gear gear;

    Json::value* root = Json::parse(fileData.data(), fileData.size());
    assert(root->type == Json::type_object);

    Json::object* object = (Json::object*)root->payload;
    assert(object->length == 4);

    Json::object_element* entries = object->start;

    assert(0 == strcmp(entries->name->string, "Entries"));

    Json::value* entries_value = entries->value;
    assert(entries_value->type == Json::type_object);

    Json::object* id_o = (Json::object*)entries_value->payload;
    assert(id_o->length == 1);

    Json::object_element* id = id_o->start;

    Json::value* id_value = id->value;
    Json::object* attrs_o = (Json::object*)id_value->payload;
    assert(attrs_o->length == 1);

    Json::object_element* attrs = attrs_o->start;
    assert(0 == strcmp(attrs->name->string, "Attributes"));

    Json::value* attrValue = attrs->value;
    assert(attrValue->type == Json::type_object);

    Json::object* attr_o = (Json::object*)attrValue->payload;

    Json::object_element* it = attr_o->start;

    do
    {
      readAttribute(it, gear);
    } while (it = it->next);

    gearList.push_back(gear);
  }

  std::sort(gearList.begin(), gearList.end(), [](const Gear& lhs, const Gear& rhs) {return lhs.name < rhs.name; });

  // fix name for Marshall 1962 "Bluesbreaker"
  for (Gear& gear : gearList)
    if (gear.name == "Marshall 1962 \"Bluesbreaker\" ")
      gear.name = "Marshall 1962 \\\"Bluesbreaker\\\"";

  std::vector<Gear> pedalList;
  std::vector<Gear> ampList;
  std::vector<Gear> cabinetList;
  std::vector<Gear> rackList;

  for (const Gear& gear : gearList)
  {
    if (gear.type == "Pedals")
      pedalList.push_back(gear);
    else if (gear.type == "Amps")
      ampList.push_back(gear);
    else if (gear.type == "Cabinets")
      cabinetList.push_back(gear);
    else if (gear.type == "Racks")
      rackList.push_back(gear);
    else
      assert(false);
  }

  char destFilepath[512];

  strcpy(destFilepath, argv[1]);

  size_t len = strlen(destFilepath);

  destFilepath[len++] = '.';
  destFilepath[len++] = 'd';
  destFilepath[len++] = 'a';
  destFilepath[len++] = 't';
  destFilepath[len++] = 'a';
  destFilepath[len++] = '\0';

  FILE* outFile = fopen(destFilepath, "w");
  {
    fprintf(outFile, "namespace Gear");
    fprintf(outFile, "\n{");
    fprintf(outFile, "\n  extern const char* pedalNames[%d];", pedalList.size() + 1);
    fprintf(outFile, "\n  extern const char* ampNames[%d];", ampList.size() + 1);
    fprintf(outFile, "\n  extern const char* cabinetNames[%d];", cabinetList.size() + 1);
    fprintf(outFile, "\n  extern const char* rackNames[%d];", rackList.size() + 1);
    fprintf(outFile, "\n");
    fprintf(outFile, "\n  struct Knob");
    fprintf(outFile, "\n  {");
    fprintf(outFile, "\n    const char* name;");
    fprintf(outFile, "\n    f32 defaultValue;");
    fprintf(outFile, "\n    f32 minValue;");
    fprintf(outFile, "\n    f32 maxValue;");
    fprintf(outFile, "\n    f32 valueStep;");
    fprintf(outFile, "\n  };");
    fprintf(outFile, "\n");
    fprintf(outFile, "\n  extern std::vector<Gear::Knob> pedalKnobs[%d];", pedalList.size());
    fprintf(outFile, "\n  extern std::vector<Gear::Knob> ampKnobs[%d];", ampList.size());
    fprintf(outFile, "\n  extern std::vector<Gear::Knob> rackKnobs[%d];", rackList.size());
    fprintf(outFile, "\n}");
  }
  fprintf(outFile, "\n\n");

  {
    fprintf(outFile, "const char* Data::Gear::pedalNames[%d] = {", pedalList.size() + 1);
    fprintf(outFile, "\n  \"\",");
    for (i32 i = 0; i < pedalList.size(); ++i)
    {
      const Gear& gear = pedalList[i];
      fprintf(outFile, "\n  \"%s\"", gear.name.c_str());
      if (i != pedalList.size() - 1)
        fprintf(outFile, ",");
    }
    fprintf(outFile, "\n};");
  }
  fprintf(outFile, "\n\n");
  {
    fprintf(outFile, "const char* Data::Gear::ampNames[%d] = {", ampList.size() + 1);
    fprintf(outFile, "\n  \"\",");
    for (i32 i = 0; i < ampList.size(); ++i)
    {
      const Gear& gear = ampList[i];
      fprintf(outFile, "\n  \"%s\"", gear.name.c_str());
      if (i != ampList.size() - 1)
        fprintf(outFile, ",");
    }
    fprintf(outFile, "\n};");
  }
  fprintf(outFile, "\n\n");
  {
    fprintf(outFile, "const char* Data::Gear::cabinetNames[%d] = {", cabinetList.size() + 1);
    fprintf(outFile, "\n  \"\",");
    for (i32 i = 0; i < cabinetList.size(); ++i)
    {
      const Gear& gear = cabinetList[i];
      fprintf(outFile, "\n  \"%s\"", gear.name.c_str());
      if (i != cabinetList.size() - 1)
        fprintf(outFile, ",");
    }
    fprintf(outFile, "\n};");
  }
  fprintf(outFile, "\n\n");
  {
    fprintf(outFile, "const char* Data::Gear::rackNames[%d] = {", rackList.size() + 1);
    fprintf(outFile, "\n  \"\",");
    for (i32 i = 0; i < rackList.size(); ++i)
    {
      const Gear& gear = rackList[i];
      fprintf(outFile, "\n  \"%s\"", gear.name.c_str());
      if (i != rackList.size() - 1)
        fprintf(outFile, ",");
    }
    fprintf(outFile, "\n};");
  }
  fprintf(outFile, "\n");

  fprintf(outFile, "\nstd::vector<Data::Gear::Knob> Data::Gear::pedalKnobs[%d] = {", pedalList.size());
  for (i32 i = 0; i < pedalList.size(); ++i)
  {
    const Gear& gear = pedalList[i];
    fprintf(outFile, "\n  { // %s", gear.name.c_str());
    for (i32 j = 0; j < gear.knobs.size(); ++j)
    {
      const Gear::Knob& knob = gear.knobs[j];
      fprintf(outFile, "\n    { \"%s\", %sf, %sf, %sf, %sf", knob.name.c_str(), knob.defaultValue.c_str(), knob.minValue.c_str(), knob.maxValue.c_str(), knob.valueStep.c_str());
      if (j != gear.knobs.size() - 1)
        fprintf(outFile, ", },");
      else
        fprintf(outFile, " }");
    }
    fprintf(outFile, "\n  }");
    if (i != pedalList.size() - 1)
      fprintf(outFile, ",");
  }
  fprintf(outFile, "\n};");
  fprintf(outFile, "\n");
  fprintf(outFile, "\nstd::vector<Data::Gear::Knob> Data::Gear::ampKnobs[%d] = {", ampList.size());
  for (i32 i = 0; i < ampList.size(); ++i)
  {
    const Gear& gear = ampList[i];
    fprintf(outFile, "\n  { // %s", gear.name.c_str());
    for (i32 j = 0; j < gear.knobs.size(); ++j)
    {
      const Gear::Knob& knob = gear.knobs[j];
      fprintf(outFile, "\n    { \"%s\", %sf, %sf, %sf, %sf", knob.name.c_str(), knob.defaultValue.c_str(), knob.minValue.c_str(), knob.maxValue.c_str(), knob.valueStep.c_str());
      if (j != gear.knobs.size() - 1)
        fprintf(outFile, ", },");
      else
        fprintf(outFile, " }");
    }
    fprintf(outFile, "\n  }");
    if (i != ampList.size() - 1)
      fprintf(outFile, ",");
  }
  fprintf(outFile, "\n};");
  fprintf(outFile, "\n");
  fprintf(outFile, "\nstd::vector<Data::Gear::Knob> Data::Gear::rackKnobs[%d] = {", rackList.size());
  for (i32 i = 0; i < rackList.size(); ++i)
  {
    const Gear& gear = rackList[i];
    fprintf(outFile, "\n  { // %s", gear.name.c_str());
    for (i32 j = 0; j < gear.knobs.size(); ++j)
    {
      const Gear::Knob& knob = gear.knobs[j];
      fprintf(outFile, "\n    { \"%s\", %sf, %sf, %sf, %sf", knob.name.c_str(), knob.defaultValue.c_str(), knob.minValue.c_str(), knob.maxValue.c_str(), knob.valueStep.c_str());
      if (j != gear.knobs.size() - 1)
        fprintf(outFile, ", },");
      else
        fprintf(outFile, " }");
    }
    fprintf(outFile, "\n  }");
    if (i != rackList.size() - 1)
      fprintf(outFile, ",");
  }
  fprintf(outFile, "\n};");
  fprintf(outFile, "\n");

  fprintf(outFile, "\n");
  fclose(outFile);

  return 0;
}
