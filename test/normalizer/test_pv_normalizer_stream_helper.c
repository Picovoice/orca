#include <stdlib.h>
#include <string.h>

#include "orca/normalizer/test_pv_normalizer_stream_helper.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"
#include "tokenizer/mock/pv_tokenizer_mock.h"

#endif

void test_pv_normalizer_stream_helper(
        pv_normalizer_stream_t *normalizer_stream_object,
        int32_t num_tokens,
        char **tokens,
        pv_status_t *add_statuses,
        pv_status_t flush_status) {
    for (int32_t j = 0; j < num_tokens; j++) {
        pv_normalizer_token_list_t *verbalizable_tokens = NULL;
        pv_status_t status = pv_normalizer_stream_add_to_input_buffer(
                normalizer_stream_object,
                tokens[j],
                &verbalizable_tokens);
        pv_test_true(
                status == add_statuses[j],
                "Add token to normalizer stream expected `%s` got `%s`.",
                pv_status_to_string(add_statuses[j]),
                                    pv_status_to_string(status));
        pv_normalizer_token_list_delete(verbalizable_tokens);
    }

    pv_normalizer_token_list_t *verbalizable_tokens = NULL;
    pv_status_t status = pv_normalizer_stream_flush_verbalizable_tokens(normalizer_stream_object, &verbalizable_tokens);
    pv_test_true(
            status == flush_status,
            "Flush normalizer stream expected `%s` got `%s`.",
            pv_status_to_string(flush_status),
            pv_status_to_string(status));
    pv_normalizer_token_list_delete(verbalizable_tokens);
}

static pv_status_t test_verbalizable_text(
        pv_normalizer_token_list_t *verbalizable_tokens,
        int32_t expected_idx,
        char **expected) {

    char *verbalizable_text = NULL;
    if (verbalizable_tokens != NULL) {
        pv_status_t status = pv_normalizer_token_list_to_string_text(
                verbalizable_tokens,
                &verbalizable_text);
        pv_test_true(status == PV_STATUS_SUCCESS, "Failed to convert verbalizable tokens to text");
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    if (expected[expected_idx] == NULL) {
        pv_test_true(
                verbalizable_text == NULL,
                "token_index=%d, got `%s`, expected NULL",
                expected_idx,
                verbalizable_text);
    } else {
        if (verbalizable_text == NULL) {
            pv_test_true(
                    false,
                    "token_index=%d, got NULL, expected `%s`",
                    expected_idx,
                    expected[expected_idx]);
        } else {
            pv_test_true(
                    strcmp(verbalizable_text, expected[expected_idx]) == 0,
                    "token_index=%d, Incorrect verbalizable text. Got `%s`, expected `%s`",
                    expected_idx,
                    verbalizable_text,
                    expected[expected_idx]);
        }
    }
    free(verbalizable_text);

    return PV_STATUS_SUCCESS;
}

void test_pv_normalizer_stream_helper_verbalizable(
        pv_normalizer_stream_t *normalizer_stream_object,
        int32_t num_tokens,
        char **tokens,
        char **expected) {
    for (int32_t j = 0; j < num_tokens; j++) {
        pv_normalizer_token_list_t *verbalizable_tokens = NULL;
        pv_status_t status = pv_normalizer_stream_add_to_input_buffer(
                normalizer_stream_object,
                tokens[j],
                &verbalizable_tokens);
        pv_test_true(status == PV_STATUS_SUCCESS, "Failed to add token to normalizer stream");
        if (status != PV_STATUS_SUCCESS) {
            return;
        }

        status = test_verbalizable_text(
                verbalizable_tokens,
                j,
                expected);
        pv_normalizer_token_list_delete(verbalizable_tokens);
        if (status != PV_STATUS_SUCCESS) {
            return;
        }
    }

    pv_normalizer_token_list_t *verbalizable_tokens = NULL;
    pv_status_t status = pv_normalizer_stream_flush_verbalizable_tokens(normalizer_stream_object, &verbalizable_tokens);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to flush normalizer stream");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    test_verbalizable_text(
            verbalizable_tokens,
            num_tokens,
            expected);
    pv_normalizer_token_list_delete(verbalizable_tokens);
}

