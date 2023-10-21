#ifndef JSON_INCITER_H_INCLUDED
#define JSON_INCITER_H_INCLUDED

// JSON_INCITER


#include <stdlib.h>
#include <stdint.h>
#include <string.h>


/**
 * @brief Length of a string element (no check is performed)
 *
 * @param Element element in question
 *
 * @return Length of string element
 */
#define JSON_INCITER_STRING_LENGTH(Element) ((Element).length - 2)

/**
 * @brief Pointer to the remaining json, after the current element
 *
 * @param Element element in question
 *
 * @return Pointer to the remaining json stream
 */
#define JSON_INCITER_ELEMENT_NEXT_START(Element) ((Element).start + (Element).length)


/**
 * @brief State of the json stream
 */
typedef enum {
    JSON_INCITER_OK = 0,         // Valid json, a correct element was found
    JSON_INCITER_DONE,           // Json stream is done (no element was returned)
    JSON_INCITER_INVALID,        // Invalid json
    JSON_INCITER_INCOMPLETE,     // Incomplete json (requires more character to successfully parse the next
                                 // element)
} json_inciter_t;


/**
 * @brief Element tag
 */
typedef enum {
    JSON_INCITER_ELEMENT_TAG_NULL = 0,
    JSON_INCITER_ELEMENT_TAG_TRUE,
    JSON_INCITER_ELEMENT_TAG_FALSE,
    JSON_INCITER_ELEMENT_TAG_NUMBER,
    JSON_INCITER_ELEMENT_TAG_STRING,
    JSON_INCITER_ELEMENT_TAG_ARRAY,
    JSON_INCITER_ELEMENT_TAG_OBJECT,
} json_inciter_element_tag_t;


/**
 * @brief JSON element, after being parsed.
 */
typedef struct {
    json_inciter_element_tag_t tag;     // Tag

    const char *start;      // Pointer to the string that makes up the element, in the original json stream
    size_t      length;     // Length of the string that makes up the element

    union {
        double      number;     // Numerical value
        const char *string;     // Pointer to the string value (within quotes)
    } as;
} json_inciter_element_t;


/*
 * Private types and functions
 */

typedef enum {
    _JSON_INCITER_TOKEN_ANY = 0,
    _JSON_INCITER_TOKEN_KEYWORD,
    _JSON_INCITER_TOKEN_NUMBER,
    _JSON_INCITER_TOKEN_STRING,
    _JSON_INCITER_TOKEN_ARRAY,
    _JSON_INCITER_TOKEN_OBJECT,
} _json_inciter_token_t;


size_t _json_inciter_skip_whitespace(const char *buffer) {
    size_t to_skip = 0;
    for (;;) {
        switch (buffer[to_skip]) {
            case '\t':
            case '\n':
            case '\r':
            case ' ':
                to_skip++;
                break;
            default:
                return to_skip;
        }
    }
}


size_t _json_inciter_skip_string(const char *buffer) {
    size_t to_skip = 0;
    for (;;) {
        switch (buffer[to_skip]) {
            case '\\':
                to_skip++;
                to_skip++;     // Escaped character; no matter what it is, skip it
                break;

            case '"':
            case '\0':
                return to_skip;

            default:
                to_skip++;
                break;
        }
    }
}


int _json_inciter_skip_to_terminator(const char *buffer, char terminator) {
    size_t to_skip      = 0;
    size_t object_depth = 0;
    size_t array_depth  = 0;

    for (;;) {
        // Stream done
        if (buffer[to_skip] == '\0') {
            return to_skip;
        }
        // Entering in nested array
        else if (buffer[to_skip] == '[') {
            array_depth++;
            to_skip++;
        }
        // Entering in nested object
        else if (buffer[to_skip] == '{') {
            object_depth++;
            to_skip++;
        }
        // Out of nested array
        else if (buffer[to_skip] == ']') {
            // Reached the end of the array
            if (terminator == ']' && array_depth == 0 && object_depth == 0) {
                return to_skip;
            }
            // Out of an array
            else if (array_depth > 0) {
                array_depth--;
                to_skip++;
            }
            // Array closed, but was not opened: invalid json
            else {
                return -1;
            }
        }
        // Out of nested object
        else if (buffer[to_skip] == '}') {
            // Reached the end of the array
            if (terminator == '}' && array_depth == 0 && object_depth == 0) {
                return to_skip;
            }
            // Out of an object
            else if (object_depth > 0) {
                object_depth--;
                to_skip++;
            }
            // Object closed, but was not opened: invalid json
            else {
                return -1;
            }
        }
        // Encountered string, terminator could be included but should be ignored
        else if (buffer[to_skip] == '"') {
            to_skip++;
            to_skip += _json_inciter_skip_string(&buffer[to_skip]);

            // If stream is not done, skip the closing quote
            if (buffer[to_skip] != '\0') {
                to_skip++;
            }
        } else {
            to_skip++;
        }
    }
}


