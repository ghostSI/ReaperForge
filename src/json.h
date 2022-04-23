#ifndef JSON_H
#define JSON_H

#include "typedefs.h"

namespace Json
{
  /* The various types JSON values can be. Used to identify what a value is. */
  enum type : u64 {
    type_string,
    type_number,
    type_object,
    type_array,
    type_true,
    type_false,
    type_null
  };

  /* a JSON value. */
  struct value {
    /* a pointer to either a string, number, object, or. */
    /* array. Should be cast to the appropriate struct type based on what.
     */
     /* the type of this value is. */
    void* payload;
    /* must be one of type. If type is type_true, type_false, or.
     */
     /* type_null, payload will be NULL. */
    type type;
  };

  /* A JSON string value. */
  struct string {
    /* utf-8 string */
    const char* string;
    /* The size (in bytes) of the string */
    u64 string_size;
  };

  /* A JSON number value. */
  struct number {
    /* ASCII string containing representation of the number. */
    const char* number;
    /* the size (in bytes) of the number. */
    u64 number_size;
  };

  /* an element of a JSON object. */
  struct object_element {
    /* the name of this element. */
    struct string* name;
    /* the value of this element. */
    struct value* value;
    /* the next object element (can be NULL if the last element in the object). */
    struct object_element* next;
  };

  /* a JSON object value. */
  struct object {
    /* a linked list of the elements in the object. */
    struct object_element* start;
    /* the number of elements in the object. */
    u64 length;
  };

  /* an element of a JSON array. */
  struct array_element {
    /* the value of this element. */
    struct value* value;
    /* the next array element (can be NULL if the last element in the array). */
    struct array_element* next;
  };

  /* a JSON array value. */
  struct array {
    /* a linked list of the elements in the array. */
    struct array_element* start;
    /* the number of elements in the array. */
    u64 length;
  };

  /* Parse a JSON text file, returning a pointer to the root of the JSON
   * structure. parse performs 1 call to malloc for the entire encoding.
   * Returns 0 if an error occurred (malformed JSON input, or malloc failed). */
  value* parse(const void* src, u64 src_size);
}

#endif // JSON_H
