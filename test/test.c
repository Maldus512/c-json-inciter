#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "json_inciter.h"


#define ASSERT_ELEMENT(Result, Tag, Start, Length)                                                                     \
    assert_int_equal(Result.tag, Tag);                                                                                 \
    assert_int_equal(Result.start, Start);                                                                             \
    assert_int_equal(Result.length, Length);

#define ASSERT_NUMBER_RESULT(Result, Tag, Start, Length, Number)                                                       \
    assert_int_equal(Result.tag, Tag);                                                                                 \
    assert_int_equal(Result.start, Start);                                                                             \
    assert_int_equal(Result.length, Length);                                                                           \
    assert_int_equal(Result.as.number, Number);


/* These functions will be used to initialize
   and clean resources up after each test run */
int setup(void **state) {
    (void)state;
    return 0;
}

int teardown(void **state) {
    (void)state;
    return 0;
}


static void test_json_inciter_null(void **state) {
    (void)state;
    json_inciter_t         result  = JSON_INCITER_OK;
    json_inciter_element_t element = {0};

    char        content_buffer[32] = {0};
    const char *json_buffer        = NULL;

    // Clean value
    json_buffer = "null";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_NULL, &json_buffer[0], 4);
    assert_int_equal(element.length, 4);
    json_inciter_copy_content(content_buffer, sizeof(content_buffer), element);
    assert_true(strcmp(content_buffer, "null") == 0);

    // Whitespace
    json_buffer = "   null";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_NULL, &json_buffer[3], 4);
    assert_int_equal(element.length, 4);
    json_inciter_copy_content(content_buffer, sizeof(content_buffer), element);
    assert_true(strcmp(content_buffer, "null") == 0);

    // Acceptable clutter
    json_buffer = "null ;cndsajkl";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_NULL, &json_buffer[0], 4);
    assert_int_equal(element.length, 4);

    // Invalid clutter
    json_buffer = "nullcndsajkl";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INVALID);

    json_buffer = "rubbish";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INVALID);

    // Incomplete
    json_buffer = "nul";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INCOMPLETE);
}


static void test_json_inciter_true(void **state) {
    (void)state;
    json_inciter_t         result      = JSON_INCITER_OK;
    json_inciter_element_t element     = {0};
    const char            *json_buffer = NULL;

    // Clean value
    json_buffer = "true";
    result      = json_inciter_parse_value(json_buffer, &element);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_TRUE, &json_buffer[0], 4);
    assert_int_equal(element.length, 4);

    // Whitespace
    json_buffer = "    true";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_TRUE, &json_buffer[4], 4);
    assert_int_equal(element.length, 4);

    // Acceptable clutter
    json_buffer = "true ;'pcdsa";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_TRUE, &json_buffer[0], 4);
    assert_int_equal(element.length, 4);

    // Invalid clutter
    json_buffer = "truecdsah";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INVALID);

    json_buffer = "rubbish";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INVALID);

    // Incomplete
    json_buffer = "tr";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INCOMPLETE);
}


static void test_json_inciter_false(void **state) {
    (void)state;
    json_inciter_t         result      = JSON_INCITER_OK;
    json_inciter_element_t element     = {0};
    const char            *json_buffer = NULL;

    // Clean value
    json_buffer = "false";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_FALSE, &json_buffer[0], 5);
    assert_int_equal(element.length, 5);

    // Whitespace
    json_buffer = "    false";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_FALSE, &json_buffer[4], 5);
    assert_int_equal(element.length, 5);

    // Acceptable clutter
    json_buffer = "false [csaslnj";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_FALSE, &json_buffer[0], 5);
    assert_int_equal(element.length, 5);

    // Invalid clutter
    json_buffer = "falseeeee";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INVALID);

    json_buffer = "rubbish";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INVALID);

    // Incomplete
    json_buffer = "fals";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INCOMPLETE);
}


static void test_json_inciter_number(void **state) {
    (void)state;
    json_inciter_t         result      = JSON_INCITER_OK;
    json_inciter_element_t element     = {0};
    const char            *json_buffer = NULL;

    // Clean values
    json_buffer = "42";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_NUMBER_RESULT(element, JSON_INCITER_ELEMENT_TAG_NUMBER, &json_buffer[0], 2, 42);
    assert_int_equal(element.length, 2);

    json_buffer = "-64";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_NUMBER_RESULT(element, JSON_INCITER_ELEMENT_TAG_NUMBER, &json_buffer[0], 3, -64);
    assert_int_equal(element.length, 3);

    json_buffer = "4096.512";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_NUMBER_RESULT(element, JSON_INCITER_ELEMENT_TAG_NUMBER, &json_buffer[0], 8, 4096.512);
    assert_int_equal(element.length, 8);

    // Invalid
    json_buffer = "hello";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INVALID);

    json_buffer = "123fourfive";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INVALID);
}


static void test_json_inciter_string(void **state) {
    (void)state;
    json_inciter_t         result      = JSON_INCITER_OK;
    json_inciter_element_t element     = {0};
    const char            *json_buffer = NULL;

    // Clean values
    json_buffer = "\"string\"";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_STRING, &json_buffer[0], 8);
    assert_true(strncmp(element.as.string, "string", strlen("string")) == 0);
    assert_int_equal(element.length, 8);
    assert_int_equal(JSON_INCITER_STRING_LENGTH(element), 6);

    json_buffer = "   \"hello\"";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_STRING, &json_buffer[3], 7);
    assert_true(strncmp(element.as.string, "hello", strlen("hello")) == 0);
    assert_int_equal(element.length, 7);
    assert_int_equal(JSON_INCITER_STRING_LENGTH(element), 5);

    json_buffer = "\"this is a \\\"string\\\"\"";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_STRING, &json_buffer[0], 22);
    assert_true(strncmp(element.as.string, "this is a \\\"string\\\"", strlen("this is a \\\"string\\\"")) == 0);
    assert_int_equal(element.length, 22);
    assert_int_equal(JSON_INCITER_STRING_LENGTH(element), 20);

    // Incomplete
    json_buffer = "\"string";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_INCOMPLETE);
}


