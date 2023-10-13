#ifndef JSON_INCITER_H_INCLUDED
#define JSON_INCITER_H_INCLUDED

// JSON_INCITER


#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#define JSON_INCITER_ELEMENT_LENGTH(Result)     ((Result).length)
#define JSON_INCITER_STRING_LENGTH(Result)      ((Result).length - 2)
#define JSON_INCITER_ELEMENT_NEXT_START(Result) ((Result).start + (Result).length)


typedef enum {
    JSON_INCITER_STATE_OK = 0,
    JSON_INCITER_STATE_DONE,
    JSON_INCITER_STATE_INVALID,
    JSON_INCITER_STATE_INCOMPLETE,
} json_inciter_state_t;


typedef enum {
    JSON_INCITER_ELEMENT_TAG_INVALID = 0,
    JSON_INCITER_ELEMENT_TAG_INCOMPLETE,
    JSON_INCITER_ELEMENT_TAG_NULL,
    JSON_INCITER_ELEMENT_TAG_TRUE,
    JSON_INCITER_ELEMENT_TAG_FALSE,
    JSON_INCITER_ELEMENT_TAG_NUMBER,
    JSON_INCITER_ELEMENT_TAG_STRING,
    JSON_INCITER_ELEMENT_TAG_ARRAY,
    JSON_INCITER_ELEMENT_TAG_OBJECT,
} json_inciter_element_tag_t;


typedef struct {
    json_inciter_element_tag_t tag;

    const char *start;
    size_t      length;

    union {
        double      number;
        const char *string;
    } as;
} json_inciter_element_t;


typedef enum {
    _JSON_INCITER_TOKEN_ANY = 0,
    _JSON_INCITER_TOKEN_KEYWORD,
    _JSON_INCITER_TOKEN_NUMBER,
    _JSON_INCITER_TOKEN_STRING,
    _JSON_INCITER_TOKEN_ARRAY,
    _JSON_INCITER_TOKEN_OBJECT,
} _json_inciter_token_t;


typedef enum {
    _JSON_INCITER_LITERAL_PARSE_SUCCESS = 0,
    _JSON_INCITER_LITERAL_PARSE_INCOMPLETE,
    _JSON_INCITER_LITERAL_PARSE_ERROR,
} _json_inciter_literal_parse_t;


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


_json_inciter_literal_parse_t _json_inciter_parse_literal(const char *buffer, const char *keyword_str) {
    size_t keyword_len = strlen(keyword_str);
    size_t buffer_len  = strlen(buffer);

    // Full keyword
    if (keyword_len <= buffer_len) {
        if (strncmp(buffer, keyword_str, keyword_len) == 0) {
            if (IS_TERMINATOR(buffer[keyword_len])) {
                return _JSON_INCITER_LITERAL_PARSE_SUCCESS;
            } else {
                return _JSON_INCITER_LITERAL_PARSE_ERROR;
            }
        } else {
            return _JSON_INCITER_LITERAL_PARSE_ERROR;
        }
    }
    // Partial keyword
    else if (strncmp(buffer, keyword_str, buffer_len) == 0) {
        return _JSON_INCITER_LITERAL_PARSE_INCOMPLETE;
    } else {
        return _JSON_INCITER_LITERAL_PARSE_ERROR;
    }
}