static bool test_phonemizable_list(
        pv_normalizer_token_list_t *phonemizable_list,
        char *verbalized,
        int32_t expected_size_idx,
        int32_t *expected_size,
        int32_t expected_verbalized_length,
        int32_t *predicted_verbalized_length) {
    if (expected_size[expected_size_idx] == 0) {
        pv_test_true(
                (phonemizable_list == NULL) || (phonemizable_list->size == 0),
                "token_index=%d, got list of size `%d`, expected NULL or size equal to 0",
                expected_size_idx,
                phonemizable_list != NULL ? phonemizable_list->size : 0);
        return true;
    }

    pv_test_true(
            phonemizable_list != NULL,
            "token_index=%d, got NULL, expected size of `%d`",
            expected_size_idx,
            expected_size[expected_size_idx]);
    if (phonemizable_list == NULL) {
        return false;
    }
    pv_test_true(
            expected_size[expected_size_idx] == (phonemizable_list->size),
            "token_index=%d, Wrong size of phonemizable list, got `%d`, expected `%d`",
            expected_size_idx,
            phonemizable_list->size,
            expected_size[expected_size_idx]);

    if (expected_size[expected_size_idx] != (phonemizable_list->size)) {
        return false;
    }

    pv_normalizer_token_t *current = phonemizable_list->head;
    bool last_was_space = false;
    for (int32_t k = 0; k < phonemizable_list->size; k++) {
        if ((strcmp(current->string, " ") == 0)) {
            if (!last_was_space) {
                if (((*predicted_verbalized_length) + 1) >= expected_verbalized_length) {
                    pv_test_true(false, "Predicted phonemizable text is too long");
                    return false;
                }
                strcat(verbalized, " ");
                (*predicted_verbalized_length) += 1;
                last_was_space = true;
            }
        } else {
            if ((int32_t) ((*predicted_verbalized_length) + strlen(current->verbalized)) >=
                expected_verbalized_length) {
                pv_test_true(false, "Predicted phonemizable text is too long");
                return false;
            }
            if (current->tag_language_agnostic != PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
                strcat(verbalized, current->verbalized);
            } else {
                strcat(verbalized, "{");
                strcat(verbalized, "|");
                strcat(verbalized, current->pronunciation);
                strcat(verbalized, "}");
            }
            (*predicted_verbalized_length) += (int32_t) strlen(current->verbalized);
            last_was_space = false;

            if ((k < (phonemizable_list->size - 1))) {
                if (((*predicted_verbalized_length) + 1) >= expected_verbalized_length) {
                    pv_test_true(false, "Predicted phonemizable text is too long");
                    return false;
                }
                strcat(verbalized, " ");
                (*predicted_verbalized_length) += 1;
                last_was_space = true;
            }
        }

        current = current->next;
    }

    return true;
}

void test_pv_normalizer_stream_helper_phonemizable(
        pv_normalizer_stream_t *normalizer_stream_object,
        int32_t num_tokens,
        char **tokens,
        int32_t *expected_size,
        const char *expected) {
    int32_t expected_verbalized_length = (int32_t) strlen(expected) + 1;
    char *verbalized = calloc(expected_verbalized_length, sizeof(char));
    pv_test_true(verbalized != NULL, "Failed to allocate memory for expected phonemizable text");
    if (!verbalized) {
        return;
    }

    int32_t predicted_verbalized_length = 0;
    for (int32_t j = 0; j < num_tokens; j++) {
        pv_normalizer_token_list_t *phonemizable_list = NULL;
        pv_status_t status = pv_normalizer_stream_add(
                normalizer_stream_object,
                tokens[j],
                &phonemizable_list);

        pv_test_true(status == PV_STATUS_SUCCESS, "Failed to add token to normalizer stream");
        if (status != PV_STATUS_SUCCESS) {
            free(verbalized);
            return;
        }

        bool pass = test_phonemizable_list(
                phonemizable_list,
                verbalized,
                j,
                expected_size,
                expected_verbalized_length,
                &predicted_verbalized_length);
        pv_normalizer_token_list_delete(phonemizable_list);
        if (!pass) {
            free(verbalized);
            return;
        }
    }

    pv_normalizer_token_list_t *phonemizable_list = NULL;
    pv_status_t status = pv_normalizer_stream_flush(normalizer_stream_object, &phonemizable_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to add token to normalizer stream");
    if (status != PV_STATUS_SUCCESS) {
        free(verbalized);
        return;
    }

    bool pass = test_phonemizable_list(
            phonemizable_list,
            verbalized,
            num_tokens,
            expected_size,
            expected_verbalized_length,
            &predicted_verbalized_length);
    pv_normalizer_token_list_delete(phonemizable_list);
    if (!pass) {
        free(verbalized);
        return;
    }

    pv_test_true(
            strcmp(verbalized, expected) == 0,
            "Incorrect phonemizable text. Got `%s`, expected `%s`",
            verbalized,
            expected);
    free(verbalized);
}
