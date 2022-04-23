#include "json.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

int Json::hexadecimal_digit(const char c) {
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

int Json::hexadecimal_value(const char *c, const unsigned long size,
                           unsigned long *result) {
  const char *p;
  int digit;

  if (size > sizeof(unsigned long) * 2) {
    return 0;
  }

  *result = 0;
  for (p = c; (unsigned long)(p - c) < size; ++p) {
    *result <<= 4;
    digit = Json::hexadecimal_digit(*p);
    if (digit < 0 || digit > 15) {
      return 0;
    }
    *result |= (unsigned char)digit;
  }
  return 1;
}

int Json::skip_whitespace(struct Json::parse_state_s *state) {
  size_t offset = state->offset;
  const size_t size = state->size;
  const char *const src = state->src;

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

int Json::skip_c_style_comments(struct Json::parse_state_s *state) {
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
    } else if ('*' == state->src[state->offset]) {
      /* we had a comment in the C-style long form. */

      /* skip '*'. */
      state->offset++;

      while (state->offset + 1 < state->size) {
        if (('*' == state->src[state->offset]) &&
            ('/' == state->src[state->offset + 1])) {
          /* we reached the end of our comment! */
          state->offset += 2;
          return 1;
        } else if ('\n' == state->src[state->offset]) {
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

int Json::skip_all_skippables(struct Json::parse_state_s *state) {
  /* skip all whitespace and other skippables until there are none left. note
   * that the previous version suffered from read past errors should. the
   * stream end on Json::skip_c_style_comments eg. '{"a" ' with comments flag.
   */

  int did_consume = 0;
  const size_t size = state->size;

  if (Json::parse_flags_allow_c_style_comments & state->flags_bitset) {
    do {
      if (state->offset == size) {
        state->error = Json::parse_error_premature_end_of_buffer;
        return 1;
      }

      did_consume = Json::skip_whitespace(state);

      /* This should really be checked on access, not in front of every call.
       */
      if (state->offset == size) {
        state->error = Json::parse_error_premature_end_of_buffer;
        return 1;
      }

      did_consume |= Json::skip_c_style_comments(state);
    } while (0 != did_consume);
  } else {
    do {
      if (state->offset == size) {
        state->error = Json::parse_error_premature_end_of_buffer;
        return 1;
      }

      did_consume = Json::skip_whitespace(state);
    } while (0 != did_consume);
  }

  if (state->offset == size) {
    state->error = Json::parse_error_premature_end_of_buffer;
    return 1;
  }

  return 0;
}

int Json::get_string_size(struct Json::parse_state_s *state, size_t is_key) {
  size_t offset = state->offset;
  const size_t size = state->size;
  size_t data_size = 0;
  const char *const src = state->src;
  const int is_single_quote = '\'' == src[offset];
  const char quote_to_use = is_single_quote ? '\'' : '"';
  const size_t flags_bitset = state->flags_bitset;
  unsigned long codepoint;
  unsigned long high_surrogate = 0;

  if ((Json::parse_flags_allow_location_information & flags_bitset) != 0 &&
      is_key != 0) {
    state->dom_size += sizeof(struct Json::string_ex_s);
  } else {
    state->dom_size += sizeof(struct Json::string_s);
  }

  if ('"' != src[offset]) {
    /* if we are allowed single quoted strings check for that too. */
    if (!((Json::parse_flags_allow_single_quoted_strings & flags_bitset) &&
          is_single_quote)) {
      state->error = Json::parse_error_expected_opening_quote;
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
      state->error = Json::parse_error_invalid_string;
      state->offset = offset;
      return 1;
    }

    if ('\\' == src[offset]) {
      /* skip reverse solidus character. */
      offset++;

      if (offset == size) {
        state->error = Json::parse_error_premature_end_of_buffer;
        state->offset = offset;
        return 1;
      }

      switch (src[offset]) {
      default:
        state->error = Json::parse_error_invalid_string_escape_sequence;
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
          state->error = Json::parse_error_invalid_string_escape_sequence;
          state->offset = offset;
          return 1;
        }

        codepoint = 0;
        if (!Json::hexadecimal_value(&src[offset + 1], 4, &codepoint)) {
          /* escaped unicode sequences must contain 4 hexadecimal digits! */
          state->error = Json::parse_error_invalid_string_escape_sequence;
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
          } else {
            state->error = Json::parse_error_invalid_string_escape_sequence;
            state->offset = offset;
            return 1;
          }
        } else if (codepoint <= 0x7f) {
          data_size += 0;
        } else if (codepoint <= 0x7ff) {
          data_size += 1;
        } else if (codepoint >= 0xd800 &&
                   codepoint <= 0xdbff) { /* high surrogate range. */
          /* The codepoint is the first half of a "utf-16 surrogate pair". so we
           * need the other half for it to be valid: \uHHHH\uLLLL. */
          if (offset + 11 > size || '\\' != src[offset + 5] ||
              'u' != src[offset + 6]) {
            state->error = Json::parse_error_invalid_string_escape_sequence;
            state->offset = offset;
            return 1;
          }
          high_surrogate = codepoint;
        } else if (codepoint >= 0xd800 &&
                   codepoint <= 0xdfff) { /* low surrogate range. */
          /* we did not read the other half before. */
          state->error = Json::parse_error_invalid_string_escape_sequence;
          state->offset = offset;
          return 1;
        } else {
          data_size += 2;
        }
        /* escaped codepoints after 0xffff are supported in json through utf-16
         * surrogate pairs: \uD83D\uDD25 for U+1F525. */

        offset += 5;
        break;
      }
    } else if (('\r' == src[offset]) || ('\n' == src[offset])) {
      if (!(Json::parse_flags_allow_multi_line_strings & flags_bitset)) {
        /* invalid escaped unicode sequence! */
        state->error = Json::parse_error_invalid_string_escape_sequence;
        state->offset = offset;
        return 1;
      }

      offset++;
    } else {
      /* skip character (valid part of sequence). */
      offset++;
    }
  }

  /* If the offset is equal to the size, we had a non-terminated string! */
  if (offset == size) {
    state->error = Json::parse_error_premature_end_of_buffer;
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

int Json::is_valid_unquoted_key_char(const char c) {
  return (('0' <= c && c <= '9') || ('a' <= c && c <= 'z') ||
    ('A' <= c && c <= 'Z') || ('_' == c));
}

int Json::get_key_size(struct Json::parse_state_s *state) {
  const size_t flags_bitset = state->flags_bitset;

  if (Json::parse_flags_allow_unquoted_keys & flags_bitset) {
    size_t offset = state->offset;
    const size_t size = state->size;
    const char *const src = state->src;
    size_t data_size = state->data_size;

    /* if we are allowing unquoted keys, first grok for a quote... */
    if ('"' == src[offset]) {
      /* ... if we got a comma, just parse the key as a string as normal. */
      return Json::get_string_size(state, 1);
    } else if ((Json::parse_flags_allow_single_quoted_strings & flags_bitset) &&
               ('\'' == src[offset])) {
      /* ... if we got a comma, just parse the key as a string as normal. */
      return Json::get_string_size(state, 1);
    } else {
      while ((offset < size) && is_valid_unquoted_key_char(src[offset])) {
        offset++;
        data_size++;
      }

      /* one more byte for null terminator ending the string! */
      data_size++;

      if (Json::parse_flags_allow_location_information & flags_bitset) {
        state->dom_size += sizeof(struct Json::string_ex_s);
      } else {
        state->dom_size += sizeof(struct Json::string_s);
      }

      /* update offset. */
      state->offset = offset;

      /* update data_size. */
      state->data_size = data_size;

      return 0;
    }
  } else {
    /* we are only allowed to have quoted keys, so just parse a string! */
    return Json::get_string_size(state, 1);
  }
}

int Json::get_object_size(struct Json::parse_state_s *state,
                         int is_global_object) {
  const size_t flags_bitset = state->flags_bitset;
  const char *const src = state->src;
  const size_t size = state->size;
  size_t elements = 0;
  int allow_comma = 0;
  int found_closing_brace = 0;

  if (is_global_object) {
    /* if we found an opening '{' of an object, we actually have a normal JSON
     * object at the root of the DOM... */
    if (!Json::skip_all_skippables(state) && '{' == state->src[state->offset]) {
      /* . and we don't actually have a global object after all! */
      is_global_object = 0;
    }
  }

  if (!is_global_object) {
    if ('{' != src[state->offset]) {
      state->error = Json::parse_error_unknown;
      return 1;
    }

    /* skip leading '{'. */
    state->offset++;
  }

  state->dom_size += sizeof(struct Json::object_s);

  if ((state->offset == size) && !is_global_object) {
    state->error = Json::parse_error_premature_end_of_buffer;
    return 1;
  }

  do {
    if (!is_global_object) {
      if (Json::skip_all_skippables(state)) {
        state->error = Json::parse_error_premature_end_of_buffer;
        return 1;
      }

      if ('}' == src[state->offset]) {
        /* skip trailing '}'. */
        state->offset++;

        found_closing_brace = 1;

        /* finished the object! */
        break;
      }
    } else {
      /* we don't require brackets, so that means the object ends when the input
       * stream ends! */
      if (Json::skip_all_skippables(state)) {
        break;
      }
    }

    /* if we parsed at least once element previously, grok for a comma. */
    if (allow_comma) {
      if (',' == src[state->offset]) {
        /* skip comma. */
        state->offset++;
        allow_comma = 0;
      } else if (Json::parse_flags_allow_no_commas & flags_bitset) {
        /* we don't require a comma, and we didn't find one, which is ok! */
        allow_comma = 0;
      } else {
        /* otherwise we are required to have a comma, and we found none. */
        state->error = Json::parse_error_expected_comma_or_closing_bracket;
        return 1;
      }

      if (Json::parse_flags_allow_trailing_comma & flags_bitset) {
        continue;
      } else {
        if (Json::skip_all_skippables(state)) {
          state->error = Json::parse_error_premature_end_of_buffer;
          return 1;
        }
      }
    }

    if (Json::get_key_size(state)) {
      /* key parsing failed! */
      state->error = Json::parse_error_invalid_string;
      return 1;
    }

    if (Json::skip_all_skippables(state)) {
      state->error = Json::parse_error_premature_end_of_buffer;
      return 1;
    }

    if (Json::parse_flags_allow_equals_in_object & flags_bitset) {
      const char current = src[state->offset];
      if ((':' != current) && ('=' != current)) {
        state->error = Json::parse_error_expected_colon;
        return 1;
      }
    } else {
      if (':' != src[state->offset]) {
        state->error = Json::parse_error_expected_colon;
        return 1;
      }
    }

    /* skip colon. */
    state->offset++;

    if (Json::skip_all_skippables(state)) {
      state->error = Json::parse_error_premature_end_of_buffer;
      return 1;
    }

    if (Json::get_value_size(state, /* is_global_object = */ 0)) {
      /* value parsing failed! */
      return 1;
    }

    /* successfully parsed a name/value pair! */
    elements++;
    allow_comma = 1;
  } while (state->offset < size);

  if ((state->offset == size) && !is_global_object && !found_closing_brace) {
    state->error = Json::parse_error_premature_end_of_buffer;
    return 1;
  }

  state->dom_size += sizeof(struct Json::object_element_s) * elements;

  return 0;
}

int Json::get_array_size(struct Json::parse_state_s *state) {
  const size_t flags_bitset = state->flags_bitset;
  size_t elements = 0;
  int allow_comma = 0;
  const char *const src = state->src;
  const size_t size = state->size;

  if ('[' != src[state->offset]) {
    /* expected array to begin with leading '['. */
    state->error = Json::parse_error_unknown;
    return 1;
  }

  /* skip leading '['. */
  state->offset++;

  state->dom_size += sizeof(struct Json::array_s);

  while (state->offset < size) {
    if (Json::skip_all_skippables(state)) {
      state->error = Json::parse_error_premature_end_of_buffer;
      return 1;
    }

    if (']' == src[state->offset]) {
      /* skip trailing ']'. */
      state->offset++;

      state->dom_size += sizeof(struct Json::array_element_s) * elements;

      /* finished the object! */
      return 0;
    }

    /* if we parsed at least once element previously, grok for a comma. */
    if (allow_comma) {
      if (',' == src[state->offset]) {
        /* skip comma. */
        state->offset++;
        allow_comma = 0;
      } else if (!(Json::parse_flags_allow_no_commas & flags_bitset)) {
        state->error = Json::parse_error_expected_comma_or_closing_bracket;
        return 1;
      }

      if (Json::parse_flags_allow_trailing_comma & flags_bitset) {
        allow_comma = 0;
        continue;
      } else {
        if (Json::skip_all_skippables(state)) {
          state->error = Json::parse_error_premature_end_of_buffer;
          return 1;
        }
      }
    }

    if (Json::get_value_size(state, /* is_global_object = */ 0)) {
      /* value parsing failed! */
      return 1;
    }

    /* successfully parsed an array element! */
    elements++;
    allow_comma = 1;
  }

  /* we consumed the entire input before finding the closing ']' of the array!
   */
  state->error = Json::parse_error_premature_end_of_buffer;
  return 1;
}

int Json::get_number_size(struct Json::parse_state_s *state) {
  const size_t flags_bitset = state->flags_bitset;
  size_t offset = state->offset;
  const size_t size = state->size;
  int had_leading_digits = 0;
  const char *const src = state->src;

  state->dom_size += sizeof(struct Json::number_s);

  if ((Json::parse_flags_allow_hexadecimal_numbers & flags_bitset) &&
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
  } else {
    int found_sign = 0;
    int inf_or_nan = 0;

    if ((offset < size) &&
        (('-' == src[offset]) ||
         ((Json::parse_flags_allow_leading_plus_sign & flags_bitset) &&
          ('+' == src[offset])))) {
      /* skip valid leading '-' or '+'. */
      offset++;

      found_sign = 1;
    }

    if (Json::parse_flags_allow_inf_and_nan & flags_bitset) {
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
      if (!(Json::parse_flags_allow_leading_or_trailing_decimal_point &
            flags_bitset) ||
          ('.' != src[offset])) {
        /* a leading '-' must be immediately followed by any digit! */
        state->error = Json::parse_error_invalid_number_format;
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
        state->error = Json::parse_error_invalid_number_format;
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
        if (!(Json::parse_flags_allow_leading_or_trailing_decimal_point &
              flags_bitset) ||
            !had_leading_digits) {
          /* a decimal point must be followed by at least one digit. */
          state->error = Json::parse_error_invalid_number_format;
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
        state->error = Json::parse_error_invalid_number_format;
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
      if (Json::parse_flags_allow_equals_in_object & flags_bitset) {
        break;
      }

      state->error = Json::parse_error_invalid_number_format;
      state->offset = offset;
      return 1;
    default:
      state->error = Json::parse_error_invalid_number_format;
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

int Json::get_value_size(struct Json::parse_state_s *state,
                        int is_global_object) {
  const size_t flags_bitset = state->flags_bitset;
  const char *const src = state->src;
  size_t offset;
  const size_t size = state->size;

  if (Json::parse_flags_allow_location_information & flags_bitset) {
    state->dom_size += sizeof(struct Json::value_ex_s);
  } else {
    state->dom_size += sizeof(struct Json::value_s);
  }

  if (is_global_object) {
    return Json::get_object_size(state, /* is_global_object = */ 1);
  } else {
    if (Json::skip_all_skippables(state)) {
      state->error = Json::parse_error_premature_end_of_buffer;
      return 1;
    }

    /* can cache offset now. */
    offset = state->offset;

    switch (src[offset]) {
    case '"':
      return Json::get_string_size(state, 0);
    case '\'':
      if (Json::parse_flags_allow_single_quoted_strings & flags_bitset) {
        return Json::get_string_size(state, 0);
      } else {
        /* invalid value! */
        state->error = Json::parse_error_invalid_value;
        return 1;
      }
    case '{':
      return Json::get_object_size(state, /* is_global_object = */ 0);
    case '[':
      return Json::get_array_size(state);
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
      return Json::get_number_size(state);
    case '+':
      if (Json::parse_flags_allow_leading_plus_sign & flags_bitset) {
        return Json::get_number_size(state);
      } else {
        /* invalid value! */
        state->error = Json::parse_error_invalid_number_format;
        return 1;
      }
    case '.':
      if (Json::parse_flags_allow_leading_or_trailing_decimal_point &
          flags_bitset) {
        return Json::get_number_size(state);
      } else {
        /* invalid value! */
        state->error = Json::parse_error_invalid_number_format;
        return 1;
      }
    default:
      if ((offset + 4) <= size && 't' == src[offset + 0] &&
          'r' == src[offset + 1] && 'u' == src[offset + 2] &&
          'e' == src[offset + 3]) {
        state->offset += 4;
        return 0;
      } else if ((offset + 5) <= size && 'f' == src[offset + 0] &&
                 'a' == src[offset + 1] && 'l' == src[offset + 2] &&
                 's' == src[offset + 3] && 'e' == src[offset + 4]) {
        state->offset += 5;
        return 0;
      } else if ((offset + 4) <= size && 'n' == state->src[offset + 0] &&
                 'u' == state->src[offset + 1] &&
                 'l' == state->src[offset + 2] &&
                 'l' == state->src[offset + 3]) {
        state->offset += 4;
        return 0;
      } else if ((Json::parse_flags_allow_inf_and_nan & flags_bitset) &&
                 (offset + 3) <= size && 'N' == src[offset + 0] &&
                 'a' == src[offset + 1] && 'N' == src[offset + 2]) {
        return Json::get_number_size(state);
      } else if ((Json::parse_flags_allow_inf_and_nan & flags_bitset) &&
                 (offset + 8) <= size && 'I' == src[offset + 0] &&
                 'n' == src[offset + 1] && 'f' == src[offset + 2] &&
                 'i' == src[offset + 3] && 'n' == src[offset + 4] &&
                 'i' == src[offset + 5] && 't' == src[offset + 6] &&
                 'y' == src[offset + 7]) {
        return Json::get_number_size(state);
      }

      /* invalid value! */
      state->error = Json::parse_error_invalid_value;
      return 1;
    }
  }
}

void Json::parse_string(struct Json::parse_state_s *state,
                       struct Json::string_s *string) {
  size_t offset = state->offset;
  size_t bytes_written = 0;
  const char *const src = state->src;
  const char quote_to_use = '\'' == src[offset] ? '\'' : '"';
  char *data = state->data;
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
        if (!Json::hexadecimal_value(&src[offset], 4, &codepoint)) {
          return; /* this shouldn't happen as the value was already validated.
                   */
        }

        offset += 4;

        if (codepoint <= 0x7fu) {
          data[bytes_written++] = (char)codepoint; /* 0xxxxxxx. */
        } else if (codepoint <= 0x7ffu) {
          data[bytes_written++] =
              (char)(0xc0u | (codepoint >> 6)); /* 110xxxxx. */
          data[bytes_written++] =
              (char)(0x80u | (codepoint & 0x3fu)); /* 10xxxxxx. */
        } else if (codepoint >= 0xd800 &&
                   codepoint <= 0xdbff) { /* high surrogate. */
          high_surrogate = codepoint;
          continue; /* we need the low half to form a complete codepoint. */
        } else if (codepoint >= 0xdc00 &&
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
        } else {
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
    } else {
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

void Json::parse_key(struct Json::parse_state_s *state,
                    struct Json::string_s *string) {
  if (Json::parse_flags_allow_unquoted_keys & state->flags_bitset) {
    const char *const src = state->src;
    char *const data = state->data;
    size_t offset = state->offset;

    /* if we are allowing unquoted keys, check for quoted anyway... */
    if (('"' == src[offset]) || ('\'' == src[offset])) {
      /* ... if we got a quote, just parse the key as a string as normal. */
      Json::parse_string(state, string);
    } else {
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
  } else {
    /* we are only allowed to have quoted keys, so just parse a string! */
    Json::parse_string(state, string);
  }
}

void Json::parse_object(struct Json::parse_state_s *state, int is_global_object,
                       struct Json::object_s *object) {
  const size_t flags_bitset = state->flags_bitset;
  const size_t size = state->size;
  const char *const src = state->src;
  size_t elements = 0;
  int allow_comma = 0;
  struct Json::object_element_s *previous = nullptr;

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

  (void)Json::skip_all_skippables(state);

  /* reset elements. */
  elements = 0;

  while (state->offset < size) {
    struct Json::object_element_s *element = nullptr;
    struct Json::string_s *string = nullptr;
    struct Json::value_s *value = nullptr;

    if (!is_global_object) {
      (void)Json::skip_all_skippables(state);

      if ('}' == src[state->offset]) {
        /* skip trailing '}'. */
        state->offset++;

        /* finished the object! */
        break;
      }
    } else {
      if (Json::skip_all_skippables(state)) {
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

    element = (struct Json::object_element_s *)state->dom;

    state->dom += sizeof(struct Json::object_element_s);

    if (nullptr == previous) {
      /* this is our first element, so record it in our object. */
      object->start = element;
    } else {
      previous->next = element;
    }

    previous = element;

    if (Json::parse_flags_allow_location_information & flags_bitset) {
      struct Json::string_ex_s *string_ex =
          (struct Json::string_ex_s *)state->dom;
      state->dom += sizeof(struct Json::string_ex_s);

      string_ex->offset = state->offset;
      string_ex->line_no = state->line_no;
      string_ex->row_no = state->offset - state->line_offset;

      string = &(string_ex->string);
    } else {
      string = (struct Json::string_s *)state->dom;
      state->dom += sizeof(struct Json::string_s);
    }

    element->name = string;

    (void)Json::parse_key(state, string);

    (void)Json::skip_all_skippables(state);

    /* skip colon or equals. */
    state->offset++;

    (void)Json::skip_all_skippables(state);

    if (Json::parse_flags_allow_location_information & flags_bitset) {
      struct Json::value_ex_s *value_ex = (struct Json::value_ex_s *)state->dom;
      state->dom += sizeof(struct Json::value_ex_s);

      value_ex->offset = state->offset;
      value_ex->line_no = state->line_no;
      value_ex->row_no = state->offset - state->line_offset;

      value = &(value_ex->value);
    } else {
      value = (struct Json::value_s *)state->dom;
      state->dom += sizeof(struct Json::value_s);
    }

    element->value = value;

    Json::parse_value(state, /* is_global_object = */ 0, value);

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

void Json::parse_array(struct Json::parse_state_s *state,
                      struct Json::array_s *array) {
  const char *const src = state->src;
  const size_t size = state->size;
  size_t elements = 0;
  int allow_comma = 0;
  struct Json::array_element_s *previous = nullptr;

  /* skip leading '['. */
  state->offset++;

  (void)Json::skip_all_skippables(state);

  /* reset elements. */
  elements = 0;

  do {
    struct Json::array_element_s *element = nullptr;
    struct Json::value_s *value = nullptr;

    (void)Json::skip_all_skippables(state);

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

    element = (struct Json::array_element_s *)state->dom;

    state->dom += sizeof(struct Json::array_element_s);

    if (nullptr == previous) {
      /* this is our first element, so record it in our array. */
      array->start = element;
    } else {
      previous->next = element;
    }

    previous = element;

    if (Json::parse_flags_allow_location_information & state->flags_bitset) {
      struct Json::value_ex_s *value_ex = (struct Json::value_ex_s *)state->dom;
      state->dom += sizeof(struct Json::value_ex_s);

      value_ex->offset = state->offset;
      value_ex->line_no = state->line_no;
      value_ex->row_no = state->offset - state->line_offset;

      value = &(value_ex->value);
    } else {
      value = (struct Json::value_s *)state->dom;
      state->dom += sizeof(struct Json::value_s);
    }

    element->value = value;

    Json::parse_value(state, /* is_global_object = */ 0, value);

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

void Json::parse_number(struct Json::parse_state_s *state,
                       struct Json::number_s *number) {
  const size_t flags_bitset = state->flags_bitset;
  size_t offset = state->offset;
  const size_t size = state->size;
  size_t bytes_written = 0;
  const char *const src = state->src;
  char *data = state->data;

  number->number = data;

  if (Json::parse_flags_allow_hexadecimal_numbers & flags_bitset) {
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

  if (Json::parse_flags_allow_inf_and_nan & flags_bitset) {
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

void Json::parse_value(struct Json::parse_state_s *state, int is_global_object,
                      struct Json::value_s *value) {
  const size_t flags_bitset = state->flags_bitset;
  const char *const src = state->src;
  const size_t size = state->size;
  size_t offset;

  (void)Json::skip_all_skippables(state);

  /* cache offset now. */
  offset = state->offset;

  if (is_global_object) {
    value->type = Json::type_object;
    value->payload = state->dom;
    state->dom += sizeof(struct Json::object_s);
    Json::parse_object(state, /* is_global_object = */ 1,
                      (struct Json::object_s *)value->payload);
  } else {
    switch (src[offset]) {
    case '"':
    case '\'':
      value->type = Json::type_string;
      value->payload = state->dom;
      state->dom += sizeof(struct Json::string_s);
      Json::parse_string(state, (struct Json::string_s *)value->payload);
      break;
    case '{':
      value->type = Json::type_object;
      value->payload = state->dom;
      state->dom += sizeof(struct Json::object_s);
      Json::parse_object(state, /* is_global_object = */ 0,
                        (struct Json::object_s *)value->payload);
      break;
    case '[':
      value->type = Json::type_array;
      value->payload = state->dom;
      state->dom += sizeof(struct Json::array_s);
      Json::parse_array(state, (struct Json::array_s *)value->payload);
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
      state->dom += sizeof(struct Json::number_s);
      Json::parse_number(state, (struct Json::number_s *)value->payload);
      break;
    default:
      if ((offset + 4) <= size && 't' == src[offset + 0] &&
          'r' == src[offset + 1] && 'u' == src[offset + 2] &&
          'e' == src[offset + 3]) {
        value->type = Json::type_true;
        value->payload = nullptr;
        state->offset += 4;
      } else if ((offset + 5) <= size && 'f' == src[offset + 0] &&
                 'a' == src[offset + 1] && 'l' == src[offset + 2] &&
                 's' == src[offset + 3] && 'e' == src[offset + 4]) {
        value->type = Json::type_false;
        value->payload = nullptr;
        state->offset += 5;
      } else if ((offset + 4) <= size && 'n' == src[offset + 0] &&
                 'u' == src[offset + 1] && 'l' == src[offset + 2] &&
                 'l' == src[offset + 3]) {
        value->type = Json::type_null;
        value->payload = nullptr;
        state->offset += 4;
      } else if ((Json::parse_flags_allow_inf_and_nan & flags_bitset) &&
                 (offset + 3) <= size && 'N' == src[offset + 0] &&
                 'a' == src[offset + 1] && 'N' == src[offset + 2]) {
        value->type = Json::type_number;
        value->payload = state->dom;
        state->dom += sizeof(struct Json::number_s);
        Json::parse_number(state, (struct Json::number_s *)value->payload);
      } else if ((Json::parse_flags_allow_inf_and_nan & flags_bitset) &&
                 (offset + 8) <= size && 'I' == src[offset + 0] &&
                 'n' == src[offset + 1] && 'f' == src[offset + 2] &&
                 'i' == src[offset + 3] && 'n' == src[offset + 4] &&
                 'i' == src[offset + 5] && 't' == src[offset + 6] &&
                 'y' == src[offset + 7]) {
        value->type = Json::type_number;
        value->payload = state->dom;
        state->dom += sizeof(struct Json::number_s);
        Json::parse_number(state, (struct Json::number_s *)value->payload);
      }
      break;
    }
  }
}

struct Json::value_s *
Json::parse_ex(const void *src, size_t src_size, size_t flags_bitset,
              void *(*alloc_func_ptr)(void *user_data, size_t size),
              void *user_data, struct Json::parse_result_s *result) {
  struct Json::parse_state_s state;
  void *allocation;
  struct Json::value_s *value;
  size_t total_size;
  int input_error;

  if (result) {
    result->error = Json::parse_error_none;
    result->error_offset = 0;
    result->error_line_no = 0;
    result->error_row_no = 0;
  }

  if (nullptr == src) {
    /* invalid src pointer was null! */
    return nullptr;
  }

  state.src = (const char *)src;
  state.size = src_size;
  state.offset = 0;
  state.line_no = 1;
  state.line_offset = 0;
  state.error = Json::parse_error_none;
  state.dom_size = 0;
  state.data_size = 0;
  state.flags_bitset = flags_bitset;

  input_error = Json::get_value_size(
      &state, (int)(Json::parse_flags_allow_global_object & state.flags_bitset));

  if (0 == input_error) {
    Json::skip_all_skippables(&state);

    if (state.offset != state.size) {
      /* our parsing didn't have an error, but there are characters remaining in
       * the input that weren't part of the JSON! */

      state.error = Json::parse_error_unexpected_trailing_characters;
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
  } else {
    allocation = alloc_func_ptr(user_data, total_size);
  }

  if (nullptr == allocation) {
    /* malloc failed! */
    if (result) {
      result->error = Json::parse_error_allocator_failed;
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

  state.dom = (char *)allocation;
  state.data = state.dom + state.dom_size;

  if (Json::parse_flags_allow_location_information & state.flags_bitset) {
    struct Json::value_ex_s *value_ex = (struct Json::value_ex_s *)state.dom;
    state.dom += sizeof(struct Json::value_ex_s);

    value_ex->offset = state.offset;
    value_ex->line_no = state.line_no;
    value_ex->row_no = state.offset - state.line_offset;

    value = &(value_ex->value);
  } else {
    value = (struct Json::value_s *)state.dom;
    state.dom += sizeof(struct Json::value_s);
  }

  Json::parse_value(
      &state, (int)(Json::parse_flags_allow_global_object & state.flags_bitset),
      value);

  return (struct Json::value_s *)allocation;
}

struct Json::value_s *Json::parse(const void *src, size_t src_size) {
  return Json::parse_ex(src, src_size, Json::parse_flags_default, nullptr,
                       nullptr, nullptr);
}

struct Json::extract_result_s {
  size_t dom_size;
  size_t data_size;
};

struct Json::value_s *Json::extract_value(const struct Json::value_s *value) {
  return Json::extract_value_ex(value, nullptr, nullptr);
}

struct Json::extract_result_s
Json::extract_get_number_size(const struct Json::number_s *const number) {
  struct Json::extract_result_s result;
  result.dom_size = sizeof(struct Json::number_s);
  result.data_size = number->number_size;
  return result;
}

struct Json::extract_result_s
Json::extract_get_string_size(const struct Json::string_s *const string) {
  struct Json::extract_result_s result;
  result.dom_size = sizeof(struct Json::string_s);
  result.data_size = string->string_size + 1;
  return result;
}

struct Json::extract_result_s
Json::extract_get_object_size(const struct Json::object_s *const object) {
  struct Json::extract_result_s result;
  size_t i;
  const struct Json::object_element_s *element = object->start;

  result.dom_size = sizeof(struct Json::object_s) +
                    (sizeof(struct Json::object_element_s) * object->length);
  result.data_size = 0;

  for (i = 0; i < object->length; i++) {
    const struct Json::extract_result_s string_result =
        Json::extract_get_string_size(element->name);
    const struct Json::extract_result_s value_result =
        Json::extract_get_value_size(element->value);

    result.dom_size += string_result.dom_size;
    result.data_size += string_result.data_size;

    result.dom_size += value_result.dom_size;
    result.data_size += value_result.data_size;

    element = element->next;
  }

  return result;
}

struct Json::extract_result_s
Json::extract_get_array_size(const struct Json::array_s *const array) {
  struct Json::extract_result_s result;
  size_t i;
  const struct Json::array_element_s *element = array->start;

  result.dom_size = sizeof(struct Json::array_s) +
                    (sizeof(struct Json::array_element_s) * array->length);
  result.data_size = 0;

  for (i = 0; i < array->length; i++) {
    const struct Json::extract_result_s value_result =
        Json::extract_get_value_size(element->value);

    result.dom_size += value_result.dom_size;
    result.data_size += value_result.data_size;

    element = element->next;
  }

  return result;
}

struct Json::extract_result_s
Json::extract_get_value_size(const struct Json::value_s *const value) {
  struct Json::extract_result_s result = {0, 0};

  switch (value->type) {
  default:
    break;
  case Json::type_object:
    result = Json::extract_get_object_size(
        (const struct Json::object_s *)value->payload);
    break;
  case Json::type_array:
    result = Json::extract_get_array_size(
        (const struct Json::array_s *)value->payload);
    break;
  case Json::type_number:
    result = Json::extract_get_number_size(
        (const struct Json::number_s *)value->payload);
    break;
  case Json::type_string:
    result = Json::extract_get_string_size(
        (const struct Json::string_s *)value->payload);
    break;
  }

  result.dom_size += sizeof(struct Json::value_s);

  return result;
}

struct Json::extract_state_s {
  char *dom;
  char *data;
};

void Json::extract_copy_value(struct Json::extract_state_s *const state,
                             const struct Json::value_s *const value) {
  struct Json::string_s *string;
  struct Json::number_s *number;
  struct Json::object_s *object;
  struct Json::array_s *array;
  struct Json::value_s *new_value;

  memcpy(state->dom, value, sizeof(struct Json::value_s));
  new_value = (struct Json::value_s *)state->dom;
  state->dom += sizeof(struct Json::value_s);
  new_value->payload = state->dom;

  if (Json::type_string == value->type) {
    memcpy(state->dom, value->payload, sizeof(struct Json::string_s));
    string = (struct Json::string_s *)state->dom;
    state->dom += sizeof(struct Json::string_s);

    memcpy(state->data, string->string, string->string_size + 1);
    string->string = state->data;
    state->data += string->string_size + 1;
  } else if (Json::type_number == value->type) {
    memcpy(state->dom, value->payload, sizeof(struct Json::number_s));
    number = (struct Json::number_s *)state->dom;
    state->dom += sizeof(struct Json::number_s);

    memcpy(state->data, number->number, number->number_size);
    number->number = state->data;
    state->data += number->number_size;
  } else if (Json::type_object == value->type) {
    struct Json::object_element_s *element;
    size_t i;

    memcpy(state->dom, value->payload, sizeof(struct Json::object_s));
    object = (struct Json::object_s *)state->dom;
    state->dom += sizeof(struct Json::object_s);

    element = object->start;
    object->start = (struct Json::object_element_s *)state->dom;

    for (i = 0; i < object->length; i++) {
      struct Json::value_s *previous_value;
      struct Json::object_element_s *previous_element;

      memcpy(state->dom, element, sizeof(struct Json::object_element_s));
      element = (struct Json::object_element_s *)state->dom;
      state->dom += sizeof(struct Json::object_element_s);

      string = element->name;
      memcpy(state->dom, string, sizeof(struct Json::string_s));
      string = (struct Json::string_s *)state->dom;
      state->dom += sizeof(struct Json::string_s);
      element->name = string;

      memcpy(state->data, string->string, string->string_size + 1);
      string->string = state->data;
      state->data += string->string_size + 1;

      previous_value = element->value;
      element->value = (struct Json::value_s *)state->dom;
      Json::extract_copy_value(state, previous_value);

      previous_element = element;
      element = element->next;

      if (element) {
        previous_element->next = (struct Json::object_element_s *)state->dom;
      }
    }
  } else if (Json::type_array == value->type) {
    struct Json::array_element_s *element;
    size_t i;

    memcpy(state->dom, value->payload, sizeof(struct Json::array_s));
    array = (struct Json::array_s *)state->dom;
    state->dom += sizeof(struct Json::array_s);

    element = array->start;
    array->start = (struct Json::array_element_s *)state->dom;

    for (i = 0; i < array->length; i++) {
      struct Json::value_s *previous_value;
      struct Json::array_element_s *previous_element;

      memcpy(state->dom, element, sizeof(struct Json::array_element_s));
      element = (struct Json::array_element_s *)state->dom;
      state->dom += sizeof(struct Json::array_element_s);

      previous_value = element->value;
      element->value = (struct Json::value_s *)state->dom;
      Json::extract_copy_value(state, previous_value);

      previous_element = element;
      element = element->next;

      if (element) {
        previous_element->next = (struct Json::array_element_s *)state->dom;
      }
    }
  }
}

struct Json::value_s *Json::extract_value_ex(const struct Json::value_s *value,
                                           void *(*alloc_func_ptr)(void *,
                                                                   size_t),
                                           void *user_data) {
  void *allocation;
  struct Json::extract_result_s result;
  struct Json::extract_state_s state;
  size_t total_size;

  if (nullptr == value) {
    /* invalid value was null! */
    return nullptr;
  }

  result = Json::extract_get_value_size(value);
  total_size = result.dom_size + result.data_size;

  if (nullptr == alloc_func_ptr) {
    allocation = malloc(total_size);
  } else {
    allocation = alloc_func_ptr(user_data, total_size);
  }

  state.dom = (char *)allocation;
  state.data = state.dom + result.dom_size;

  Json::extract_copy_value(&state, value);

  return (struct Json::value_s *)allocation;
}

struct Json::string_s *Json::value_as_string(struct Json::value_s *const value) {
  if (value->type != Json::type_string) {
    return nullptr;
  }

  return (struct Json::string_s *)value->payload;
}

struct Json::number_s *Json::value_as_number(struct Json::value_s *const value) {
  if (value->type != Json::type_number) {
    return nullptr;
  }

  return (struct Json::number_s *)value->payload;
}

struct Json::object_s *Json::value_as_object(struct Json::value_s *const value) {
  if (value->type != Json::type_object) {
    return nullptr;
  }

  return (struct Json::object_s *)value->payload;
}

struct Json::array_s *Json::value_as_array(struct Json::value_s *const value) {
  if (value->type != Json::type_array) {
    return nullptr;
  }

  return (struct Json::array_s *)value->payload;
}

int Json::value_is_true(const struct Json::value_s *const value) {
  return value->type == Json::type_true;
}

int Json::value_is_false(const struct Json::value_s *const value) {
  return value->type == Json::type_false;
}

int Json::value_is_null(const struct Json::value_s *const value) {
  return value->type == Json::type_null;
}

int Json::write_get_number_size(const struct Json::number_s *number,
                               size_t *size) {
  uintmax_t parsed_number;
  size_t i;

  if (number->number_size >= 2) {
    switch (number->number[1]) {
    default:
      break;
    case 'x':
    case 'X':
      /* the number is a Json::parse_flags_allow_hexadecimal_numbers hexadecimal
       * so we have to do extra work to convert it to a non-hexadecimal for JSON
       * output. */
      parsed_number = _strtoui64(number->number, nullptr, 0);

      i = 0;

      while (0 != parsed_number) {
        parsed_number /= 10;
        i++;
      }

      *size += i;
      return 0;
    }
  }

  /* check to see if the number has leading/trailing decimal point. */
  i = 0;

  /* skip any leading '+' or '-'. */
  if ((i < number->number_size) &&
      (('+' == number->number[i]) || ('-' == number->number[i]))) {
    i++;
  }

  /* check if we have infinity. */
  if ((i < number->number_size) && ('I' == number->number[i])) {
    const char *inf = "Infinity";
    size_t k;

    for (k = i; k < number->number_size; k++) {
      const char c = *inf++;

      /* Check if we found the Infinity string! */
      if ('\0' == c) {
        break;
      } else if (c != number->number[k]) {
        break;
      }
    }

    if ('\0' == *inf) {
      /* Inf becomes 1.7976931348623158e308 because JSON can't support it. */
      *size += 22;

      /* if we had a leading '-' we need to record it in the JSON output. */
      if ('-' == number->number[0]) {
        *size += 1;
      }
    }

    return 0;
  }

  /* check if we have nan. */
  if ((i < number->number_size) && ('N' == number->number[i])) {
    const char *nan = "NaN";
    size_t k;

    for (k = i; k < number->number_size; k++) {
      const char c = *nan++;

      /* Check if we found the NaN string! */
      if ('\0' == c) {
        break;
      } else if (c != number->number[k]) {
        break;
      }
    }

    if ('\0' == *nan) {
      /* NaN becomes 1 because JSON can't support it. */
      *size += 1;

      return 0;
    }
  }

  /* if we had a leading decimal point. */
  if ((i < number->number_size) && ('.' == number->number[i])) {
    /* 1 + because we had a leading decimal point. */
    *size += 1;
    goto cleanup;
  }

  for (; i < number->number_size; i++) {
    const char c = number->number[i];
    if (!('0' <= c && c <= '9')) {
      break;
    }
  }

  /* if we had a trailing decimal point. */
  if ((i + 1 == number->number_size) && ('.' == number->number[i])) {
    /* 1 + because we had a trailing decimal point. */
    *size += 1;
    goto cleanup;
  }

cleanup:
  *size += number->number_size; /* the actual string of the number. */

  /* if we had a leading '+' we don't record it in the JSON output. */
  if ('+' == number->number[0]) {
    *size -= 1;
  }

  return 0;
}

int Json::write_get_string_size(const struct Json::string_s *string,
                               size_t *size) {
  size_t i;
  for (i = 0; i < string->string_size; i++) {
    switch (string->string[i]) {
    case '"':
    case '\\':
    case '\b':
    case '\f':
    case '\n':
    case '\r':
    case '\t':
      *size += 2;
      break;
    default:
      *size += 1;
      break;
    }
  }

  *size += 2; /* need to encode the surrounding '"' characters. */

  return 0;
}

int Json::write_minified_get_array_size(const struct Json::array_s *array,
                                       size_t *size) {
  struct Json::array_element_s *element;

  *size += 2; /* '[' and ']'. */

  if (1 < array->length) {
    *size += array->length - 1; /* ','s seperate each element. */
  }

  for (element = array->start; nullptr != element; element = element->next) {
    if (Json::write_minified_get_value_size(element->value, size)) {
      /* value was malformed! */
      return 1;
    }
  }

  return 0;
}

int Json::write_minified_get_object_size(const struct Json::object_s *object,
                                        size_t *size) {
  struct Json::object_element_s *element;

  *size += 2; /* '{' and '}'. */

  *size += object->length; /* ':'s seperate each name/value pair. */

  if (1 < object->length) {
    *size += object->length - 1; /* ','s seperate each element. */
  }

  for (element = object->start; nullptr != element; element = element->next) {
    if (Json::write_get_string_size(element->name, size)) {
      /* string was malformed! */
      return 1;
    }

    if (Json::write_minified_get_value_size(element->value, size)) {
      /* value was malformed! */
      return 1;
    }
  }

  return 0;
}

int Json::write_minified_get_value_size(const struct Json::value_s *value,
                                       size_t *size) {
  switch (value->type) {
  default:
    /* unknown value type found! */
    return 1;
  case Json::type_number:
    return Json::write_get_number_size((struct Json::number_s *)value->payload,
                                      size);
  case Json::type_string:
    return Json::write_get_string_size((struct Json::string_s *)value->payload,
                                      size);
  case Json::type_array:
    return Json::write_minified_get_array_size(
        (struct Json::array_s *)value->payload, size);
  case Json::type_object:
    return Json::write_minified_get_object_size(
        (struct Json::object_s *)value->payload, size);
  case Json::type_true:
    *size += 4; /* the string "true". */
    return 0;
  case Json::type_false:
    *size += 5; /* the string "false". */
    return 0;
  case Json::type_null:
    *size += 4; /* the string "null". */
    return 0;
  }
}

char *Json::write_number(const struct Json::number_s *number, char *data) {
  uintmax_t parsed_number, backup;
  size_t i;

  if (number->number_size >= 2) {
    switch (number->number[1]) {
    default:
      break;
    case 'x':
    case 'X':
      /* The number is a Json::parse_flags_allow_hexadecimal_numbers hexadecimal
       * so we have to do extra work to convert it to a non-hexadecimal for JSON
       * output. */
      parsed_number = _strtoui64(number->number, nullptr, 0);

      /* We need a copy of parsed number twice, so take a backup of it. */
      backup = parsed_number;

      i = 0;

      while (0 != parsed_number) {
        parsed_number /= 10;
        i++;
      }

      /* Restore parsed_number to its original value stored in the backup. */
      parsed_number = backup;

      /* Now use backup to take a copy of i, or the length of the string. */
      backup = i;

      do {
        *(data + i - 1) = '0' + (char)(parsed_number % 10);
        parsed_number /= 10;
        i--;
      } while (0 != parsed_number);

      data += backup;

      return data;
    }
  }

  /* check to see if the number has leading/trailing decimal point. */
  i = 0;

  /* skip any leading '-'. */
  if ((i < number->number_size) &&
      (('+' == number->number[i]) || ('-' == number->number[i]))) {
    i++;
  }

  /* check if we have infinity. */
  if ((i < number->number_size) && ('I' == number->number[i])) {
    const char *inf = "Infinity";
    size_t k;

    for (k = i; k < number->number_size; k++) {
      const char c = *inf++;

      /* Check if we found the Infinity string! */
      if ('\0' == c) {
        break;
      } else if (c != number->number[k]) {
        break;
      }
    }

    if ('\0' == *inf++) {
      const char *dbl_max;

      /* if we had a leading '-' we need to record it in the JSON output. */
      if ('-' == number->number[0]) {
        *data++ = '-';
      }

      /* Inf becomes 1.7976931348623158e308 because JSON can't support it. */
      for (dbl_max = "1.7976931348623158e308"; '\0' != *dbl_max; dbl_max++) {
        *data++ = *dbl_max;
      }

      return data;
    }
  }

  /* check if we have nan. */
  if ((i < number->number_size) && ('N' == number->number[i])) {
    const char *nan = "NaN";
    size_t k;

    for (k = i; k < number->number_size; k++) {
      const char c = *nan++;

      /* Check if we found the NaN string! */
      if ('\0' == c) {
        break;
      } else if (c != number->number[k]) {
        break;
      }
    }

    if ('\0' == *nan++) {
      /* NaN becomes 0 because JSON can't support it. */
      *data++ = '0';
      return data;
    }
  }

  /* if we had a leading decimal point. */
  if ((i < number->number_size) && ('.' == number->number[i])) {
    i = 0;

    /* skip any leading '+'. */
    if ('+' == number->number[i]) {
      i++;
    }

    /* output the leading '-' if we had one. */
    if ('-' == number->number[i]) {
      *data++ = '-';
      i++;
    }

    /* insert a '0' to fix the leading decimal point for JSON output. */
    *data++ = '0';

    /* and output the rest of the number as normal. */
    for (; i < number->number_size; i++) {
      *data++ = number->number[i];
    }

    return data;
  }

  for (; i < number->number_size; i++) {
    const char c = number->number[i];
    if (!('0' <= c && c <= '9')) {
      break;
    }
  }

  /* if we had a trailing decimal point. */
  if ((i + 1 == number->number_size) && ('.' == number->number[i])) {
    i = 0;

    /* skip any leading '+'. */
    if ('+' == number->number[i]) {
      i++;
    }

    /* output the leading '-' if we had one. */
    if ('-' == number->number[i]) {
      *data++ = '-';
      i++;
    }

    /* and output the rest of the number as normal. */
    for (; i < number->number_size; i++) {
      *data++ = number->number[i];
    }

    /* insert a '0' to fix the trailing decimal point for JSON output. */
    *data++ = '0';

    return data;
  }

  i = 0;

  /* skip any leading '+'. */
  if ('+' == number->number[i]) {
    i++;
  }

  for (; i < number->number_size; i++) {
    *data++ = number->number[i];
  }

  return data;
}

char *Json::write_string(const struct Json::string_s *string, char *data) {
  size_t i;

  *data++ = '"'; /* open the string. */

  for (i = 0; i < string->string_size; i++) {
    switch (string->string[i]) {
    case '"':
      *data++ = '\\'; /* escape the control character. */
      *data++ = '"';
      break;
    case '\\':
      *data++ = '\\'; /* escape the control character. */
      *data++ = '\\';
      break;
    case '\b':
      *data++ = '\\'; /* escape the control character. */
      *data++ = 'b';
      break;
    case '\f':
      *data++ = '\\'; /* escape the control character. */
      *data++ = 'f';
      break;
    case '\n':
      *data++ = '\\'; /* escape the control character. */
      *data++ = 'n';
      break;
    case '\r':
      *data++ = '\\'; /* escape the control character. */
      *data++ = 'r';
      break;
    case '\t':
      *data++ = '\\'; /* escape the control character. */
      *data++ = 't';
      break;
    default:
      *data++ = string->string[i];
      break;
    }
  }

  *data++ = '"'; /* close the string. */

  return data;
}

char *Json::write_minified_array(const struct Json::array_s *array, char *data) {
  struct Json::array_element_s *element = nullptr;

  *data++ = '['; /* open the array. */

  for (element = array->start; nullptr != element; element = element->next) {
    if (element != array->start) {
      *data++ = ','; /* ','s seperate each element. */
    }

    data = Json::write_minified_value(element->value, data);

    if (nullptr == data) {
      /* value was malformed! */
      return nullptr;
    }
  }

  *data++ = ']'; /* close the array. */

  return data;
}

char *Json::write_minified_object(const struct Json::object_s *object,
                                 char *data) {
  struct Json::object_element_s *element = nullptr;

  *data++ = '{'; /* open the object. */

  for (element = object->start; nullptr != element; element = element->next) {
    if (element != object->start) {
      *data++ = ','; /* ','s seperate each element. */
    }

    data = Json::write_string(element->name, data);

    if (nullptr == data) {
      /* string was malformed! */
      return nullptr;
    }

    *data++ = ':'; /* ':'s seperate each name/value pair. */

    data = Json::write_minified_value(element->value, data);

    if (nullptr == data) {
      /* value was malformed! */
      return nullptr;
    }
  }

  *data++ = '}'; /* close the object. */

  return data;
}

char *Json::write_minified_value(const struct Json::value_s *value, char *data) {
  switch (value->type) {
  default:
    /* unknown value type found! */
    return nullptr;
  case Json::type_number:
    return Json::write_number((struct Json::number_s *)value->payload, data);
  case Json::type_string:
    return Json::write_string((struct Json::string_s *)value->payload, data);
  case Json::type_array:
    return Json::write_minified_array((struct Json::array_s *)value->payload,
                                     data);
  case Json::type_object:
    return Json::write_minified_object((struct Json::object_s *)value->payload,
                                      data);
  case Json::type_true:
    data[0] = 't';
    data[1] = 'r';
    data[2] = 'u';
    data[3] = 'e';
    return data + 4;
  case Json::type_false:
    data[0] = 'f';
    data[1] = 'a';
    data[2] = 'l';
    data[3] = 's';
    data[4] = 'e';
    return data + 5;
  case Json::type_null:
    data[0] = 'n';
    data[1] = 'u';
    data[2] = 'l';
    data[3] = 'l';
    return data + 4;
  }
}

void *Json::write_minified(const struct Json::value_s *value, size_t *out_size) {
  size_t size = 0;
  char *data = nullptr;
  char *data_end = nullptr;

  if (nullptr == value) {
    return nullptr;
  }

  if (Json::write_minified_get_value_size(value, &size)) {
    /* value was malformed! */
    return nullptr;
  }

  size += 1; /* for the '\0' null terminating character. */

  data = (char *)malloc(size);

  if (nullptr == data) {
    /* malloc failed! */
    return nullptr;
  }

  data_end = Json::write_minified_value(value, data);

  if (nullptr == data_end) {
    /* bad chi occurred! */
    free(data);
    return nullptr;
  }

  /* null terminated the string. */
  *data_end = '\0';

  if (nullptr != out_size) {
    *out_size = size;
  }

  return data;
}

int Json::write_pretty_get_array_size(const struct Json::array_s *array,
                                     size_t depth, size_t indent_size,
                                     size_t newline_size, size_t *size) {
  struct Json::array_element_s *element;

  *size += 1; /* '['. */

  if (0 < array->length) {
    /* if we have any elements we need to add a newline after our '['. */
    *size += newline_size;

    *size += array->length - 1; /* ','s seperate each element. */

    for (element = array->start; nullptr != element;
         element = element->next) {
      /* each element gets an indent. */
      *size += (depth + 1) * indent_size;

      if (Json::write_pretty_get_value_size(element->value, depth + 1,
                                           indent_size, newline_size, size)) {
        /* value was malformed! */
        return 1;
      }

      /* each element gets a newline too. */
      *size += newline_size;
    }

    /* since we wrote out some elements, need to add a newline and indentation.
     */
    /* to the trailing ']'. */
    *size += depth * indent_size;
  }

  *size += 1; /* ']'. */

  return 0;
}

int Json::write_pretty_get_object_size(const struct Json::object_s *object,
                                      size_t depth, size_t indent_size,
                                      size_t newline_size, size_t *size) {
  struct Json::object_element_s *element;

  *size += 1; /* '{'. */

  if (0 < object->length) {
    *size += newline_size; /* need a newline next. */

    *size += object->length - 1; /* ','s seperate each element. */

    for (element = object->start; nullptr != element;
         element = element->next) {
      /* each element gets an indent and newline. */
      *size += (depth + 1) * indent_size;
      *size += newline_size;

      if (Json::write_get_string_size(element->name, size)) {
        /* string was malformed! */
        return 1;
      }

      *size += 3; /* seperate each name/value pair with " : ". */

      if (Json::write_pretty_get_value_size(element->value, depth + 1,
                                           indent_size, newline_size, size)) {
        /* value was malformed! */
        return 1;
      }
    }

    *size += depth * indent_size;
  }

  *size += 1; /* '}'. */

  return 0;
}

int Json::write_pretty_get_value_size(const struct Json::value_s *value,
                                     size_t depth, size_t indent_size,
                                     size_t newline_size, size_t *size) {
  switch (value->type) {
  default:
    /* unknown value type found! */
    return 1;
  case Json::type_number:
    return Json::write_get_number_size((struct Json::number_s *)value->payload,
                                      size);
  case Json::type_string:
    return Json::write_get_string_size((struct Json::string_s *)value->payload,
                                      size);
  case Json::type_array:
    return Json::write_pretty_get_array_size(
        (struct Json::array_s *)value->payload, depth, indent_size, newline_size,
        size);
  case Json::type_object:
    return Json::write_pretty_get_object_size(
        (struct Json::object_s *)value->payload, depth, indent_size,
        newline_size, size);
  case Json::type_true:
    *size += 4; /* the string "true". */
    return 0;
  case Json::type_false:
    *size += 5; /* the string "false". */
    return 0;
  case Json::type_null:
    *size += 4; /* the string "null". */
    return 0;
  }
}

char *Json::write_pretty_array(const struct Json::array_s *array, size_t depth,
                              const char *indent, const char *newline,
                              char *data) {
  size_t k, m;
  struct Json::array_element_s *element;

  *data++ = '['; /* open the array. */

  if (0 < array->length) {
    for (k = 0; '\0' != newline[k]; k++) {
      *data++ = newline[k];
    }

    for (element = array->start; nullptr != element;
         element = element->next) {
      if (element != array->start) {
        *data++ = ','; /* ','s seperate each element. */

        for (k = 0; '\0' != newline[k]; k++) {
          *data++ = newline[k];
        }
      }

      for (k = 0; k < depth + 1; k++) {
        for (m = 0; '\0' != indent[m]; m++) {
          *data++ = indent[m];
        }
      }

      data = Json::write_pretty_value(element->value, depth + 1, indent, newline,
                                     data);

      if (nullptr == data) {
        /* value was malformed! */
        return nullptr;
      }
    }

    for (k = 0; '\0' != newline[k]; k++) {
      *data++ = newline[k];
    }

    for (k = 0; k < depth; k++) {
      for (m = 0; '\0' != indent[m]; m++) {
        *data++ = indent[m];
      }
    }
  }

  *data++ = ']'; /* close the array. */

  return data;
}

char *Json::write_pretty_object(const struct Json::object_s *object, size_t depth,
                               const char *indent, const char *newline,
                               char *data) {
  size_t k, m;
  struct Json::object_element_s *element;

  *data++ = '{'; /* open the object. */

  if (0 < object->length) {
    for (k = 0; '\0' != newline[k]; k++) {
      *data++ = newline[k];
    }

    for (element = object->start; nullptr != element;
         element = element->next) {
      if (element != object->start) {
        *data++ = ','; /* ','s seperate each element. */

        for (k = 0; '\0' != newline[k]; k++) {
          *data++ = newline[k];
        }
      }

      for (k = 0; k < depth + 1; k++) {
        for (m = 0; '\0' != indent[m]; m++) {
          *data++ = indent[m];
        }
      }

      data = Json::write_string(element->name, data);

      if (nullptr == data) {
        /* string was malformed! */
        return nullptr;
      }

      /* " : "s seperate each name/value pair. */
      *data++ = ' ';
      *data++ = ':';
      *data++ = ' ';

      data = Json::write_pretty_value(element->value, depth + 1, indent, newline,
                                     data);

      if (nullptr == data) {
        /* value was malformed! */
        return nullptr;
      }
    }

    for (k = 0; '\0' != newline[k]; k++) {
      *data++ = newline[k];
    }

    for (k = 0; k < depth; k++) {
      for (m = 0; '\0' != indent[m]; m++) {
        *data++ = indent[m];
      }
    }
  }

  *data++ = '}'; /* close the object. */

  return data;
}

char *Json::write_pretty_value(const struct Json::value_s *value, size_t depth,
                              const char *indent, const char *newline,
                              char *data) {
  switch (value->type) {
  default:
    /* unknown value type found! */
    return nullptr;
  case Json::type_number:
    return Json::write_number((struct Json::number_s *)value->payload, data);
  case Json::type_string:
    return Json::write_string((struct Json::string_s *)value->payload, data);
  case Json::type_array:
    return Json::write_pretty_array((struct Json::array_s *)value->payload, depth,
                                   indent, newline, data);
  case Json::type_object:
    return Json::write_pretty_object((struct Json::object_s *)value->payload,
                                    depth, indent, newline, data);
  case Json::type_true:
    data[0] = 't';
    data[1] = 'r';
    data[2] = 'u';
    data[3] = 'e';
    return data + 4;
  case Json::type_false:
    data[0] = 'f';
    data[1] = 'a';
    data[2] = 'l';
    data[3] = 's';
    data[4] = 'e';
    return data + 5;
  case Json::type_null:
    data[0] = 'n';
    data[1] = 'u';
    data[2] = 'l';
    data[3] = 'l';
    return data + 4;
  }
}

void *Json::write_pretty(const struct Json::value_s *value, const char *indent,
                        const char *newline, size_t *out_size) {
  size_t size = 0;
  size_t indent_size = 0;
  size_t newline_size = 0;
  char *data = nullptr;
  char *data_end = nullptr;

  if (nullptr == value) {
    return nullptr;
  }

  if (nullptr == indent) {
    indent = "  "; /* default to two spaces. */
  }

  if (nullptr == newline) {
    newline = "\n"; /* default to linux newlines. */
  }

  while ('\0' != indent[indent_size]) {
    ++indent_size; /* skip non-null terminating characters. */
  }

  while ('\0' != newline[newline_size]) {
    ++newline_size; /* skip non-null terminating characters. */
  }

  if (Json::write_pretty_get_value_size(value, 0, indent_size, newline_size,
                                       &size)) {
    /* value was malformed! */
    return nullptr;
  }

  size += 1; /* for the '\0' null terminating character. */

  data = (char *)malloc(size);

  if (nullptr == data) {
    /* malloc failed! */
    return nullptr;
  }

  data_end = Json::write_pretty_value(value, 0, indent, newline, data);

  if (nullptr == data_end) {
    /* bad chi occurred! */
    free(data);
    return nullptr;
  }

  /* null terminated the string. */
  *data_end = '\0';

  if (nullptr != out_size) {
    *out_size = size;
  }

  return data;
}