_json_inciter_token_t _json_inciter_get_next_token_type(char current_char) {
    switch (current_char) {
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
            return _JSON_INCITER_TOKEN_NUMBER;

        case '"':
            return _JSON_INCITER_TOKEN_STRING;

        case '[':
            return _JSON_INCITER_TOKEN_ARRAY;

        case '{':
            return _JSON_INCITER_TOKEN_OBJECT;

        default:
            return _JSON_INCITER_TOKEN_KEYWORD;
    }
}


#define IS_TERMINATOR(Char)                                                                                            \
    ((Char) == ',' || (Char) == ' ' || (Char) == '\t' || (Char) == '\n' || (Char) == '\r' || (Char) == '}' ||          \
     (Char) == ']' || (Char) == '\0')


json_inciter_t _json_inciter_parse_literal(const char *buffer, const char *keyword_str) {
    size_t keyword_len = strlen(keyword_str);
    size_t buffer_len  = strlen(buffer);

    // Full keyword
    if (keyword_len <= buffer_len) {
        if (strncmp(buffer, keyword_str, keyword_len) == 0) {
            if (IS_TERMINATOR(buffer[keyword_len])) {
                return JSON_INCITER_OK;
            } else {
                return JSON_INCITER_INVALID;
            }
        } else {
            return JSON_INCITER_INVALID;
        }
    }
    // Partial keyword
    else if (strncmp(buffer, keyword_str, buffer_len) == 0) {
        return JSON_INCITER_INCOMPLETE;
    } else {
        return JSON_INCITER_INVALID;
    }
}


