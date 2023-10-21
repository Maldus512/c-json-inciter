# JSON *Inc*remental *Ite*rator

Parsing JSON is hard work. Take the following [example](https://json.org/example.html):

```json
{
    "glossary": {
        "title": "example glossary",
        "GlossDiv": {
            "title": "S",
            "GlossList": {
                "GlossEntry": {
                    "ID": "SGML",
                    "SortAs": "SGML",
                    "GlossTerm": "Standard Generalized Markup Language",
                    "Acronym": "SGML",
                    "Abbrev": "ISO 8879:1986",
                    "GlossDef": {
                        "para": "A meta-markup language, used to create markup languages such as DocBook.",
                        "GlossSeeAlso": ["GML", "XML"]
                    },
                    "GlossSee": "markup"
                }
            }
        }
    }
}
```

Say we wanted to extract the value of the `title` property. 
One of the most obvious options would be to use the excellent [cJSON](https://github.com/DaveGamble/cJSON) library, parse the string and navigate to the required key.

This input alone is 583 bytes (361 if minified, but you can't rely on stray json to do you any favours), and cJSON requires an additional 1473 bytes to allocate the data structures that represent the parse result.

2 KB of RAM to get a measly title may be acceptable on a 32GB machine, but it's a tad too much in certain contexts.

It's not the library's fault, of course; it's just designed for that usage because every `cJSON` object mantains a copy of the data it carries.
A more conservative approach would be to keep a pointer to the data (which is mostly comprised of strings anyway) in the original json string, without creating any copies.
It's the approach taken by [jsmn](https://github.com/zserge/jsmn), and it works quite well: after parsing the result is stored in a token array that requires between 400 and 500 bytes, depending on word size.

Still, it's two times the original string. It hurts to allocate so much memory with such a simple target.

Enter `json_inciter`. 
Like jsmn it stores pointers to the elements' data instead of creating copies; unlike jsmn it doesn't require parsing the entire string to navigate it.
The only memory required is the one in the string (even if partial).

## Usage

Obtain `json_inciter.h` and include it.

```c
    // ... 

    json_inciter_element_t root = {0};
    json_inciter_t result = json_inciter_parse_value(json_content, &root);

    if (result == JSON_INCITER_OK && root.tag == JSON_INCITER_ELEMENT_TAG_OBJECT) {
        json_inciter_element_t title_element = {0};
        result = json_inciter_find_value_in_object(root, "title", &title_element);

        if (result == JSON_INCITER_OK && title_element.tag == JSON_INCITER_ELEMENT_TAG_STRING) {
            char title[32] = {0};
            json_inciter_copy_content(title, sizeof(title), title_element);
            printf("Title found: %s\n", title);
        }
    }
```

## API

Operations return the `json_inciter_t` enum.

```c
typedef enum {
    JSON_INCITER_OK = 0,         // Valid json, a correct element was found
    JSON_INCITER_DONE,           // Json stream is done (no element was returned)
    JSON_INCITER_INVALID,        // Invalid json
    JSON_INCITER_INCOMPLETE,     // Incomplete json (requires more character to successfully parse the next
                                 // element)
} json_inciter_t;
```

`JSON_INCITER_DONE` and `JSON_INCITER_INCOMPLETE` are not necessarily error conditions: the former indicates that the json string is finished, while the latter tells us that there should be more (whether or not it can be the case is up to the developer).

If `JSON_INCITER_INVALID` is returned then the string cannot successfully be interpreted as json.

A parsing operation fills up a `json_inciter_element_t` struct:

```c
typedef enum {
    JSON_INCITER_ELEMENT_TAG_NULL = 0,
    JSON_INCITER_ELEMENT_TAG_TRUE,
    JSON_INCITER_ELEMENT_TAG_FALSE,
    JSON_INCITER_ELEMENT_TAG_NUMBER,
    JSON_INCITER_ELEMENT_TAG_STRING,
    JSON_INCITER_ELEMENT_TAG_ARRAY,
    JSON_INCITER_ELEMENT_TAG_OBJECT,
} json_inciter_element_tag_t;


typedef struct {
    json_inciter_element_tag_t tag;     // Tag

    const char *start;      // Pointer to the string that makes up the element, in the original json stream
    size_t      length;     // Length of the string that makes up the element

    union {
        double      number;     // Numerical value
        const char *string;     // Pointer to the string value (within quotes)
    } as;
} json_inciter_element_t;
```

The parsing result should be judged by the returned `json_inciter_t`.

Every possible value has a corresponding tag. 
`true`, `false` and `null` get a specific tag each because they don't carry any additional value.

Numbers and strings have an additional field, the number value and the beginning of the string (ignoring the quotes) respectively.

The main API entry allows to parse any json value.

```c
json_inciter_t json_inciter_parse_value(const char *buffer, json_inciter_element_t *element);
```

**Note:** a successful invocation of `json_inciter_parse_value` *does not* guarantee valid json.
This is because in order to keep memory consumption to a minimum nested json structures are ignored; that is, the string `"{\"this is not valid json\"}"` is parsed without issue, returning a `JSON_INCITER_ELEMENT_TAG_OBJECT` whose content is the string between curly braces.
Further invocations that evaluate the element's contents will eventually return `JSON_INCITER_INVALID`. 

Once an object or array has been found its contents can be iterated with the following functions:

```c
json_inciter_t json_inciter_parse_pair(const char *buffer, const char **key, size_t *key_len,
                                       json_inciter_element_t *element);
json_inciter_t json_inciter_next_element_start(const char *buffer, json_inciter_element_tag_t tag,
                                               const char **next_start);
const char *json_inciter_element_content_start(json_inciter_element_t element);
```

Together they can be use to traverse an arbitrary json object or array.

```c
    // ...
    do {
        json_inciter_element_t value    = {0};

        // Parsing a key-value pair in an object
        const char            *key      = NULL;
        size_t                 key_size = 0;

        json_inciter_t result = json_inciter_parse_pair(json_content, &key, &key_size, &value);

        // Or parsing a value in an array
        //json_inciter_t result = json_inciter_parse_value(json_content, &value);


        if (result == JSON_INCITER_OK && key_size == required_key_len && 
                strncmp(required_key, key, required_key_len) == 0) {
            return value;
        }

        json_content = JSON_INCITER_ELEMENT_NEXT_START(value);
        iteration_result =
            json_inciter_next_element_start(json_content, JSON_INCITER_ELEMENT_TAG_OBJECT, &json_content);
    } while (iteration_result == JSON_INCITER_OK);
```

There is an additional function that looks for a specific key in an object, `json_inciter_find_value_in_object`, which just applies the previous example.

Finally, `json_inciter_copy_content` is a utility function that copies the content of the element (e.g. the value for a string) into a provided buffer.
