#ifndef JSON_H
#define JSON_H

#include "typedefs.h"

namespace Json
{


  struct value_s;
  struct parse_result_s;

  enum parse_flags_e {
    parse_flags_default = 0,

    /* allow trailing commas in objects and arrays. For example, both [true,] and
    {"a" : null,} would be allowed with this option on. */
    parse_flags_allow_trailing_comma = 0x1,

    /* allow unquoted keys for objects. For example, {a : null} would be allowed
    with this option on. */
    parse_flags_allow_unquoted_keys = 0x2,

    /* allow a global unbracketed object. For example, a : null, b : true, c : {}
    would be allowed with this option on. */
    parse_flags_allow_global_object = 0x4,

    /* allow objects to use '=' instead of ':' between key/value pairs. For
    example, a = null, b : true would be allowed with this option on. */
    parse_flags_allow_equals_in_object = 0x8,

    /* allow that objects don't have to have comma separators between key/value
    pairs. */
    parse_flags_allow_no_commas = 0x10,

    /* allow c-style comments (either variants) to be ignored in the input JSON
    file. */
    parse_flags_allow_c_style_comments = 0x20,

    /* deprecated flag, unused. */
    parse_flags_deprecated = 0x40,

    /* record location information for each value. */
    parse_flags_allow_location_information = 0x80,

    /* allow strings to be 'single quoted'. */
    parse_flags_allow_single_quoted_strings = 0x100,

    /* allow numbers to be hexadecimal. */
    parse_flags_allow_hexadecimal_numbers = 0x200,

    /* allow numbers like +123 to be parsed. */
    parse_flags_allow_leading_plus_sign = 0x400,

    /* allow numbers like .0123 or 123. to be parsed. */
    parse_flags_allow_leading_or_trailing_decimal_point = 0x800,

    /* allow Infinity, -Infinity, NaN, -NaN. */
    parse_flags_allow_inf_and_nan = 0x1000,

    /* allow multi line string values. */
    parse_flags_allow_multi_line_strings = 0x2000,

    /* allow simplified JSON to be parsed. Simplified JSON is an enabling of a set
    of other parsing options. */
    parse_flags_allow_simplified_json =
    (parse_flags_allow_trailing_comma |
      parse_flags_allow_unquoted_keys |
      parse_flags_allow_global_object |
      parse_flags_allow_equals_in_object |
      parse_flags_allow_no_commas),

    /* allow JSON5 to be parsed. JSON5 is an enabling of a set of other parsing
       options. */
    parse_flags_allow_json5 =
    (parse_flags_allow_trailing_comma |
      parse_flags_allow_unquoted_keys |
      parse_flags_allow_c_style_comments |
      parse_flags_allow_single_quoted_strings |
      parse_flags_allow_hexadecimal_numbers |
      parse_flags_allow_leading_plus_sign |
      parse_flags_allow_leading_or_trailing_decimal_point |
      parse_flags_allow_inf_and_nan |
      parse_flags_allow_multi_line_strings)
  };

  /* Parse a JSON text file, returning a pointer to the root of the JSON
   * structure. parse performs 1 call to malloc for the entire encoding.
   * Returns 0 if an error occurred (malformed JSON input, or malloc failed). */
  struct value_s* parse(const void* src, size_t src_size);

  /* Parse a JSON text file, returning a pointer to the root of the JSON
   * structure. parse performs 1 call to alloc_func_ptr for the entire
   * encoding. Returns 0 if an error occurred (malformed JSON input, or malloc
   * failed). If an error occurred, the result struct (if not NULL) will explain
   * the type of error, and the location in the input it occurred. If
   * alloc_func_ptr is null then malloc is used. */
  struct value_s*
    parse_ex(const void* src, size_t src_size, size_t flags_bitset,
      void* (*alloc_func_ptr)(void*, size_t), void* user_data,
      struct parse_result_s* result);

  /* Extracts a value and all the data that makes it up into a newly created
   * value. extract_value performs 1 call to malloc for the entire encoding.
   */
  struct value_s*
    extract_value(const struct value_s* value);

  /* Extracts a value and all the data that makes it up into a newly created
   * value. extract_value performs 1 call to alloc_func_ptr for the entire
   * encoding. If alloc_func_ptr is null then malloc is used. */
  struct value_s*
    extract_value_ex(const struct value_s* value,
      void* (*alloc_func_ptr)(void*, size_t), void* user_data);

  /* Write out a minified JSON utf-8 string. This string is an encoding of the
   * minimal string characters required to still encode the same data.
   * write_minified performs 1 call to malloc for the entire encoding. Return
   * 0 if an error occurred (malformed JSON input, or malloc failed). The out_size
   * parameter is optional as the utf-8 string is null terminated. */
  void* write_minified(const struct value_s* value,
    size_t* out_size);

