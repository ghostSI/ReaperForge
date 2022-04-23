#ifndef JSON_H
#define JSON_H

#include "typedefs.h"

namespace Json
{
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

  /* A JSON string value. */
  struct string_s {
    /* utf-8 string */
    const char* string;
    /* The size (in bytes) of the string */
    size_t string_size;
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

  /* Parse a JSON text file, returning a pointer to the root of the JSON
   * structure. parse performs 1 call to malloc for the entire encoding.
   * Returns 0 if an error occurred (malformed JSON input, or malloc failed). */
  value_s* parse(const void* src, size_t src_size);
}

#endif // JSON_H
