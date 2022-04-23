#include "json.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

enum struct ParseFlags : u64 {
  default_ = 0,

  /* allow trailing commas in objects and arrays. For example, both [true,] and
  {"a" : null,} would be allowed with this option on. */
  allow_trailing_comma = 0x1,

  /* allow unquoted keys for objects. For example, {a : null} would be allowed
  with this option on. */
  allow_unquoted_keys = 0x2,

  /* allow a global unbracketed object. For example, a : null, b : true, c : {}
  would be allowed with this option on. */
  allow_global_object = 0x4,

  /* allow objects to use '=' instead of ':' between key/value pairs. For
  example, a = null, b : true would be allowed with this option on. */
  allow_equals_in_object = 0x8,

  /* allow that objects don't have to have comma separators between key/value
  pairs. */
  allow_no_commas = 0x10,

  /* allow c-style comments (either variants) to be ignored in the input JSON
  file. */
  allow_c_style_comments = 0x20,

  /* deprecated flag, unused. */
  deprecated = 0x40,

  /* record location information for each value. */
  allow_location_information = 0x80,

  /* allow strings to be 'single quoted'. */
  allow_single_quoted_strings = 0x100,

  /* allow numbers to be hexadecimal. */
  allow_hexadecimal_numbers = 0x200,

  /* allow numbers like +123 to be parsed. */
  allow_leading_plus_sign = 0x400,

  /* allow numbers like .0123 or 123. to be parsed. */
  allow_leading_or_trailing_decimal_point = 0x800,

  /* allow Infinity, -Infinity, NaN, -NaN. */
  allow_inf_and_nan = 0x1000,

  /* allow multi line string values. */
  allow_multi_line_strings = 0x2000,

  /* allow simplified JSON to be parsed. Simplified JSON is an enabling of a set
  of other parsing options. */
  allow_simplified_json =
  (allow_trailing_comma |
    allow_unquoted_keys |
    allow_global_object |
    allow_equals_in_object |
    allow_no_commas),

  /* allow JSON5 to be parsed. JSON5 is an enabling of a set of other parsing
     options. */
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

/* a parsing error code. */
enum struct ParseError {
  /* no error occurred (huzzah!). */
  none = 0,

  /* expected either a comma or a closing '}' or ']' to close an object or. */
  /* array! */
  expected_comma_or_closing_bracket,

  /* colon separating name/value pair was missing! */
  expected_colon,

  /* expected string to begin with '"'! */
  expected_opening_quote,

  /* invalid escaped sequence in string! */
  invalid_string_escape_sequence,

  /* invalid number format! */
  invalid_number_format,

  /* invalid value! */
  invalid_value,

  /* reached end of buffer before object/array was complete! */
  premature_end_of_buffer,

  /* string was malformed! */
  invalid_string,

  /* a call to malloc, or a user provider allocator, failed. */
  allocator_failed,

  /* the JSON input had unexpected trailing characters that weren't part of the. */
  /* JSON value. */
  unexpected_trailing_characters,

  /* catch-all error for everything else that exploded (real bad chi!). */
  unknown
};

/* A JSON string value (extended). */
struct string_ex_s {
  /* The JSON string this extends. */
  struct Json::string_s string;

  /* The character offset for the value in the JSON input. */
  size_t offset;

  /* The line number for the value in the JSON input. */
  size_t line_no;

  /* The row number for the value in the JSON input, in bytes. */
  size_t row_no;
};

/* a JSON value (extended). */
struct value_ex_s {
  /* the JSON value this extends. */
  struct Json::value_s value;

  /* the character offset for the value in the JSON input. */
  size_t offset;

  /* the line number for the value in the JSON input. */
  size_t line_no;

  /* the row number for the value in the JSON input, in bytes. */
  size_t row_no;
};

/* error report from parse_ex(). */
struct parse_result_s {
  /* the error code (one of ParseError::e). */
  ParseError error;

  /* the character offset for the error in the JSON input. */
  size_t error_offset;

  /* the line number for the error in the JSON input. */
  size_t error_line_no;