  /* Write out a pretty JSON utf-8 string. This string is encoded such that the
   * resultant JSON is pretty in that it is easily human readable. The indent and
   * newline parameters allow a user to specify what kind of indentation and
   * newline they want (two spaces / three spaces / tabs? \r, \n, \r\n ?). Both
   * indent and newline can be NULL, indent defaults to two spaces ("  "), and
   * newline defaults to linux newlines ('\n' as the newline character).
   * write_pretty performs 1 call to malloc for the entire encoding. Return 0
   * if an error occurred (malformed JSON input, or malloc failed). The out_size
   * parameter is optional as the utf-8 string is null terminated. */
  void* write_pretty(const struct value_s* value,
    const char* indent, const char* newline,
    size_t* out_size);

  /* Reinterpret a JSON value as a string. Returns null is the value was not a
   * string. */
  struct string_s*
    value_as_string(struct value_s* const value);

  /* Reinterpret a JSON value as a number. Returns null is the value was not a
   * number. */
  struct number_s*
    value_as_number(struct value_s* const value);

  /* Reinterpret a JSON value as an object. Returns null is the value was not an
   * object. */
  struct object_s*
    value_as_object(struct value_s* const value);

  /* Reinterpret a JSON value as an array. Returns null is the value was not an
   * array. */
  struct array_s*
    value_as_array(struct value_s* const value);

  /* Whether the value is true. */
  int value_is_true(const struct value_s* const value);

  /* Whether the value is false. */
  int value_is_false(const struct value_s* const value);

  /* Whether the value is null. */
  int value_is_null(const struct value_s* const value);

  /* The various types JSON values can be. Used to identify what a value is. */
  enum type_e : u64 {
    type_string,
    type_number,
    type_object,
    type_array,
    type_true,
    type_false,
    type_null
  };

  /* A JSON string value. */
  struct string_s {
    /* utf-8 string */
    const char* string;
    /* The size (in bytes) of the string */
    size_t string_size;
  };

  /* A JSON string value (extended). */
  struct string_ex_s {
    /* The JSON string this extends. */
    struct string_s string;

    /* The character offset for the value in the JSON input. */
    size_t offset;

    /* The line number for the value in the JSON input. */
    size_t line_no;

    /* The row number for the value in the JSON input, in bytes. */
    size_t row_no;
  };

  /* A JSON number value. */
  struct number_s {
    /* ASCII string containing representation of the number. */
    const char* number;
    /* the size (in bytes) of the number. */
    size_t number_size;
  };

  /* an element of a JSON object. */
  struct object_element_s {
    /* the name of this element. */
    struct string_s* name;
    /* the value of this element. */
    struct value_s* value;
    /* the next object element (can be NULL if the last element in the object). */
    struct object_element_s* next;
  };

  /* a JSON object value. */
  struct object_s {
    /* a linked list of the elements in the object. */
    struct object_element_s* start;
    /* the number of elements in the object. */
    size_t length;
  };

  /* an element of a JSON array. */
  struct array_element_s {
    /* the value of this element. */
    struct value_s* value;
    /* the next array element (can be NULL if the last element in the array). */
    struct array_element_s* next;
  };

  /* a JSON array value. */
  struct array_s {
    /* a linked list of the elements in the array. */
    struct array_element_s* start;
    /* the number of elements in the array. */
    size_t length;
  };

  /* a JSON value. */
  struct value_s {
    /* a pointer to either a string_s, number_s, object_s, or. */
    /* array_s. Should be cast to the appropriate struct type based on what.
     */
     /* the type of this value is. */
    void* payload;
    /* must be one of type_e. If type is type_true, type_false, or.
     */
     /* type_null, payload will be NULL. */
    type_e type;
  };

  /* a JSON value (extended). */
  struct value_ex_s {
    /* the JSON value this extends. */
    struct value_s value;

    /* the character offset for the value in the JSON input. */
    size_t offset;

    /* the line number for the value in the JSON input. */
    size_t line_no;

    /* the row number for the value in the JSON input, in bytes. */
    size_t row_no;
  };

  /* a parsing error code. */
  enum parse_error_e {
    /* no error occurred (huzzah!). */
    parse_error_none = 0,

    /* expected either a comma or a closing '}' or ']' to close an object or. */
    /* array! */
    parse_error_expected_comma_or_closing_bracket,

    /* colon separating name/value pair was missing! */
    parse_error_expected_colon,

    /* expected string to begin with '"'! */
    parse_error_expected_opening_quote,