static void test_json_inciter_array(void **state) {
    (void)state;
    json_inciter_t         result      = JSON_INCITER_OK;
    json_inciter_element_t element     = {0};
    const char            *json_buffer = NULL;

    // Clean values
    json_buffer = "[1,2, 3]";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_ARRAY, &json_buffer[0], 8);

    json_buffer = "[1,{\"one\":\"uno\", \"two]\":\"du[e]\"}, [\"string[27]\", \"mydata\"],2,3]";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_ARRAY, &json_buffer[0], 63);
}


static void test_json_inciter_object(void **state) {
    (void)state;
    json_inciter_t         result      = JSON_INCITER_OK;
    json_inciter_element_t element     = {0};
    const char            *json_buffer = NULL;

    // Clean values
    json_buffer = "{\"one\":1,\"two\":2, \"three\":3}";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_OBJECT, &json_buffer[0], 0x1c);

    json_buffer = "{\"stuff\":1, \"deep\":{\"one\":\"uno\", \"two]\":\"du[e]\"}, [\"string[27]\", \"mydata\"]}";
    result      = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_OBJECT, &json_buffer[0], 0x4b);

    json_buffer =
        "{\"useless\":0, \"uninteresting\":[1,2,3], \"important\":\"data\", \"irrelevant\":{\"stuff\" : null}}";
    result = json_inciter_parse_value(json_buffer, &element);
    assert_int_equal(result, JSON_INCITER_OK);
    ASSERT_ELEMENT(element, JSON_INCITER_ELEMENT_TAG_OBJECT, &json_buffer[0], 0x59);

    json_inciter_element_t value = {0};
    result                       = json_inciter_find_value_in_object(element, "important", &value);
    assert_int_equal(result, JSON_INCITER_OK);
    assert_int_equal(value.tag, JSON_INCITER_ELEMENT_TAG_STRING);
    char content_buffer[32] = {0};
    json_inciter_copy_content(content_buffer, sizeof(content_buffer), value);
    assert_true(strcmp(content_buffer, "data"));
}


static void test_json_inciter_api_extraction(void **state) {
    (void)state;

    char *json_content = NULL;

    FILE *fp = fopen("api.json", "r");
    assert(fp != NULL);
    fseek(fp, 0L, SEEK_END);
    size_t total = ftell(fp) + 1;
    json_content = malloc(total);
    assert(json_content != NULL);
    memset(json_content, 0, total);
    rewind(fp);

    fread(json_content, 1, total, fp);

    json_inciter_element_t root = {0};
    assert_int_equal(json_inciter_parse_value(json_content, &root), JSON_INCITER_OK);
    assert_int_equal(root.tag, JSON_INCITER_ELEMENT_TAG_OBJECT);

    json_inciter_element_t name_element = {0};
    assert_int_equal(json_inciter_find_value_in_object(root, "missing", &name_element), JSON_INCITER_DONE);
    assert_int_equal(json_inciter_find_value_in_object(root, "name", &name_element), JSON_INCITER_OK);
    assert_int_equal(name_element.tag, JSON_INCITER_ELEMENT_TAG_STRING);

    json_inciter_element_t assets_element = {0};
    assert_int_equal(json_inciter_find_value_in_object(root, "assets", &assets_element), JSON_INCITER_OK);
    assert_int_equal(assets_element.tag, JSON_INCITER_ELEMENT_TAG_ARRAY);

    json_inciter_t json_inciter_state = JSON_INCITER_OK;
    const char    *assets_buffer      = json_inciter_element_content_start(assets_element);
    do {
        json_inciter_element_t value = {0};
        assert_int_equal(json_inciter_parse_value(assets_buffer, &value), JSON_INCITER_OK);

        assert_int_equal(value.tag, JSON_INCITER_ELEMENT_TAG_OBJECT);

        json_inciter_element_t download_url_element = {0};
        assert_int_equal(json_inciter_find_value_in_object(value, "browser_download_url", &download_url_element),
                         JSON_INCITER_OK);

        assets_buffer = JSON_INCITER_ELEMENT_NEXT_START(value);
        json_inciter_state =
            json_inciter_next_element_start(assets_buffer, JSON_INCITER_ELEMENT_TAG_ARRAY, &assets_buffer);
    } while (json_inciter_state == JSON_INCITER_OK);

    assert_int_equal(json_inciter_state, JSON_INCITER_DONE);
}



int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_json_inciter_null),   cmocka_unit_test(test_json_inciter_true),
        cmocka_unit_test(test_json_inciter_false),  cmocka_unit_test(test_json_inciter_number),
        cmocka_unit_test(test_json_inciter_string), cmocka_unit_test(test_json_inciter_array),
        cmocka_unit_test(test_json_inciter_object), cmocka_unit_test(test_json_inciter_api_extraction),
    };

    /* If setup and teardown functions are not
       needed, then NULL may be passed instead */

    int count_fail_tests = cmocka_run_group_tests(tests, setup, teardown);

    return count_fail_tests;
}