json_inciter_t _json_inciter_parse_value_of_type(const char *buffer, _json_inciter_token_t token,
                                                 json_inciter_element_t *element) {
    size_t parsing_index = 0;

    parsing_index += _json_inciter_skip_whitespace(&buffer[parsing_index]);

    if (token == _JSON_INCITER_TOKEN_ANY) {
        token = _json_inciter_get_next_token_type(buffer[parsing_index]);
    }

    switch (token) {
        case _JSON_INCITER_TOKEN_KEYWORD: {
            const char *keyword_null  = "null";
            const char *keyword_true  = "true";
            const char *keyword_false = "false";

            json_inciter_t result = JSON_INCITER_INVALID;
            element->start        = &buffer[parsing_index];
            element->length       = 0;

            result = _json_inciter_parse_literal(&buffer[parsing_index], keyword_null);
            if (result == JSON_INCITER_OK) {
                element->tag    = JSON_INCITER_ELEMENT_TAG_NULL;
                element->length = strlen(keyword_null);
                return JSON_INCITER_OK;
            } else if (result == JSON_INCITER_INCOMPLETE) {
                return JSON_INCITER_INCOMPLETE;
            }

            result = _json_inciter_parse_literal(&buffer[parsing_index], keyword_true);
            if (result == JSON_INCITER_OK) {
                element->tag    = JSON_INCITER_ELEMENT_TAG_TRUE;
                element->length = strlen(keyword_true);
                return JSON_INCITER_OK;
            } else if (result == JSON_INCITER_INCOMPLETE) {
                return JSON_INCITER_INCOMPLETE;
            }

            result = _json_inciter_parse_literal(&buffer[parsing_index], keyword_false);
            if (result == JSON_INCITER_OK) {
                element->tag    = JSON_INCITER_ELEMENT_TAG_FALSE;
                element->length = strlen(keyword_false);
                return JSON_INCITER_OK;
            } else if (result == JSON_INCITER_INCOMPLETE) {
                return JSON_INCITER_INCOMPLETE;
            }

            return JSON_INCITER_INVALID;
        }

        case _JSON_INCITER_TOKEN_NUMBER: {
            const char *endptr = NULL;

            element->start     = &buffer[parsing_index];
            element->as.number = strtod(&buffer[parsing_index], (char **)&endptr);

            // No conversion, invalid number
            if (endptr == &buffer[parsing_index]) {
                element->length = 0;
                return JSON_INCITER_INVALID;
            }
            // Conversion successful
            else {
                size_t terminator_index = parsing_index + (endptr - &buffer[parsing_index]);
                if (IS_TERMINATOR(buffer[terminator_index])) {
                    element->length = terminator_index - parsing_index;
                    element->tag    = JSON_INCITER_ELEMENT_TAG_NUMBER;
                    return JSON_INCITER_OK;
                } else {
                    element->length = 0;
                    return JSON_INCITER_INVALID;
                }
            }
            break;
        }

        case _JSON_INCITER_TOKEN_STRING: {
            element->start     = &buffer[parsing_index];
            element->length    = 0;
            element->as.string = NULL;

            if (buffer[parsing_index] != '"') {
                return JSON_INCITER_INVALID;
            }
            parsing_index++;

            size_t string_end = parsing_index + _json_inciter_skip_string(&buffer[parsing_index]);
            // Valid string
            if (buffer[string_end] == '"') {
                element->tag       = JSON_INCITER_ELEMENT_TAG_STRING;
                element->start     = &buffer[parsing_index - 1];         // Include the quotes
                element->length    = string_end - parsing_index + 2;     // Include the quotes
                element->as.string = &buffer[parsing_index];             // No quotes
                return JSON_INCITER_OK;
            }
            // End of stream
            else {
                return JSON_INCITER_INCOMPLETE;
            }

            break;
        }

        case _JSON_INCITER_TOKEN_ARRAY: {
            element->start  = &buffer[parsing_index];
            element->length = 0;

            if (buffer[parsing_index] != '[') {
                return JSON_INCITER_INVALID;
            }
            parsing_index++;

            int terminator_position = _json_inciter_skip_to_terminator(&buffer[parsing_index], ']');
            if (terminator_position < 0) {
                return JSON_INCITER_INVALID;
            } else {
                element->tag    = JSON_INCITER_ELEMENT_TAG_ARRAY;
                element->length = terminator_position + 2;
                return JSON_INCITER_OK;
            }
            break;
        }

        case _JSON_INCITER_TOKEN_OBJECT: {
            element->start  = &buffer[parsing_index];
            element->length = 0;

            if (buffer[parsing_index] != '{') {
                return JSON_INCITER_INVALID;
            }
            parsing_index++;

            int terminator_position = _json_inciter_skip_to_terminator(&buffer[parsing_index], '}');
            if (terminator_position < 0) {
                return JSON_INCITER_INVALID;
            } else {
                element->tag    = JSON_INCITER_ELEMENT_TAG_OBJECT;
                element->length = terminator_position + 2;
                return JSON_INCITER_OK;
            }
            break;
        }

        default:
            break;
    }

    return JSON_INCITER_INVALID;
}


/**
 * Public API
 */


/**
 * @brief attempts to parse a jso n buffer
 *
 * @param buffer the json string
 * @param element a pointer to the struct to be filled with the parsed element
 *
 * @return result state
 */
json_inciter_t json_inciter_parse_value(const char *buffer, json_inciter_element_t *element) {
    return _json_inciter_parse_value_of_type(buffer, _JSON_INCITER_TOKEN_ANY, element);
}


/**
 * @brief parse a key-value pair
 *
 * @param buffer the json string
 * @param key pointer to a buffer to be filled with the key string
 * @param key_len pointer to an index to be filled with the key string length
 * @param element a pointer to the struct to be filled with the parsed element
 *
 * @return result state
 */
json_inciter_t json_inciter_parse_pair(const char *buffer, const char **key, size_t *key_len,
                                       json_inciter_element_t *element) {
    size_t parsing_index = _json_inciter_skip_whitespace(buffer);

    json_inciter_element_t key_element = {0};

    json_inciter_t result =
        _json_inciter_parse_value_of_type(&buffer[parsing_index], _JSON_INCITER_TOKEN_STRING, &key_element);
    if (result != JSON_INCITER_OK) {
        return result;
    }

    if (key_element.tag == JSON_INCITER_ELEMENT_TAG_STRING) {
        if (key != NULL) {
            *key = key_element.as.string;
        }
        if (key_len != NULL) {
            *key_len = JSON_INCITER_STRING_LENGTH(key_element);
        }

        parsing_index += key_element.length;
        parsing_index += _json_inciter_skip_whitespace(&buffer[parsing_index]);

        // Pair
        if (buffer[parsing_index] == ':') {
            parsing_index++;
            return json_inciter_parse_value(&buffer[parsing_index], element);
        }
        // Incomplete
        else if (buffer[parsing_index] == '\0') {
            return JSON_INCITER_INCOMPLETE;
        }
        // Invalid
        else {
            return JSON_INCITER_INVALID;
        }
    } else {
        return JSON_INCITER_INVALID;
    }
}