    /* invalid escaped sequence in string! */
    parse_error_invalid_string_escape_sequence,

    /* invalid number format! */
    parse_error_invalid_number_format,

    /* invalid value! */
    parse_error_invalid_value,

    /* reached end of buffer before object/array was complete! */
    parse_error_premature_end_of_buffer,

    /* string was malformed! */
    parse_error_invalid_string,

    /* a call to malloc, or a user provider allocator, failed. */
    parse_error_allocator_failed,

    /* the JSON input had unexpected trailing characters that weren't part of the. */
    /* JSON value. */
    parse_error_unexpected_trailing_characters,

    /* catch-all error for everything else that exploded (real bad chi!). */
    parse_error_unknown
  };

  /* error report from parse_ex(). */
  struct parse_result_s {
    /* the error code (one of parse_error_e). */
    size_t error;

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
    size_t flags_bitset;
    char* data;
    char* dom;
    size_t dom_size;
    size_t data_size;
    size_t line_no;  /* line counter for error reporting. */
    size_t line_offset; /* (offset-line_offset) is the character number (in
            bytes). */
    size_t error;
  };

  int hexadecimal_digit(const char c);
  int hexadecimal_value(const char* c, const unsigned long size, unsigned long* result);
  int skip_whitespace(struct parse_state_s* state);
  int skip_c_style_comments(struct parse_state_s* state);
  int skip_all_skippables(struct parse_state_s* state);
  int get_value_size(struct parse_state_s* state, int is_global_object);
  int get_string_size(struct parse_state_s* state, size_t is_key);
  int is_valid_unquoted_key_char(const char c);
  int get_key_size(struct parse_state_s* state);
  int get_object_size(struct parse_state_s* state, int is_global_object);
  int get_array_size(struct parse_state_s* state);
  int get_number_size(struct parse_state_s* state);
  int get_value_size(struct parse_state_s* state, int is_global_object);
  void parse_value(struct parse_state_s* state, int is_global_object, struct value_s* value);
  void parse_string(struct parse_state_s* state, struct string_s* string);
  void parse_key(struct parse_state_s* state, struct string_s* string);
  void parse_object(struct parse_state_s* state, int is_global_object, struct object_s* object);
  void parse_array(struct parse_state_s* state, struct array_s* array);
  void parse_number(struct parse_state_s* state, struct number_s* number);
  void parse_value(struct parse_state_s* state, int is_global_object, struct value_s* value);
  int write_minified_get_value_size(const struct value_s* value, size_t* size);
  int write_get_number_size(const struct number_s* number, size_t* size);
  int write_get_string_size(const struct string_s* string, size_t* size);
  int write_minified_get_array_size(const struct array_s* array, size_t* size);
  int write_minified_get_object_size(const struct object_s* object, size_t* size);
  int write_minified_get_value_size(const struct value_s* value, size_t* size);
  char* write_minified_value(const struct value_s* value, char* data);
  char* write_number(const struct number_s* number, char* data);
  char* write_string(const struct string_s* string, char* data);
  char* write_minified_array(const struct array_s* array, char* data);
  char* write_minified_object(const struct object_s* object, char* data);
  char* write_minified_value(const struct value_s* value, char* data);
  int write_pretty_get_value_size(const struct value_s* value, size_t depth, size_t indent_size, size_t newline_size, size_t* size);
  int write_pretty_get_array_size(const struct array_s* array, size_t depth, size_t indent_size, size_t newline_size, size_t* size);
  int write_pretty_get_object_size(const struct object_s* object, size_t depth, size_t indent_size, size_t newline_size, size_t* size);
  int write_pretty_get_value_size(const struct value_s* value, size_t depth, size_t indent_size, size_t newline_size, size_t* size);
  char* write_pretty_value(const struct value_s* value, size_t depth, const char* indent, const char* newline, char* data);
  char* write_pretty_array(const struct array_s* array, size_t depth, const char* indent, const char* newline, char* data);
  char* write_pretty_object(const struct object_s* object, size_t depth, const char* indent, const char* newline, char* data);
  char* write_pretty_value(const struct value_s* value, size_t depth, const char* indent, const char* newline, char* data);
  struct extract_result_s extract_get_number_size(const struct number_s* const number);
  struct extract_result_s extract_get_string_size(const struct string_s* const string);
  struct extract_result_s extract_get_object_size(const struct object_s* const object);
  struct extract_result_s extract_get_array_size(const struct array_s* const array);
  struct extract_result_s extract_get_value_size(const struct value_s* const value);
  void extract_copy_value(struct extract_state_s* const state, const struct value_s* const value);
}


#endif // JSON_H