  /* the row number for the error, in bytes. */
  size_t error_row_no;
};

struct parse_state_s {
  const char* src;
  size_t size;
  size_t offset;
  ParseFlags flags_bitset;
  char* data;
  char* dom;
  size_t dom_size;
  size_t data_size;
  size_t line_no;  /* line counter for error reporting. */
  size_t line_offset; /* (offset-line_offset) is the character number (in
          bytes). */
  ParseError error;
};

struct extract_state_s {
  char* dom;
  char* data;
};

struct extract_result_s {
  size_t dom_size;
  size_t data_size;
};

static int get_object_size(struct parse_state_s* state, int is_global_object);
static void parse_value(struct parse_state_s* state, int is_global_object, struct Json::value_s* value);
static int get_array_size(struct parse_state_s* state);
static extract_result_s extract_get_array_size(const Json::array_s* const array);
static extract_result_s extract_get_object_size(const Json::object_s* const object);



static int hexadecimal_digit(const char c) {
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

static int hexadecimal_value(const char* c, const unsigned long size, unsigned long* result) {
  const char* p;
  int digit;

  if (size > sizeof(unsigned long) * 2) {
    return 0;
  }

  *result = 0;
  for (p = c; (unsigned long)(p - c) < size; ++p) {
    *result <<= 4;
    digit = hexadecimal_digit(*p);
    if (digit < 0 || digit > 15) {
      return 0;
    }
    *result |= (unsigned char)digit;
  }
  return 1;
}

static int skip_whitespace(parse_state_s* state) {
  size_t offset = state->offset;
  const size_t size = state->size;
  const char* const src = state->src;

  /* the only valid whitespace according to ECMA-404 is ' ', '\n', '\r' and
   * '\t'. */
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
      /* Update offset. */
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

  /* Update offset. */
  state->offset = offset;
  return 1;
}

static int skip_c_style_comments(parse_state_s* state) {
  /* do we have a comment?. */
  if ('/' == state->src[state->offset]) {
    /* skip '/'. */
    state->offset++;

    if ('/' == state->src[state->offset]) {
      /* we had a comment of the form //. */

      /* skip second '/'. */
      state->offset++;

      while (state->offset < state->size) {
        switch (state->src[state->offset]) {
        default:
          /* skip the character in the comment. */
          state->offset++;
          break;
        case '\n':
          /* if we have a newline, our comment has ended! Skip the newline. */
          state->offset++;

          /* we entered a newline, so move our line info forward. */
          state->line_no++;
          state->line_offset = state->offset;
          return 1;
        }
      }

      /* we reached the end of the JSON file! */
      return 1;
    }
    else if ('*' == state->src[state->offset]) {
      /* we had a comment in the C-style long form. */

      /* skip '*'. */
      state->offset++;

      while (state->offset + 1 < state->size) {
        if (('*' == state->src[state->offset]) &&
          ('/' == state->src[state->offset + 1])) {
          /* we reached the end of our comment! */
          state->offset += 2;
          return 1;
        }
        else if ('\n' == state->src[state->offset]) {
          /* we entered a newline, so move our line info forward. */
          state->line_no++;
          state->line_offset = state->offset;
        }

        /* skip character within comment. */
        state->offset++;
      }

      /* Comment wasn't ended correctly which is a failure. */
      return 1;
    }
  }

  /* we didn't have any comment, which is ok too! */
  return 0;
}

static int skip_all_skippables(parse_state_s* state) {
  /* skip all whitespace and other skippables until there are none left. note
   * that the previous version suffered from read past errors should. the
   * stream end on skip_c_style_comments eg. '{"a" ' with comments flag.
   */

  int did_consume = 0;
  const size_t size = state->size;

  if (to_underlying(ParseFlags::allow_c_style_comments & state->flags_bitset)) {
    do {
      if (state->offset == size) {
        state->error = ParseError::premature_end_of_buffer;
        return 1;
      }

      did_consume = skip_whitespace(state);

      /* This should really be checked on access, not in front of every call.
       */
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

static int get_string_size(parse_state_s* state, size_t is_key) {
  size_t offset = state->offset;
  const size_t size = state->size;
  size_t data_size = 0;
  const char* const src = state->src;
  const int is_single_quote = '\'' == src[offset];
  const char quote_to_use = is_single_quote ? '\'' : '"';
  const ParseFlags flags_bitset = state->flags_bitset;
  unsigned long codepoint;
  unsigned long high_surrogate = 0;

  if (to_underlying(ParseFlags::allow_location_information & flags_bitset) != 0 &&
    is_key != 0) {
    state->dom_size += sizeof(string_ex_s);
  }
  else {
    state->dom_size += sizeof(Json::string_s);
  }

  if ('"' != src[offset]) {
    /* if we are allowed single quoted strings check for that too. */
    if (!(to_underlying(ParseFlags::allow_single_quoted_strings & flags_bitset) &&
      is_single_quote)) {
      state->error = ParseError::expected_opening_quote;
      state->offset = offset;
      return 1;
    }
  }

  /* skip leading '"' or '\''. */
  offset++;

  while ((offset < size) && (quote_to_use != src[offset])) {
    /* add space for the character. */
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
      /* skip reverse solidus character. */
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
        /* all valid characters! */
        offset++;
        break;
      case 'u':
        if (!(offset + 5 < size)) {
          /* invalid escaped unicode sequence! */
          state->error = ParseError::invalid_string_escape_sequence;
          state->offset = offset;
          return 1;
        }

        codepoint = 0;
        if (!hexadecimal_value(&src[offset + 1], 4, &codepoint)) {
          /* escaped unicode sequences must contain 4 hexadecimal digits! */
          state->error = ParseError::invalid_string_escape_sequence;
          state->offset = offset;
          return 1;
        }

        /* Valid sequence!
         * see: https://en.wikipedia.org/wiki/UTF-8#Invalid_code_points.
         *      1       7       U + 0000        U + 007F        0xxxxxxx.
         *      2       11      U + 0080        U + 07FF        110xxxxx
         * 10xxxxxx.
         *      3       16      U + 0800        U + FFFF        1110xxxx
         * 10xxxxxx        10xxxxxx.
         *      4       21      U + 10000       U + 10FFFF      11110xxx
         * 10xxxxxx        10xxxxxx        10xxxxxx.
         * Note: the high and low surrogate halves used by UTF-16 (U+D800
         * through U+DFFF) and code points not encodable by UTF-16 (those after
         * U+10FFFF) are not legal Unicode values, and their UTF-8 encoding must
         * be treated as an invalid byte sequence. */

        if (high_surrogate != 0) {
          /* we previously read the high half of the \uxxxx\uxxxx pair, so now
           * we expect the low half. */
          if (codepoint >= 0xdc00 &&
            codepoint <= 0xdfff) { /* low surrogate range. */
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
          codepoint <= 0xdbff) { /* high surrogate range. */
 /* The codepoint is the first half of a "utf-16 surrogate pair". so we
  * need the other half for it to be valid: \uHHHH\uLLLL. */
          if (offset + 11 > size || '\\' != src[offset + 5] ||
            'u' != src[offset + 6]) {
            state->error = ParseError::invalid_string_escape_sequence;
            state->offset = offset;
            return 1;
          }
          high_surrogate = codepoint;
        }
        else if (codepoint >= 0xd800 &&
          codepoint <= 0xdfff) { /* low surrogate range. */
 /* we did not read the other half before. */
          state->error = ParseError::invalid_string_escape_sequence;
          state->offset = offset;
          return 1;
        }
        else {
          data_size += 2;
        }
        /* escaped codepoints after 0xffff are supported in json through utf-16
         * surrogate pairs: \uD83D\uDD25 for U+1F525. */

        offset += 5;
        break;
      }
    }
    else if (('\r' == src[offset]) || ('\n' == src[offset])) {
      if (!to_underlying(ParseFlags::allow_multi_line_strings & flags_bitset)) {
        /* invalid escaped unicode sequence! */
        state->error = ParseError::invalid_string_escape_sequence;
        state->offset = offset;
        return 1;
      }

      offset++;
    }
    else {
      /* skip character (valid part of sequence). */
      offset++;
    }
  }

  /* If the offset is equal to the size, we had a non-terminated string! */
  if (offset == size) {
    state->error = ParseError::premature_end_of_buffer;
    state->offset = offset - 1;
    return 1;
  }

  /* skip trailing '"' or '\''. */
  offset++;

  /* add enough space to store the string. */
  state->data_size += data_size;

  /* one more byte for null terminator ending the string! */
  state->data_size++;

  /* update offset. */
  state->offset = offset;

  return 0;
}

static int is_valid_unquoted_key_char(const char c) {
  return (('0' <= c && c <= '9') || ('a' <= c && c <= 'z') ||
    ('A' <= c && c <= 'Z') || ('_' == c));
}

static int get_key_size(parse_state_s* state) {
  const ParseFlags flags_bitset = state->flags_bitset;

  if (to_underlying(ParseFlags::allow_unquoted_keys & flags_bitset)) {
    size_t offset = state->offset;
    const size_t size = state->size;
    const char* const src = state->src;
    size_t data_size = state->data_size;

    /* if we are allowing unquoted keys, first grok for a quote... */
    if ('"' == src[offset]) {
      /* ... if we got a comma, just parse the key as a string as normal. */
      return get_string_size(state, 1);
    }
    else if (to_underlying(ParseFlags::allow_single_quoted_strings & flags_bitset) &&
      ('\'' == src[offset])) {
      /* ... if we got a comma, just parse the key as a string as normal. */
      return get_string_size(state, 1);
    }
    else {
      while ((offset < size) && is_valid_unquoted_key_char(src[offset])) {
        offset++;
        data_size++;
      }

      /* one more byte for null terminator ending the string! */
      data_size++;

      if (to_underlying(ParseFlags::allow_location_information & flags_bitset)) {
        state->dom_size += sizeof(string_ex_s);
      }
      else {
        state->dom_size += sizeof(Json::string_s);
      }

      /* update offset. */
      state->offset = offset;

      /* update data_size. */
      state->data_size = data_size;

      return 0;
    }
  }
  else {
    /* we are only allowed to have quoted keys, so just parse a string! */
    return get_string_size(state, 1);
  }
}

static int get_number_size(parse_state_s* state) {
  const ParseFlags flags_bitset = state->flags_bitset;
  size_t offset = state->offset;
  const size_t size = state->size;
  int had_leading_digits = 0;
  const char* const src = state->src;

  state->dom_size += sizeof(Json::number_s);

  if (to_underlying(ParseFlags::allow_hexadecimal_numbers & flags_bitset) &&
    (offset + 1 < size) && ('0' == src[offset]) &&
    (('x' == src[offset + 1]) || ('X' == src[offset + 1]))) {
    /* skip the leading 0x that identifies a hexadecimal number. */
    offset += 2;

    /* consume hexadecimal digits. */
    while ((offset < size) && (('0' <= src[offset] && src[offset] <= '9') ||
      ('a' <= src[offset] && src[offset] <= 'f') ||
      ('A' <= src[offset] && src[offset] <= 'F'))) {
      offset++;
    }
  }
  else {
    int found_sign = 0;
    int inf_or_nan = 0;

    if ((offset < size) &&
      (('-' == src[offset]) ||
        (to_underlying(ParseFlags::allow_leading_plus_sign & flags_bitset) &&
          ('+' == src[offset])))) {
      /* skip valid leading '-' or '+'. */
      offset++;

      found_sign = 1;
    }

    if (to_underlying(ParseFlags::allow_inf_and_nan & flags_bitset)) {
      const char inf[9] = "Infinity";
      const size_t inf_strlen = sizeof(inf) - 1;
      const char nan[4] = "NaN";
      const size_t nan_strlen = sizeof(nan) - 1;

      if (offset + inf_strlen < size) {
        int found = 1;
        size_t i;
        for (i = 0; i < inf_strlen; i++) {
          if (inf[i] != src[offset + i]) {
            found = 0;
            break;
          }
        }

        if (found) {
          /* We found our special 'Infinity' keyword! */
          offset += inf_strlen;

          inf_or_nan = 1;
        }
      }

      if (offset + nan_strlen < size) {
        int found = 1;
        size_t i;
        for (i = 0; i < nan_strlen; i++) {
          if (nan[i] != src[offset + i]) {
            found = 0;
            break;
          }
        }

        if (found) {
          /* We found our special 'NaN' keyword! */
          offset += nan_strlen;

          inf_or_nan = 1;
        }
      }
    }

    if (found_sign && !inf_or_nan && (offset < size) &&
      !('0' <= src[offset] && src[offset] <= '9')) {
      /* check if we are allowing leading '.'. */
      if (!to_underlying(ParseFlags::allow_leading_or_trailing_decimal_point &
        flags_bitset) ||
        ('.' != src[offset])) {
        /* a leading '-' must be immediately followed by any digit! */
        state->error = ParseError::invalid_number_format;
        state->offset = offset;
        return 1;
      }
    }

    if ((offset < size) && ('0' == src[offset])) {
      /* skip valid '0'. */
      offset++;

      /* we need to record whether we had any leading digits for checks later.
       */
      had_leading_digits = 1;

      if ((offset < size) && ('0' <= src[offset] && src[offset] <= '9')) {
        /* a leading '0' must not be immediately followed by any digit! */
        state->error = ParseError::invalid_number_format;
        state->offset = offset;
        return 1;
      }
    }

    /* the main digits of our number next. */
    while ((offset < size) && ('0' <= src[offset] && src[offset] <= '9')) {
      offset++;

      /* we need to record whether we had any leading digits for checks later.
       */
      had_leading_digits = 1;
    }

    if ((offset < size) && ('.' == src[offset])) {
      offset++;

      if (!('0' <= src[offset] && src[offset] <= '9')) {
        if (!to_underlying(ParseFlags::allow_leading_or_trailing_decimal_point &
          flags_bitset) ||
          !had_leading_digits) {
          /* a decimal point must be followed by at least one digit. */
          state->error = ParseError::invalid_number_format;
          state->offset = offset;
          return 1;
        }
      }

      /* a decimal point can be followed by more digits of course! */
      while ((offset < size) && ('0' <= src[offset] && src[offset] <= '9')) {
        offset++;
      }
    }

    if ((offset < size) && ('e' == src[offset] || 'E' == src[offset])) {
      /* our number has an exponent! Skip 'e' or 'E'. */
      offset++;

      if ((offset < size) && ('-' == src[offset] || '+' == src[offset])) {
        /* skip optional '-' or '+'. */
        offset++;
      }

      if ((offset < size) && !('0' <= src[offset] && src[offset] <= '9')) {
        /* an exponent must have at least one digit! */
        state->error = ParseError::invalid_number_format;
        state->offset = offset;
        return 1;
      }

      /* consume exponent digits. */
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
      /* all of the above are ok. */
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

  /* one more byte for null terminator ending the number string! */
  state->data_size++;

  /* update offset. */
  state->offset = offset;

  return 0;
}

static int get_value_size(parse_state_s* state, int is_global_object) {
  const ParseFlags flags_bitset = state->flags_bitset;
  const char* const src = state->src;
  size_t offset;
  const size_t size = state->size;

  if (to_underlying(ParseFlags::allow_location_information & flags_bitset)) {
    state->dom_size += sizeof(value_ex_s);
  }
  else {
    state->dom_size += sizeof(Json::value_s);
  }

  if (is_global_object) {
    return get_object_size(state, /* is_global_object = */ 1);
  }
  else {
    if (skip_all_skippables(state)) {
      state->error = ParseError::premature_end_of_buffer;
      return 1;
    }

    /* can cache offset now. */
    offset = state->offset;

    switch (src[offset]) {
    case '"':
      return get_string_size(state, 0);
    case '\'':
      if (to_underlying(ParseFlags::allow_single_quoted_strings & flags_bitset)) {
        return get_string_size(state, 0);
      }
      else {
        /* invalid value! */
        state->error = ParseError::invalid_value;
        return 1;
      }
    case '{':
      return get_object_size(state, /* is_global_object = */ 0);
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
        /* invalid value! */
        state->error = ParseError::invalid_number_format;
        return 1;
      }
    case '.':
      if (to_underlying(ParseFlags::allow_leading_or_trailing_decimal_point &
        flags_bitset)) {
        return get_number_size(state);
      }
      else {
        /* invalid value! */
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

      /* invalid value! */
      state->error = ParseError::invalid_value;
      return 1;
    }
  }
}

static int get_object_size(parse_state_s* state, int is_global_object) {
  const ParseFlags flags_bitset = state->flags_bitset;
  const char* const src = state->src;
  const size_t size = state->size;
  size_t elements = 0;
  int allow_comma = 0;
  int found_closing_brace = 0;

  if (is_global_object) {
    /* if we found an opening '{' of an object, we actually have a normal JSON
     * object at the root of the DOM... */
    if (!skip_all_skippables(state) && '{' == state->src[state->offset]) {
      /* . and we don't actually have a global object after all! */
      is_global_object = 0;
    }
  }

  if (!is_global_object) {
    if ('{' != src[state->offset]) {
      state->error = ParseError::unknown;
      return 1;
    }

    /* skip leading '{'. */
    state->offset++;
  }

  state->dom_size += sizeof(Json::object_s);

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
        /* skip trailing '}'. */
        state->offset++;

        found_closing_brace = 1;

        /* finished the object! */
        break;
      }
    }
    else {
      /* we don't require brackets, so that means the object ends when the input
       * stream ends! */
      if (skip_all_skippables(state)) {
        break;
      }
    }

    /* if we parsed at least once element previously, grok for a comma. */
    if (allow_comma) {
      if (',' == src[state->offset]) {
        /* skip comma. */
        state->offset++;
        allow_comma = 0;
      }
      else if (to_underlying(ParseFlags::allow_no_commas & flags_bitset)) {
        /* we don't require a comma, and we didn't find one, which is ok! */
        allow_comma = 0;
      }
      else {
        /* otherwise we are required to have a comma, and we found none. */
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
      /* key parsing failed! */
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

    /* skip colon. */
    state->offset++;

    if (skip_all_skippables(state)) {
      state->error = ParseError::premature_end_of_buffer;
      return 1;
    }

    if (get_value_size(state, /* is_global_object = */ 0)) {
      /* value parsing failed! */
      return 1;
    }

    /* successfully parsed a name/value pair! */
    elements++;
    allow_comma = 1;
  } while (state->offset < size);

  if ((state->offset == size) && !is_global_object && !found_closing_brace) {
    state->error = ParseError::premature_end_of_buffer;
    return 1;
  }

  state->dom_size += sizeof(Json::object_element_s) * elements;

  return 0;
}

static int get_array_size(parse_state_s* state) {
  const ParseFlags flags_bitset = state->flags_bitset;
  size_t elements = 0;
  int allow_comma = 0;
  const char* const src = state->src;
  const size_t size = state->size;

  if ('[' != src[state->offset]) {
    /* expected array to begin with leading '['. */
    state->error = ParseError::unknown;
    return 1;
  }

  /* skip leading '['. */
  state->offset++;

  state->dom_size += sizeof(Json::array_s);

  while (state->offset < size) {
    if (skip_all_skippables(state)) {
      state->error = ParseError::premature_end_of_buffer;
      return 1;
    }

    if (']' == src[state->offset]) {
      /* skip trailing ']'. */
      state->offset++;

      state->dom_size += sizeof(Json::array_element_s) * elements;

      /* finished the object! */
      return 0;
    }

    /* if we parsed at least once element previously, grok for a comma. */
    if (allow_comma) {
      if (',' == src[state->offset]) {
        /* skip comma. */
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

    if (get_value_size(state, /* is_global_object = */ 0)) {
      /* value parsing failed! */
      return 1;
    }

    /* successfully parsed an array element! */
    elements++;
    allow_comma = 1;
  }

  /* we consumed the entire input before finding the closing ']' of the array!
   */
  state->error = ParseError::premature_end_of_buffer;
  return 1;
}

static void parse_string(parse_state_s* state, Json::string_s* string) {
  size_t offset = state->offset;
  size_t bytes_written = 0;
  const char* const src = state->src;
  const char quote_to_use = '\'' == src[offset] ? '\'' : '"';
  char* data = state->data;
  unsigned long high_surrogate = 0;
  unsigned long codepoint;

  string->string = data;

  /* skip leading '"' or '\''. */
  offset++;

  while (quote_to_use != src[offset]) {
    if ('\\' == src[offset]) {
      /* skip the reverse solidus. */
      offset++;

      switch (src[offset++]) {
      default:
        return; /* we cannot ever reach here. */
      case 'u': {
        codepoint = 0;
        if (!hexadecimal_value(&src[offset], 4, &codepoint)) {
          return; /* this shouldn't happen as the value was already validated.
                   */
        }

        offset += 4;

        if (codepoint <= 0x7fu) {
          data[bytes_written++] = (char)codepoint; /* 0xxxxxxx. */
        }
        else if (codepoint <= 0x7ffu) {
          data[bytes_written++] =
            (char)(0xc0u | (codepoint >> 6)); /* 110xxxxx. */
          data[bytes_written++] =
            (char)(0x80u | (codepoint & 0x3fu)); /* 10xxxxxx. */
        }
        else if (codepoint >= 0xd800 &&
          codepoint <= 0xdbff) { /* high surrogate. */
          high_surrogate = codepoint;
          continue; /* we need the low half to form a complete codepoint. */
        }
        else if (codepoint >= 0xdc00 &&
          codepoint <= 0xdfff) { /* low surrogate. */
 /* combine with the previously read half to obtain the complete
  * codepoint. */
          const unsigned long surrogate_offset =
            0x10000u - (0xD800u << 10) - 0xDC00u;
          codepoint = (high_surrogate << 10) + codepoint + surrogate_offset;
          high_surrogate = 0;
          data[bytes_written++] =
            (char)(0xF0u | (codepoint >> 18)); /* 11110xxx. */
          data[bytes_written++] =
            (char)(0x80u | ((codepoint >> 12) & 0x3fu)); /* 10xxxxxx. */
          data[bytes_written++] =
            (char)(0x80u | ((codepoint >> 6) & 0x3fu)); /* 10xxxxxx. */
          data[bytes_written++] =
            (char)(0x80u | (codepoint & 0x3fu)); /* 10xxxxxx. */
        }
        else {
          /* we assume the value was validated and thus is within the valid
           * range. */
          data[bytes_written++] =
            (char)(0xe0u | (codepoint >> 12)); /* 1110xxxx. */
          data[bytes_written++] =
            (char)(0x80u | ((codepoint >> 6) & 0x3fu)); /* 10xxxxxx. */
          data[bytes_written++] =
            (char)(0x80u | (codepoint & 0x3fu)); /* 10xxxxxx. */
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

        /* check if we have a "\r\n" sequence. */
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
      /* copy the character. */
      data[bytes_written++] = src[offset++];
    }
  }

  /* skip trailing '"' or '\''. */
  offset++;

  /* record the size of the string. */
  string->string_size = bytes_written;

  /* add null terminator to string. */
  data[bytes_written++] = '\0';

  /* move data along. */
  state->data += bytes_written;

  /* update offset. */
  state->offset = offset;
}

static void parse_key(parse_state_s* state, Json::string_s* string) {
  if (to_underlying(ParseFlags::allow_unquoted_keys & state->flags_bitset)) {
    const char* const src = state->src;
    char* const data = state->data;
    size_t offset = state->offset;

    /* if we are allowing unquoted keys, check for quoted anyway... */
    if (('"' == src[offset]) || ('\'' == src[offset])) {
      /* ... if we got a quote, just parse the key as a string as normal. */
      parse_string(state, string);
    }
    else {
      size_t size = 0;

      string->string = state->data;

      while (is_valid_unquoted_key_char(src[offset])) {
        data[size++] = src[offset++];
      }

      /* add null terminator to string. */
      data[size] = '\0';

      /* record the size of the string. */
      string->string_size = size++;

      /* move data along. */
      state->data += size;

      /* update offset. */
      state->offset = offset;
    }
  }
  else {
    /* we are only allowed to have quoted keys, so just parse a string! */
    parse_string(state, string);
  }
}

static void parse_object(parse_state_s* state, int is_global_object, Json::object_s* object) {
  const ParseFlags flags_bitset = state->flags_bitset;
  const size_t size = state->size;
  const char* const src = state->src;
  size_t elements = 0;
  int allow_comma = 0;
  Json::object_element_s* previous = nullptr;

  if (is_global_object) {
    /* if we skipped some whitespace, and then found an opening '{' of an. */
    /* object, we actually have a normal JSON object at the root of the DOM...
     */
    if ('{' == src[state->offset]) {
      /* . and we don't actually have a global object after all! */
      is_global_object = 0;
    }
  }

  if (!is_global_object) {
    /* skip leading '{'. */
    state->offset++;
  }

  (void)skip_all_skippables(state);

  /* reset elements. */
  elements = 0;

  while (state->offset < size) {
    Json::object_element_s* element = nullptr;
    Json::string_s* string = nullptr;
    Json::value_s* value = nullptr;

    if (!is_global_object) {
      (void)skip_all_skippables(state);

      if ('}' == src[state->offset]) {
        /* skip trailing '}'. */
        state->offset++;

        /* finished the object! */
        break;
      }
    }
    else {
      if (skip_all_skippables(state)) {
        /* global object ends when the file ends! */
        break;
      }
    }

    /* if we parsed at least one element previously, grok for a comma. */
    if (allow_comma) {
      if (',' == src[state->offset]) {
        /* skip comma. */
        state->offset++;
        allow_comma = 0;
        continue;
      }
    }

    element = (Json::object_element_s*)state->dom;

    state->dom += sizeof(Json::object_element_s);

    if (nullptr == previous) {
      /* this is our first element, so record it in our object. */
      object->start = element;
    }
    else {
      previous->next = element;
    }

    previous = element;

    if (to_underlying(ParseFlags::allow_location_information & flags_bitset)) {
      string_ex_s* string_ex =
        (string_ex_s*)state->dom;
      state->dom += sizeof(string_ex_s);

      string_ex->offset = state->offset;
      string_ex->line_no = state->line_no;
      string_ex->row_no = state->offset - state->line_offset;

      string = &(string_ex->string);
    }
    else {
      string = (Json::string_s*)state->dom;
      state->dom += sizeof(Json::string_s);
    }

    element->name = string;

    (void)parse_key(state, string);

    (void)skip_all_skippables(state);

    /* skip colon or equals. */
    state->offset++;

    (void)skip_all_skippables(state);

    if (to_underlying(ParseFlags::allow_location_information & flags_bitset)) {
      value_ex_s* value_ex = (value_ex_s*)state->dom;
      state->dom += sizeof(value_ex_s);

      value_ex->offset = state->offset;
      value_ex->line_no = state->line_no;
      value_ex->row_no = state->offset - state->line_offset;

      value = &(value_ex->value);
    }
    else {
      value = (Json::value_s*)state->dom;
      state->dom += sizeof(Json::value_s);
    }

    element->value = value;

    parse_value(state, /* is_global_object = */ 0, value);

    /* successfully parsed a name/value pair! */
    elements++;
    allow_comma = 1;
  }

  /* if we had at least one element, end the linked list. */
  if (previous) {
    previous->next = nullptr;
  }

  if (0 == elements) {
    object->start = nullptr;
  }

  object->length = elements;
}

static void parse_array(parse_state_s* state, Json::array_s* array) {
  const char* const src = state->src;
  const size_t size = state->size;
  size_t elements = 0;
  int allow_comma = 0;
  Json::array_element_s* previous = nullptr;

  /* skip leading '['. */
  state->offset++;

  (void)skip_all_skippables(state);

  /* reset elements. */
  elements = 0;

  do {
    Json::array_element_s* element = nullptr;
    Json::value_s* value = nullptr;

    (void)skip_all_skippables(state);

    if (']' == src[state->offset]) {
      /* skip trailing ']'. */
      state->offset++;

      /* finished the array! */
      break;
    }

    /* if we parsed at least one element previously, grok for a comma. */
    if (allow_comma) {
      if (',' == src[state->offset]) {
        /* skip comma. */
        state->offset++;
        allow_comma = 0;
        continue;
      }
    }

    element = (Json::array_element_s*)state->dom;

    state->dom += sizeof(Json::array_element_s);

    if (nullptr == previous) {
      /* this is our first element, so record it in our array. */
      array->start = element;
    }
    else {
      previous->next = element;
    }

    previous = element;

    if (to_underlying(ParseFlags::allow_location_information & state->flags_bitset)) {
      value_ex_s* value_ex = (value_ex_s*)state->dom;
      state->dom += sizeof(value_ex_s);

      value_ex->offset = state->offset;
      value_ex->line_no = state->line_no;
      value_ex->row_no = state->offset - state->line_offset;

      value = &(value_ex->value);
    }
    else {
      value = (Json::value_s*)state->dom;
      state->dom += sizeof(Json::value_s);
    }

    element->value = value;

    parse_value(state, /* is_global_object = */ 0, value);

    /* successfully parsed an array element! */
    elements++;
    allow_comma = 1;
  } while (state->offset < size);

  /* end the linked list. */
  if (previous) {
    previous->next = nullptr;
  }

  if (0 == elements) {
    array->start = nullptr;
  }

  array->length = elements;
}

static void parse_number(parse_state_s* state, Json::number_s* number) {
  const ParseFlags flags_bitset = state->flags_bitset;
  size_t offset = state->offset;
  const size_t size = state->size;
  size_t bytes_written = 0;
  const char* const src = state->src;
  char* data = state->data;

  number->number = data;

  if (to_underlying(ParseFlags::allow_hexadecimal_numbers & flags_bitset)) {
    if (('0' == src[offset]) &&
      (('x' == src[offset + 1]) || ('X' == src[offset + 1]))) {
      /* consume hexadecimal digits. */
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
    int end = 0;

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
    const size_t inf_strlen = 8; /* = strlen("Infinity");. */
    const size_t nan_strlen = 3; /* = strlen("NaN");. */

    if (offset + inf_strlen < size) {
      if ('I' == src[offset]) {
        size_t i;
        /* We found our special 'Infinity' keyword! */
        for (i = 0; i < inf_strlen; i++) {
          data[bytes_written++] = src[offset++];
        }
      }
    }

    if (offset + nan_strlen < size) {
      if ('N' == src[offset]) {
        size_t i;
        /* We found our special 'NaN' keyword! */
        for (i = 0; i < nan_strlen; i++) {
          data[bytes_written++] = src[offset++];
        }
      }
    }
  }

  /* record the size of the number. */
  number->number_size = bytes_written;
  /* add null terminator to number string. */
  data[bytes_written++] = '\0';
  /* move data along. */
  state->data += bytes_written;
  /* update offset. */
  state->offset = offset;
}

static void parse_value(parse_state_s* state, int is_global_object, Json::value_s* value) {
  const ParseFlags flags_bitset = state->flags_bitset;
  const char* const src = state->src;
  const size_t size = state->size;
  size_t offset;

  (void)skip_all_skippables(state);

  /* cache offset now. */
  offset = state->offset;

  if (is_global_object) {
    value->type = Json::type_object;
    value->payload = state->dom;
    state->dom += sizeof(Json::object_s);
    parse_object(state, /* is_global_object = */ 1,
      (Json::object_s*)value->payload);
  }
  else {
    switch (src[offset]) {
    case '"':
    case '\'':
      value->type = Json::type_string;
      value->payload = state->dom;
      state->dom += sizeof(Json::string_s);
      parse_string(state, (Json::string_s*)value->payload);
      break;
    case '{':
      value->type = Json::type_object;
      value->payload = state->dom;
      state->dom += sizeof(Json::object_s);
      parse_object(state, /* is_global_object = */ 0,
        (Json::object_s*)value->payload);
      break;
    case '[':
      value->type = Json::type_array;
      value->payload = state->dom;
      state->dom += sizeof(Json::array_s);
      parse_array(state, (Json::array_s*)value->payload);
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
      state->dom += sizeof(Json::number_s);
      parse_number(state, (Json::number_s*)value->payload);
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
        state->dom += sizeof(Json::number_s);
        parse_number(state, (Json::number_s*)value->payload);
      }
      else if (to_underlying(ParseFlags::allow_inf_and_nan & flags_bitset) &&
        (offset + 8) <= size && 'I' == src[offset + 0] &&
        'n' == src[offset + 1] && 'f' == src[offset + 2] &&
        'i' == src[offset + 3] && 'n' == src[offset + 4] &&
        'i' == src[offset + 5] && 't' == src[offset + 6] &&
        'y' == src[offset + 7]) {
        value->type = Json::type_number;
        value->payload = state->dom;
        state->dom += sizeof(Json::number_s);
        parse_number(state, (Json::number_s*)value->payload);
      }
      break;
    }
  }
}

static extract_result_s extract_get_number_size(const Json::number_s* const number) {
  extract_result_s result;
  result.dom_size = sizeof(Json::number_s);
  result.data_size = number->number_size;
  return result;
}

static extract_result_s extract_get_string_size(const Json::string_s* const string) {
  extract_result_s result;
  result.dom_size = sizeof(Json::string_s);
  result.data_size = string->string_size + 1;
  return result;
}

static extract_result_s extract_get_value_size(const Json::value_s* const value) {
  extract_result_s result = { 0, 0 };

  switch (value->type) {
  default:
    break;
  case Json::type_object:
    result = extract_get_object_size(
      (const Json::object_s*)value->payload);
    break;
  case Json::type_array:
    result = extract_get_array_size(
      (const Json::array_s*)value->payload);
    break;
  case Json::type_number:
    result = extract_get_number_size(
      (const Json::number_s*)value->payload);
    break;
  case Json::type_string:
    result = extract_get_string_size(
      (const Json::string_s*)value->payload);
    break;
  }

  result.dom_size += sizeof(Json::value_s);

  return result;
}

static extract_result_s extract_get_object_size(const Json::object_s* const object) {
  extract_result_s result;
  size_t i;
  const Json::object_element_s* element = object->start;

  result.dom_size = sizeof(Json::object_s) +
    (sizeof(Json::object_element_s) * object->length);
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

static extract_result_s extract_get_array_size(const Json::array_s* const array) {
  extract_result_s result;
  size_t i;
  const Json::array_element_s* element = array->start;

  result.dom_size = sizeof(Json::array_s) +
    (sizeof(Json::array_element_s) * array->length);
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

static void extract_copy_value(extract_state_s* const state, const Json::value_s* const value) {
  Json::string_s* string;
  Json::number_s* number;
  Json::object_s* object;
  Json::array_s* array;
  Json::value_s* new_value;

  memcpy(state->dom, value, sizeof(Json::value_s));
  new_value = (Json::value_s*)state->dom;
  state->dom += sizeof(Json::value_s);
  new_value->payload = state->dom;

  if (Json::type_string == value->type) {
    memcpy(state->dom, value->payload, sizeof(Json::string_s));
    string = (Json::string_s*)state->dom;
    state->dom += sizeof(Json::string_s);

    memcpy(state->data, string->string, string->string_size + 1);
    string->string = state->data;
    state->data += string->string_size + 1;
  }
  else if (Json::type_number == value->type) {
    memcpy(state->dom, value->payload, sizeof(Json::number_s));
    number = (Json::number_s*)state->dom;
    state->dom += sizeof(Json::number_s);

    memcpy(state->data, number->number, number->number_size);
    number->number = state->data;
    state->data += number->number_size;
  }
  else if (Json::type_object == value->type) {
    Json::object_element_s* element;
    size_t i;

    memcpy(state->dom, value->payload, sizeof(Json::object_s));
    object = (Json::object_s*)state->dom;
    state->dom += sizeof(Json::object_s);

    element = object->start;
    object->start = (Json::object_element_s*)state->dom;

    for (i = 0; i < object->length; i++) {
      Json::value_s* previous_value;
      Json::object_element_s* previous_element;

      memcpy(state->dom, element, sizeof(Json::object_element_s));
      element = (Json::object_element_s*)state->dom;
      state->dom += sizeof(Json::object_element_s);

      string = element->name;
      memcpy(state->dom, string, sizeof(Json::string_s));
      string = (Json::string_s*)state->dom;
      state->dom += sizeof(Json::string_s);
      element->name = string;

      memcpy(state->data, string->string, string->string_size + 1);
      string->string = state->data;
      state->data += string->string_size + 1;

      previous_value = element->value;
      element->value = (Json::value_s*)state->dom;
      extract_copy_value(state, previous_value);

      previous_element = element;
      element = element->next;

      if (element) {
        previous_element->next = (Json::object_element_s*)state->dom;
      }
    }
  }
  else if (Json::type_array == value->type) {
    Json::array_element_s* element;
    size_t i;

    memcpy(state->dom, value->payload, sizeof(Json::array_s));
    array = (Json::array_s*)state->dom;
    state->dom += sizeof(Json::array_s);

    element = array->start;
    array->start = (Json::array_element_s*)state->dom;

    for (i = 0; i < array->length; i++) {
      Json::value_s* previous_value;
      Json::array_element_s* previous_element;

      memcpy(state->dom, element, sizeof(Json::array_element_s));
      element = (Json::array_element_s*)state->dom;
      state->dom += sizeof(Json::array_element_s);

      previous_value = element->value;
      element->value = (Json::value_s*)state->dom;
      extract_copy_value(state, previous_value);

      previous_element = element;
      element = element->next;

      if (element) {
        previous_element->next = (Json::array_element_s*)state->dom;
      }
    }
  }
}

static Json::value_s* parse_ex(const void* src, size_t src_size, ParseFlags flags_bitset, void* (*alloc_func_ptr)(void* user_data, size_t size), void* user_data, parse_result_s* result) {
  parse_state_s state;
  void* allocation;
  Json::value_s* value;
  size_t total_size;
  int input_error;

  if (result) {
    result->error = ParseError::none;
    result->error_offset = 0;
    result->error_line_no = 0;
    result->error_row_no = 0;
  }

  if (nullptr == src) {
    /* invalid src pointer was null! */
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
    &state, (int)(ParseFlags::allow_global_object & state.flags_bitset));

  if (0 == input_error) {
    skip_all_skippables(&state);

    if (state.offset != state.size) {
      /* our parsing didn't have an error, but there are characters remaining in
       * the input that weren't part of the JSON! */

      state.error = ParseError::unexpected_trailing_characters;
      input_error = 1;
    }
  }

  if (input_error) {
    /* parsing value's size failed (most likely an invalid JSON DOM!). */
    if (result) {
      result->error = state.error;
      result->error_offset = state.offset;
      result->error_line_no = state.line_no;
      result->error_row_no = state.offset - state.line_offset;
    }
    return nullptr;
  }

  /* our total allocation is the combination of the dom and data sizes (we. */
  /* first encode the structure of the JSON, and then the data referenced by. */
  /* the JSON values). */
  total_size = state.dom_size + state.data_size;

  if (nullptr == alloc_func_ptr) {
    allocation = malloc(total_size);
  }
  else {
    allocation = alloc_func_ptr(user_data, total_size);
  }

  if (nullptr == allocation) {
    /* malloc failed! */
    if (result) {
      result->error = ParseError::allocator_failed;
      result->error_offset = 0;
      result->error_line_no = 0;
      result->error_row_no = 0;
    }

    return nullptr;
  }

  /* reset offset so we can reuse it. */
  state.offset = 0;

  /* reset the line information so we can reuse it. */
  state.line_no = 1;
  state.line_offset = 0;

  state.dom = (char*)allocation;
  state.data = state.dom + state.dom_size;

  if (to_underlying(ParseFlags::allow_location_information & state.flags_bitset)) {
    value_ex_s* value_ex = (value_ex_s*)state.dom;
    state.dom += sizeof(value_ex_s);

    value_ex->offset = state.offset;
    value_ex->line_no = state.line_no;
    value_ex->row_no = state.offset - state.line_offset;

    value = &(value_ex->value);
  }
  else {
    value = (Json::value_s*)state.dom;
    state.dom += sizeof(Json::value_s);
  }

  parse_value(
    &state, (int)(ParseFlags::allow_global_object & state.flags_bitset),
    value);

  return (Json::value_s*)allocation;
}

Json::value_s* Json::parse(const void* src, size_t src_size) {
  return parse_ex(src, src_size, ParseFlags::default_, nullptr,
    nullptr, nullptr);
}