/**
 * @brief find the pointer to the next element in the json buffer
 *
 * @param buffer json
 * @param tag JSON_INCITER_ELEMENT_TAG_OBJECT or JSON_INCITER_ELEMENT_TAG_ARRAY
 * @param next_start pointer to be filled with the position of the next element, if any
 *
 * @return result
 */
json_inciter_t json_inciter_next_element_start(const char *buffer, json_inciter_element_tag_t tag,
                                               const char **next_start) {
    size_t parsing_index = _json_inciter_skip_whitespace(buffer);
    switch (buffer[parsing_index]) {
        // Comma, everything as expected
        case ',':
            if (next_start != NULL) {
                *next_start = &buffer[parsing_index + 1];
            }
            return JSON_INCITER_OK;
        case '}':
            if (tag == JSON_INCITER_ELEMENT_TAG_OBJECT) {
                return JSON_INCITER_DONE;
            } else {
                return JSON_INCITER_INVALID;
            }
        case ']':
            if (tag == JSON_INCITER_ELEMENT_TAG_ARRAY) {
                return JSON_INCITER_DONE;
            } else {
                return JSON_INCITER_INVALID;
            }
        case '\0':
            return JSON_INCITER_INCOMPLETE;
        default:
            return JSON_INCITER_INVALID;
    }
}


/**
 * @brief get a pointer to the beginning of the content of the element
 *
 * @param element
 *
 * @return pointer
 */
const char *json_inciter_element_content_start(json_inciter_element_t element) {
    switch (element.tag) {
        case JSON_INCITER_ELEMENT_TAG_OBJECT:
        case JSON_INCITER_ELEMENT_TAG_ARRAY:
        case JSON_INCITER_ELEMENT_TAG_STRING:
            return element.start + 1;
        default:
            return element.start;
    }
}


/**
 * @brief Look for a specific key in an object
 *
 * @param object
 * @param required_key
 * @param element
 *
 * @return result
 */
json_inciter_t json_inciter_find_value_in_object(json_inciter_element_t object, const char *required_key,
                                                 json_inciter_element_t *element) {
    json_inciter_t iteration_result = JSON_INCITER_OK;
    const char    *json_content     = json_inciter_element_content_start(object);
    size_t         required_key_len = strlen(required_key);

    do {
        const char            *key      = NULL;
        size_t                 key_size = 0;
        json_inciter_element_t value    = {0};
        json_inciter_t         result   = json_inciter_parse_pair(json_content, &key, &key_size, &value);

        if (result != JSON_INCITER_OK) {
            return result;
        }

        if (key_size == required_key_len && strncmp(required_key, key, required_key_len) == 0) {
            *element = value;
            return JSON_INCITER_OK;
        }

        json_content = JSON_INCITER_ELEMENT_NEXT_START(value);
        iteration_result =
            json_inciter_next_element_start(json_content, JSON_INCITER_ELEMENT_TAG_OBJECT, &json_content);
    } while (iteration_result == JSON_INCITER_OK);

    return JSON_INCITER_DONE;
}


/**
 * @brief copy the content of the element into a buffer. Useful mostly for strings.
 *
 * @param content_buffer the buffer to be filled with the content
 * @param max_len the size of the buffer
 * @param element
 *
 * @return 0 if successful (i.e. the buffer size was sufficient to hold the content), -1 otherwise
 */
uint8_t json_inciter_copy_content(char *content_buffer, size_t max_len, json_inciter_element_t element) {
    size_t      len   = 0;
    const char *start = NULL;

    if (element.tag == JSON_INCITER_ELEMENT_TAG_STRING) {
        len   = JSON_INCITER_STRING_LENGTH(element);
        start = element.as.string + 1;
    } else {
        len   = element.length;
        start = element.start;
    }

    if (len < max_len) {
        memcpy(content_buffer, start, len);
        content_buffer[len] = '\0';
        return 0;
    } else {
        return -1;
    }
}


#undef IS_TERMINATOR


#endif