json_inciter_element_t _json_inciter_parse_value_of_type(const char *buffer, _json_inciter_token_t token) {
    size_t parsing_index = 0;

    json_inciter_element_t element = {
        .tag    = JSON_INCITER_ELEMENT_TAG_INCOMPLETE,
        .start  = buffer,
        .length = 0,
    };

    parsing_index += _json_inciter_skip_whitespace(&buffer[parsing_index]);

    if (token == _JSON_INCITER_TOKEN_ANY) {
        token = _json_inciter_get_next_token_type(buffer[parsing_index]);
    }

    switch (token) {
        case _JSON_INCITER_TOKEN_KEYWORD: {
            const char *keyword_null  = "null";
            const char *keyword_true  = "true";
            const char *keyword_false = "false";

            _json_inciter_literal_parse_t literal_element = _JSON_INCITER_LITERAL_PARSE_ERROR;
            element.start                                 = &buffer[parsing_index];
            element.length                                = 0;

            literal_element = _json_inciter_parse_literal(&buffer[parsing_index], keyword_null);
            if (literal_element == _JSON_INCITER_LITERAL_PARSE_SUCCESS) {
                element.tag    = JSON_INCITER_ELEMENT_TAG_NULL;
                element.length = strlen(keyword_null);
                break;
            } else if (literal_element == _JSON_INCITER_LITERAL_PARSE_INCOMPLETE) {
                element.tag = JSON_INCITER_ELEMENT_TAG_INCOMPLETE;
                break;
            }

            literal_element = _json_inciter_parse_literal(&buffer[parsing_index], keyword_true);
            if (literal_element == _JSON_INCITER_LITERAL_PARSE_SUCCESS) {
                element.tag    = JSON_INCITER_ELEMENT_TAG_TRUE;
                element.length = strlen(keyword_true);
                break;
            } else if (literal_element == _JSON_INCITER_LITERAL_PARSE_INCOMPLETE) {
                element.tag = JSON_INCITER_ELEMENT_TAG_INCOMPLETE;
                break;
            }

            literal_element = _json_inciter_parse_literal(&buffer[parsing_index], keyword_false);
            if (literal_element == _JSON_INCITER_LITERAL_PARSE_SUCCESS) {
                element.tag    = JSON_INCITER_ELEMENT_TAG_FALSE;
                element.length = strlen(keyword_false);
                break;
            } else if (literal_element == _JSON_INCITER_LITERAL_PARSE_INCOMPLETE) {
                element.tag = JSON_INCITER_ELEMENT_TAG_INCOMPLETE;
                break;
            }

            element.tag = JSON_INCITER_ELEMENT_TAG_INVALID;
            break;
        }

        case _JSON_INCITER_TOKEN_NUMBER: {
            const char *endptr = NULL;

            element.start     = &buffer[parsing_index];
            element.as.number = strtod(&buffer[parsing_index], (char **)&endptr);

            // No conversion, invalid number
            if (endptr == &buffer[parsing_index]) {
                element.tag    = JSON_INCITER_ELEMENT_TAG_INVALID;
                element.length = 0;
            }
            // Conversion successful
            else {
                size_t terminator_index = parsing_index + (endptr - &buffer[parsing_index]);
                if (IS_TERMINATOR(buffer[terminator_index])) {
                    element.length = terminator_index - parsing_index;
                    element.tag    = JSON_INCITER_ELEMENT_TAG_NUMBER;
                } else {
                    element.tag    = JSON_INCITER_ELEMENT_TAG_INVALID;
                    element.length = 0;
                }
            }
            break;
        }

        case _JSON_INCITER_TOKEN_STRING: {
            element.start     = &buffer[parsing_index];
            element.length    = 0;
            element.as.string = NULL;

            if (buffer[parsing_index] != '"') {
                element.tag = JSON_INCITER_ELEMENT_TAG_INVALID;
                break;
            }
            parsing_index++;

            size_t string_end = parsing_index + _json_inciter_skip_string(&buffer[parsing_index]);
            // Valid string
            if (buffer[string_end] == '"') {
                element.tag       = JSON_INCITER_ELEMENT_TAG_STRING;
                element.start     = &buffer[parsing_index - 1];         // Include the quotes
                element.length    = string_end - parsing_index + 2;     // Include the quotes
                element.as.string = &buffer[parsing_index];             // No quotes
            }
            // End of stream
            else {
                element.tag = JSON_INCITER_ELEMENT_TAG_INCOMPLETE;
            }

            break;
        }

        case _JSON_INCITER_TOKEN_ARRAY: {
            element.start  = &buffer[parsing_index];
            element.length = 0;

            if (buffer[parsing_index] != '[') {
                element.tag = JSON_INCITER_ELEMENT_TAG_INVALID;
                break;
            }
            parsing_index++;

            int terminator_position = _json_inciter_skip_to_terminator(&buffer[parsing_index], ']');
            if (terminator_position < 0) {
                element.tag = JSON_INCITER_ELEMENT_TAG_INVALID;
            } else {
                element.tag    = JSON_INCITER_ELEMENT_TAG_ARRAY;
                element.length = terminator_position + 2;
            }
            break;
        }

        case _JSON_INCITER_TOKEN_OBJECT: {
            element.start  = &buffer[parsing_index];
            element.length = 0;

            if (buffer[parsing_index] != '{') {
                element.tag = JSON_INCITER_ELEMENT_TAG_INVALID;
                break;
            }
            parsing_index++;

            int terminator_position = _json_inciter_skip_to_terminator(&buffer[parsing_index], '}');
            if (terminator_position < 0) {
                element.tag = JSON_INCITER_ELEMENT_TAG_INVALID;
            } else {
                element.tag    = JSON_INCITER_ELEMENT_TAG_OBJECT;
                element.length = terminator_position + 2;
            }
            break;
        }

        default:
            break;
    }

    return element;
}


json_inciter_element_t json_inciter_parse_value(const char *buffer) {
    return _json_inciter_parse_value_of_type(buffer, _JSON_INCITER_TOKEN_ANY);
}


uint8_t json_inciter_copy_content(char *content_buffer, size_t max_len, json_inciter_element_t element) {
    size_t      len   = 0;
    const char *start = NULL;

    if (element.tag == JSON_INCITER_ELEMENT_TAG_STRING) {
        len   = JSON_INCITER_STRING_LENGTH(element);
        start = element.as.string;
    } else {
        len   = JSON_INCITER_ELEMENT_LENGTH(element);
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


json_inciter_element_t json_inciter_parse_pair(const char *buffer, const char **key, size_t *key_len) {
    json_inciter_element_t element = {
        .tag    = JSON_INCITER_ELEMENT_TAG_INCOMPLETE,
        .start  = buffer,
        .length = 0,
    };

    size_t parsing_index = _json_inciter_skip_whitespace(buffer);

    json_inciter_element_t key_element =
        _json_inciter_parse_value_of_type(&buffer[parsing_index], _JSON_INCITER_TOKEN_STRING);
    if (key_element.tag == JSON_INCITER_ELEMENT_TAG_STRING) {
        if (key != NULL) {
            *key = key_element.as.string;
        }
        if (key_len != NULL) {
            *key_len = JSON_INCITER_STRING_LENGTH(key_element);
        }

        parsing_index += JSON_INCITER_ELEMENT_LENGTH(key_element);
        parsing_index += _json_inciter_skip_whitespace(&buffer[parsing_index]);

        // Pair
        if (buffer[parsing_index] == ':') {
            parsing_index++;
            json_inciter_element_t element = json_inciter_parse_value(&buffer[parsing_index]);
            return element;
        }
        // Incomplete
        else if (buffer[parsing_index] == '\0') {
            return element;
        }
        // Invalid
        else {
            element.tag    = JSON_INCITER_ELEMENT_TAG_INVALID;
            element.start  = &buffer[parsing_index];
            element.length = 0;
            return element;
        }
    } else {
        return key_element;
    }
}


json_inciter_state_t json_inciter_next_element_start(const char *buffer, json_inciter_element_tag_t tag,
                                                     const char **next_start) {
    size_t parsing_index = _json_inciter_skip_whitespace(buffer);
    switch (buffer[parsing_index]) {
        // Comma, everything as expected
        case ',':
            if (next_start != NULL) {
                *next_start = &buffer[parsing_index + 1];
            }
            return JSON_INCITER_STATE_OK;
        case '}':
            if (tag == JSON_INCITER_ELEMENT_TAG_OBJECT) {
                return JSON_INCITER_STATE_DONE;
            } else {
                return JSON_INCITER_STATE_INVALID;
            }
        case ']':
            if (tag == JSON_INCITER_ELEMENT_TAG_ARRAY) {
                return JSON_INCITER_STATE_DONE;
            } else {
                return JSON_INCITER_STATE_INVALID;
            }
        case '\0':
            return JSON_INCITER_STATE_INCOMPLETE;
        default:
            return JSON_INCITER_STATE_INVALID;
    }
}


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


json_inciter_state_t json_inciter_find_value_in_object(json_inciter_element_t object, const char *required_key,
                                                       json_inciter_element_t *element) {
    json_inciter_state_t json_inciter_state = JSON_INCITER_STATE_OK;
    const char          *json_content       = json_inciter_element_content_start(object);
    size_t               required_key_len   = strlen(required_key);

    do {
        const char            *key      = NULL;
        size_t                 key_size = 0;
        json_inciter_element_t value    = json_inciter_parse_pair(json_content, &key, &key_size);
        if (value.tag == JSON_INCITER_ELEMENT_TAG_INVALID) {
            return JSON_INCITER_STATE_INVALID;
        } else if (value.tag == JSON_INCITER_ELEMENT_TAG_INCOMPLETE) {
            return JSON_INCITER_STATE_INCOMPLETE;
        }

        if (key_size == required_key_len && strncmp(required_key, key, required_key_len) == 0) {
            *element = value;
            return JSON_INCITER_STATE_OK;
        }

        json_content = JSON_INCITER_ELEMENT_NEXT_START(value);
        json_inciter_state =
            json_inciter_next_element_start(json_content, JSON_INCITER_ELEMENT_TAG_OBJECT, &json_content);
    } while (json_inciter_state == JSON_INCITER_STATE_OK);

    return JSON_INCITER_STATE_DONE;
}


#undef IS_TERMINATOR


#endif
