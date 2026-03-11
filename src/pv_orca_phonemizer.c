#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "hippo/pv_hippo_internal.h"
#include "io/pv_log.h"
#include "lm/pv_dict.h"
#include "normalizer/pv_normalizer_token.h"
#include "normalizer/pv_normalizer_util.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_language_data.h"
#include "orca/pv_orca_phonemizer.h"
#include "util/pv_check_status.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_status_t pv_orca_phonemizer_pronounce_character(
        const pv_orca_phonemizer_t *object,
        char *character,
        char **string);

static pv_status_t pv_orca_phonemizer_spell_out_helper(
        const pv_orca_phonemizer_t *object,
        const char *string,
        bool dry_run,
        int32_t *length,
        char *verbalized);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_param_serialize)(const pv_orca_phonemizer_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t count = fwrite(&(param->add_eos_punctuation), sizeof(bool), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->add_bos_phoneme), sizeof(bool), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->add_eos_phoneme), sizeof(bool), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->add_word_boundary_phoneme), sizeof(bool), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->num_phoneme_multiplier), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_param_load)(FILE *f, pv_orca_phonemizer_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_phonemizer_param_t *p = calloc(1, sizeof(pv_orca_phonemizer_param_t));
    PV_CHECK_ALLOC(p);

    size_t count = pv_fread(&(p->add_eos_punctuation), sizeof(bool), 1, f);
    if (count != 1) {
        pv_orca_phonemizer_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread(&(p->add_bos_phoneme), sizeof(bool), 1, f);
    if (count != 1) {
        pv_orca_phonemizer_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread(&(p->add_eos_phoneme), sizeof(bool), 1, f);
    if (count != 1) {
        pv_orca_phonemizer_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread(&(p->add_word_boundary_phoneme), sizeof(bool), 1, f);
    if (count != 1) {
        pv_orca_phonemizer_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread(&(p->num_phoneme_multiplier), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_orca_phonemizer_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_phoneme_multiplier <= 0) {
        pv_orca_phonemizer_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_phonemizer_param_delete)(pv_orca_phonemizer_param_t *param) {
    if (param) {
        free(param);
    }
}

struct pv_orca_phonemizer {
    const pv_orca_phonemizer_param_t *param;
    pv_hippo_t *hippo;
    pv_lexicon_t *lexicon;
    pv_dict_t *dict;
    pv_heteronym_tree_t *heteronym_tree;

    pv_language_info_t *language_info;

    int32_t num_phonemes;
    int32_t num_encoded_phonemes;

    int32_t terminator_index;
    int32_t word_boundary_phoneme_index;
    int32_t bos_phoneme_index;
    int32_t eos_phoneme_index;

    int32_t num_phoneme_multiplier;

    int32_t num_letters_alphabet;
    const char *const *alphabet_letters;
    const char *const *alphabet_pronunciations;
};

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_init)(
        const pv_orca_phonemizer_param_t *param,
        pv_hippo_t *hippo,
        pv_lexicon_t *lexicon,
        pv_dict_t *dict,
        pv_heteronym_tree_t *heteronym_tree,
        pv_language_info_t *language_info,
        pv_orca_phonemizer_t **object) {
    PV_ASSERT(hippo);
    PV_ASSERT(lexicon);
    PV_ASSERT(dict);
    PV_ASSERT(language_info);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_phonemizer_t *o = calloc(1, sizeof(pv_orca_phonemizer_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    o->hippo = hippo;
    o->lexicon = lexicon;
    o->dict = dict;
    o->language_info = language_info;
    o->heteronym_tree = heteronym_tree;

    o->num_phoneme_multiplier = param->num_phoneme_multiplier;

    o->word_boundary_phoneme_index = -1;
    o->bos_phoneme_index = -1;
    o->eos_phoneme_index = -1;
    o->terminator_index = -1;

    o->num_phonemes = pv_language_info_num_phonemes(o->language_info);
    o->num_encoded_phonemes = (o->num_phonemes * o->num_phoneme_multiplier);

    if (param->add_bos_phoneme || param->add_eos_phoneme || param->add_word_boundary_phoneme) {

        int32_t offset = 0;
        if (param->add_word_boundary_phoneme) {
            o->word_boundary_phoneme_index = o->num_encoded_phonemes;
            offset += 1;
        }
        if (param->add_bos_phoneme) {
            o->bos_phoneme_index = o->num_encoded_phonemes + offset;
            offset += 1;
        }
        if (param->add_eos_phoneme) {
            o->eos_phoneme_index = o->num_encoded_phonemes + offset;
        }
    }

    if (param->add_eos_punctuation) {
        int32_t terminator_index = 0;
        pv_status_t status = pv_orca_phonemizer_get_terminator_index(o->language_info, &terminator_index);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_phonemizer_get_terminator_index,
                    pv_status_to_string(status));
            free(o);
            return status;
        }
        // Because `pv_language_info_phoneme_index_from_string()` plus 1 internally, so need to minus 1 to cancel out.
        o->terminator_index = terminator_index - 1;
    }

    pv_normalizer_language_t language = 0;
    pv_status_t status = pv_normalizer_util_infer_language_from_language_info(o->language_info, &language);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_infer_language_from_language_info,
                pv_status_to_string(status));
        free(o);
        return status;
    }

    status = pv_orca_phonemizer_initialize_alphabet(
            language,
            &(o->num_letters_alphabet),
            &(o->alphabet_letters),
            &(o->alphabet_pronunciations));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_initialize_alphabet,
                pv_status_to_string(status));
        free(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_initialize_alphabet)(
        const pv_normalizer_language_t language,
        int32_t *num_letters_alphabet,
        const char *const **alphabet_letters,
        const char *const **alphabet_pronunciations) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(num_letters_alphabet);
    PV_ASSERT(alphabet_letters);
    PV_ASSERT(alphabet_pronunciations);

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *num_letters_alphabet = PV_ARRAY_LEN(PV_ORCA_PHONEMIZER_EN_ALPHABET_UPPERCASE_LETTERS);
            *alphabet_letters = PV_ORCA_PHONEMIZER_EN_ALPHABET_UPPERCASE_LETTERS;
            *alphabet_pronunciations = PV_ORCA_PHONEMIZER_EN_ALPHABET_PRONUNCIATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *num_letters_alphabet = PV_ARRAY_LEN(PV_ORCA_PHONEMIZER_DE_ALPHABET_UPPERCASE_LETTERS);
            *alphabet_letters = PV_ORCA_PHONEMIZER_DE_ALPHABET_UPPERCASE_LETTERS;
            *alphabet_pronunciations = PV_ORCA_PHONEMIZER_DE_ALPHABET_PRONUNCIATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *num_letters_alphabet = PV_ARRAY_LEN(PV_ORCA_PHONEMIZER_FR_ALPHABET_UPPERCASE_LETTERS);
            *alphabet_letters = PV_ORCA_PHONEMIZER_FR_ALPHABET_UPPERCASE_LETTERS;
            *alphabet_pronunciations = PV_ORCA_PHONEMIZER_FR_ALPHABET_PRONUNCIATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *num_letters_alphabet = PV_ARRAY_LEN(PV_ORCA_PHONEMIZER_ES_ALPHABET_UPPERCASE_LETTERS);
            *alphabet_letters = PV_ORCA_PHONEMIZER_ES_ALPHABET_UPPERCASE_LETTERS;
            *alphabet_pronunciations = PV_ORCA_PHONEMIZER_ES_ALPHABET_PRONUNCIATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *num_letters_alphabet = PV_ARRAY_LEN(PV_ORCA_PHONEMIZER_IT_ALPHABET_UPPERCASE_LETTERS);
            *alphabet_letters = PV_ORCA_PHONEMIZER_IT_ALPHABET_UPPERCASE_LETTERS;
            *alphabet_pronunciations = PV_ORCA_PHONEMIZER_IT_ALPHABET_PRONUNCIATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *num_letters_alphabet = PV_ARRAY_LEN(PV_ORCA_PHONEMIZER_PT_ALPHABET_UPPERCASE_LETTERS);
            *alphabet_letters = PV_ORCA_PHONEMIZER_PT_ALPHABET_UPPERCASE_LETTERS;
            *alphabet_pronunciations = PV_ORCA_PHONEMIZER_PT_ALPHABET_PRONUNCIATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_KO:
        case PV_NORMALIZER_LANGUAGE_JA: {
            *num_letters_alphabet = 0;
            *alphabet_letters = NULL;
            *alphabet_pronunciations = NULL;
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_phonemizer_delete)(pv_orca_phonemizer_t *object) {
    if (object) {
        free(object);
    }
}

int32_t PV_MOCKABLE(pv_orca_phonemizer_get_word_boundary_index)(const pv_orca_phonemizer_t *object) {
    PV_ASSERT(object);

    return object->word_boundary_phoneme_index;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_punctuation_end_indices)(
        const pv_orca_phonemizer_t *object,
        const int32_t num_punctuation_chars,
        const char **punctuation_chars,
        int32_t **punctuation_indices) {
    PV_ASSERT(object);
    PV_ASSERT(num_punctuation_chars >= 0);
    PV_ASSERT(punctuation_chars);
    PV_ASSERT(punctuation_indices);

    *punctuation_indices = NULL;

    int32_t *punctuation_indices_internal = malloc(num_punctuation_chars * sizeof(int32_t));
    if (!punctuation_indices_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("num_punctuation_indices_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < num_punctuation_chars; i++) {
        const char *current = punctuation_chars[i];

        int32_t num_punctuation_index = 0;
        pv_status_t status = pv_language_info_phoneme_index_from_string(
                object->language_info,
                current,
                &num_punctuation_index);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_info_phoneme_index_from_string,
                    pv_status_to_string(status));
            free(punctuation_indices_internal);
            return status;
        }

        if (object->num_phoneme_multiplier == 2) {
            punctuation_indices_internal[i] = (2 * num_punctuation_index) + 1 - object->num_phoneme_multiplier;
        } else {
            punctuation_indices_internal[i] = num_punctuation_index;
        }
    }

    *punctuation_indices = punctuation_indices_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_terminator_index)(
        const pv_language_info_t *language_info,
        int32_t *terminator_index) {
    PV_ASSERT(language_info);
    PV_ASSERT(terminator_index);

    const char *terminator = NULL;
    pv_status_t status = pv_language_info_terminator_index_to_string(language_info, 0, &terminator);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_info_terminator_index_to_string,
                pv_status_to_string(status));
        return status;
    }
    status = pv_language_info_phoneme_index_from_string(language_info, terminator, terminator_index);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_info_phoneme_index_from_string,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_orca_phonemizer_pronounce_character(
        const pv_orca_phonemizer_t *object,
        char *character,
        char **string) {
    PV_ASSERT(character);
    PV_ASSERT(string);

    *string = NULL;

    pv_status_t status = pv_normalizer_util_upper_inplace(character);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_upper_inplace,
                pv_status_to_string(status));
        return status;
    }

    int32_t index = 0;
    for (int32_t i = 0; i < object->num_letters_alphabet; i++) {
        if (strcmp(character, object->alphabet_letters[i]) == 0) {
            index = i;
            break;
        }
    }

    *string = calloc(strlen(object->alphabet_pronunciations[index]) + 1, sizeof(char));
    if (!(*string)) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("*string"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    strcpy(*string, object->alphabet_pronunciations[index]);

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_orca_phonemizer_spell_out_helper(
        const pv_orca_phonemizer_t *object,
        const char *string,
        bool dry_run,
        int32_t *length,
        char *verbalized) {
    PV_ASSERT(string);
    PV_ASSERT(!dry_run || length);

    if (dry_run) {
        PV_ASSERT(length);
        *length = 0;
    } else {
        PV_ASSERT(verbalized);
    }

    int32_t length_internal = 0;
    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
    int32_t verbalized_index = 0;
    int32_t i = 0;

    while (i < (int32_t) strlen(string)) {
        int32_t num_bytes_character = 0;
        pv_status_t status = pv_language_num_bytes_character((unsigned char) string[i], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            return status;
        }
        for (int32_t j = 0; j < num_bytes_character; j++) {
            character[j] = string[i + j];
        }
        character[num_bytes_character] = '\0';

        char *pronunciation = NULL;
        status = pv_orca_phonemizer_pronounce_character(object, character, &pronunciation);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_phonemizer_pronounce_character,
                    pv_status_to_string(status));
            return status;
        }

        if (dry_run) {
            length_internal += (int32_t) strlen(pronunciation);
        } else {
            strcpy(verbalized + verbalized_index, pronunciation);
            verbalized_index += (int32_t) strlen(pronunciation);
        }

        free(pronunciation);

        if (i < (int32_t) strlen(string) - num_bytes_character) {
            if (dry_run) {
                ++length_internal;
            } else {
                verbalized[verbalized_index] = ' ';
                ++verbalized_index;
            }
        }

        i += num_bytes_character;
    }

    if (dry_run) {
        *length = length_internal;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_spell_out)(
        const pv_orca_phonemizer_t *object,
        const char *string,
        char **pronunciation) {
    PV_ASSERT(string);
    PV_ASSERT(pronunciation);

    *pronunciation = NULL;

    int32_t length = 0;
    pv_status_t status = pv_orca_phonemizer_spell_out_helper(object, string, true, &length, NULL);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_spell_out_helper,
                pv_status_to_string(status));
        return status;
    }

    char *pronunciation_internal = calloc(length + 1, sizeof(char));
    if (!pronunciation_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pronunciation"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_orca_phonemizer_spell_out_helper(object, string, false, NULL, pronunciation_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_spell_out_helper,
                pv_status_to_string(status));
        free(pronunciation_internal);
        return status;
    }

    *pronunciation = pronunciation_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_phonemes_hippo)(
        const pv_orca_phonemizer_t *object,
        const char *word,
        int32_t *num_phonemes,
        int32_t **phonemes) {
    PV_ASSERT(object);
    PV_ASSERT(word);
    PV_ASSERT(num_phonemes);
    PV_ASSERT(phonemes);

    *num_phonemes = 0;
    *phonemes = NULL;

    pv_hippo_t *hippo = object->hippo;

    int32_t num_pronunciations = 0;
    int32_t *num_pronunciations_phonemes = NULL;
    const char ***pronunciations = NULL;
    float *probabilities = NULL;
    pv_status_t status = pv_hippo_pronounce(
            hippo,
            word,
            &num_pronunciations,
            &num_pronunciations_phonemes,
            &pronunciations,
            &probabilities);
    free(probabilities);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_hippo_pronounce,
                pv_status_to_string(status));
        return status;
    }

    *num_phonemes = num_pronunciations_phonemes[0];
    *phonemes = malloc(sizeof(int32_t) * num_pronunciations_phonemes[0]);
    if (!(*phonemes)) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc_array_index,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("phonemes", 0));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < num_pronunciations_phonemes[0]; i++) {
        status = pv_language_info_phoneme_index_from_string(
                pv_hippo_language_info(hippo),
                pronunciations[0][i],
                *phonemes + i);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_info_phoneme_index_from_string,
                    pv_status_to_string(status));
            free(*phonemes);
            return status;
        }
    }

#if defined(__ORCA_PHONEMIZER_LOG_DEBUG__)

    LOG_DEBUG_INLINE("[Phonemizer] `hippo` Token, Phonemes: `%s`, `", word);
    for (int32_t i = 0; i < *num_phonemes; i++) {
        const char *character = NULL;
        pv_status_t status = pv_language_info_phoneme_index_to_string(
                object->language_info,
                (*phonemes)[i],
                &character);
        LOG_DEBUG_INLINE("%s-", character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_info_phoneme_index_to_string,
                    pv_status_to_string(status));
            free(*phonemes);
            return status;
        }
    }
    LOG_DEBUG_INLINE_SIMPLE("`\n");

#endif

    free(num_pronunciations_phonemes);
    for (int32_t i = 0; i < num_pronunciations; i++) {
        free(pronunciations[i]);
    }
    if (pronunciations) {
        free(pronunciations);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_phonemes_lexicon)(
        const pv_orca_phonemizer_t *object,
        const char *token,
        int32_t *num_phonemes,
        int32_t **phonemes) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(num_phonemes);
    PV_ASSERT(phonemes);

    *num_phonemes = 0;
    *phonemes = NULL;

    int32_t num_word_phonemes = 0;
    const int32_t *word_phonemes = NULL;
    pv_dict_pronunciation(
            object->dict,
            pv_lexicon_encode(object->lexicon, token),
            0,
            &num_word_phonemes,
            &word_phonemes);

    *num_phonemes = num_word_phonemes;
    *phonemes = malloc(sizeof(int32_t) * num_word_phonemes);
    if (!(*phonemes)) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc_array_index,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("phonemes", 0));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memcpy(*phonemes, word_phonemes, sizeof(int32_t) * num_word_phonemes);

#if defined(__ORCA_PHONEMIZER_LOG_DEBUG__)

    LOG_DEBUG_INLINE("[Phonemizer] `lexicon` Token, Phonemes: `%s`, `", token);
    for (int32_t i = 0; i < *num_phonemes; i++) {
        const char *character = NULL;
        pv_status_t status = pv_language_info_phoneme_index_to_string(
                object->language_info,
                (*phonemes)[i],
                &character);
        LOG_DEBUG_INLINE("%s-", character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_info_phoneme_index_to_string,
                    pv_status_to_string(status));
            free(*phonemes);
            return status;
        }
    }
    LOG_DEBUG_INLINE_SIMPLE("`\n");

#endif

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_phonemes_heteronym)(
        const pv_orca_phonemizer_t *object,
        int32_t heteronym_token_index,
        const pv_normalizer_token_t **start_text_token,
        int32_t num_prev_text_tokens,
        const pv_normalizer_token_t **prev_text_tokens,
        int32_t *num_phonemes,
        int32_t **phonemes) {
    PV_ASSERT(object);
    PV_ASSERT(heteronym_token_index >= 0);
    PV_ASSERT(start_text_token);
    PV_ASSERT(num_phonemes);
    PV_ASSERT(phonemes);
    PV_ASSERT(num_prev_text_tokens >= 0);
    if (num_prev_text_tokens != 0) {
        PV_ASSERT(prev_text_tokens);
    }

    *num_phonemes = 0;
    *phonemes = NULL;

    int32_t lexicon_words[num_prev_text_tokens + heteronym_token_index + 1];
    char *heteronym = start_text_token[heteronym_token_index]->verbalized;

    int32_t lexicon_word_index = 0;
    for (int32_t i = heteronym_token_index; i >= 0; i--) {
        char *verbalized = start_text_token[i]->verbalized;
        if (verbalized == NULL) {
            continue;
        }
        int32_t word = pv_lexicon_encode(object->lexicon, verbalized);
        if (word != -1) {
            lexicon_words[lexicon_word_index] = word;
        } else {
            lexicon_words[lexicon_word_index] = pv_lexicon_encode(object->lexicon, "*");
        }
        lexicon_word_index += 1;
    }
    for (int32_t i = num_prev_text_tokens - 1; i >= 0; i--) {
        char *verbalized = prev_text_tokens[i]->verbalized;
        if (verbalized == NULL) {
            continue;
        }
        int32_t word = pv_lexicon_encode(object->lexicon, verbalized);
        if (word != -1) {
            lexicon_words[lexicon_word_index] = word;
        } else {
            lexicon_words[lexicon_word_index] = pv_lexicon_encode(object->lexicon, "*");
        }
        lexicon_word_index += 1;
    }

    int32_t phoneme_index = 0;
    pv_heteronym_tree_pronunciation(
            object->heteronym_tree,
            lexicon_words,
            lexicon_word_index,
            &phoneme_index);
    if (phoneme_index < 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("phoneme_index"));
        return PV_STATUS_INVALID_ARGUMENT;
    }

    int32_t num_word_phonemes = 0;
    const int32_t *word_phonemes = NULL;
    pv_dict_pronunciation(
            object->dict,
            pv_lexicon_encode(object->lexicon, heteronym),
            phoneme_index,
            &num_word_phonemes,
            &word_phonemes);
    if (num_word_phonemes == -1 || num_word_phonemes == 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("num_word_phonemes"));
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *num_phonemes = num_word_phonemes;
    *phonemes = malloc(sizeof(int32_t) * num_word_phonemes);
    if (!(*phonemes)) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc_array_index,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("phonemes", 0));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memcpy(*phonemes, word_phonemes, sizeof(int32_t) * num_word_phonemes);

#if defined(__ORCA_PHONEMIZER_LOG_DEBUG__)

    LOG_DEBUG_INLINE("[Phonemizer] `lexicon` Token, Phonemes for heteronym: `%s`, `", heteronym);
    for (int32_t i = 0; i < *num_phonemes; i++) {
        const char *character = NULL;
        pv_status_t status = pv_language_info_phoneme_index_to_string(
                object->language_info,
                (*phonemes)[i],
                &character);
        LOG_DEBUG_INLINE("%s-", character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_info_phoneme_index_to_string,
                    pv_status_to_string(status));
            free(*phonemes);
            return status;
        }
    }
    LOG_DEBUG_INLINE_SIMPLE("`\n");

#endif

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_phonemes_pronunciation)(
        const pv_orca_phonemizer_t *object,
        const char *token,
        int32_t *num_phonemes,
        int32_t **phonemes) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(num_phonemes);
    PV_ASSERT(phonemes);

    *phonemes = NULL;
    *num_phonemes = 0;

    size_t length = strlen(token);
    if (length == 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("token"));
        return PV_STATUS_INVALID_ARGUMENT;
    }

    int32_t num_phonemes_internal = 0;
    for (size_t i = 0; i < length; i++) {
        if (token[i] == ' ') {
            num_phonemes_internal++;
        }
    }
    num_phonemes_internal++;

    int32_t *phonemes_internal = malloc(sizeof(int32_t) * num_phonemes_internal);
    if (!phonemes_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc_array_index,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("phonemes_internal", 0));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t start_index = 0;
    int32_t end_index = 0;
    int32_t phoneme_index = 0;
    for (size_t i = 0; i <= length; i++) {
        bool is_space = token[i] == ' ';
        bool is_eos = (token[i] == '\0') || (i == length);

        if (is_space || is_eos) {
            size_t length_phoneme = end_index - start_index;
            char *ph = malloc((length_phoneme + 1) * sizeof(char));
            if (!ph) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token"));
                free(phonemes_internal);
                return PV_STATUS_OUT_OF_MEMORY;
            }

            memcpy(ph, token + start_index, length_phoneme);
            ph[length_phoneme] = '\0';

            pv_status_t status = pv_language_info_phoneme_index_from_string(
                    object->language_info,
                    ph,
                    phonemes_internal + phoneme_index);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT(
                        &pv_error_msg_orca_invalid_custom_pronunciation_phoneme,
                        PV_ERROR_ARGS_PUBLIC(ph),
                        PV_ERROR_ARGS_PRIVATE_EMPTY());
                free(ph);
                free(phonemes_internal);
                return status;
            }
            free(ph);
            phoneme_index++;
            start_index = end_index + 1;
            end_index = start_index;
        } else {
            end_index++;
        }
    }

#ifdef __ORCA_PHONEMIZER_LOG_DEBUG__

    LOG_DEBUG_INLINE("[Phonemizer] `custom pron` Token, Phonemes: `%s`, `", token);
    for (int32_t i = 0; i < *num_phonemes; i++) {
        const char *character = NULL;
        pv_status_t status = pv_language_info_phoneme_index_to_string(
                object->language_info,
                phonemes_internal[i],
                &character);
        LOG_DEBUG_INLINE("%s-", character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_info_phoneme_index_to_string,
                    pv_status_to_string(status));
            free(phonemes_internal);
            return status;
        }
    }
    LOG_DEBUG_INLINE_SIMPLE("`\n");

#endif

    *num_phonemes = num_phonemes_internal;
    *phonemes = phonemes_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_phonemes_spell_out)(
        const pv_orca_phonemizer_t *object,
        pv_normalizer_token_t *token,
        int32_t *num_phonemes,
        int32_t **phonemes) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(num_phonemes);
    PV_ASSERT(phonemes);

    *phonemes = NULL;
    *num_phonemes = 0;

    char *pronunciation = NULL;
    pv_status_t status = pv_orca_phonemizer_spell_out(object, token->string, &pronunciation);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_spell_out,
                pv_status_to_string(status));
        return status;
    }
    token->pronunciation = pronunciation;

    status = pv_orca_phonemizer_get_phonemes_pronunciation(
            object,
            token->pronunciation,
            num_phonemes,
            phonemes);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_get_phonemes_pronunciation,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_orca_phonemizer_get_num_phonemes_per_text_token(
        const pv_orca_phonemizer_t *object,
        const int32_t num_text_tokens,
        const int32_t add_terminator_manually,
        bool allow_prepend_bos,
        bool allow_append_eos,
        const int32_t *num_phonemes_buffer,
        const int32_t **phonemes_buffer,
        int32_t **text_tokens_num_encoded_phonemes) {
    PV_ASSERT(object);
    PV_ASSERT(num_text_tokens >= 0);
    PV_ASSERT(add_terminator_manually >= 0);
    PV_ASSERT(add_terminator_manually <= 1);
    PV_ASSERT(num_phonemes_buffer);
    PV_ASSERT(text_tokens_num_encoded_phonemes);

    PV_ASSERT(object->num_phoneme_multiplier == 1);
    PV_ASSERT(object->word_boundary_phoneme_index > 0);

    int32_t num_text_tokens_with_terminator = num_text_tokens + add_terminator_manually;

    *text_tokens_num_encoded_phonemes = NULL;

    int32_t *num_phonemes = malloc(num_text_tokens_with_terminator * sizeof(int32_t));
    if (!num_phonemes) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("num_phonemes"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t j = 0; j < num_text_tokens_with_terminator; j++) {
        bool last_text_token = (j == (num_text_tokens_with_terminator - 1));

        int32_t num = 0;

        if ((j == 0) && allow_prepend_bos && (object->bos_phoneme_index > 0)) {
            num += 1;
        }

        if (last_text_token) {
            if (add_terminator_manually) {
                num += 1 * object->num_phoneme_multiplier;

                if (object->word_boundary_phoneme_index > 0) {
                    num += 1;
                }
            } else {
                if (num_phonemes_buffer[j] == 0) {
                    num += 0;
                } else if (phonemes_buffer[j][0] == object->word_boundary_phoneme_index) {
                    num += 1;
                } else {
                    num += num_phonemes_buffer[j] * object->num_phoneme_multiplier;
                }
            }

            if (allow_append_eos && (object->eos_phoneme_index > 0)) {
                num += 1;
            }
        } else {
            if (num_phonemes_buffer[j] == 0) {
                num += 0;
            } else if (phonemes_buffer[j][0] == object->word_boundary_phoneme_index) {
                num += 1;
            } else {
                num += num_phonemes_buffer[j] * object->num_phoneme_multiplier;

                if ((object->word_boundary_phoneme_index > 0) &&
                    ((j < (num_text_tokens - 1)) &&
                     (phonemes_buffer[j + 1][0] != object->word_boundary_phoneme_index))) {
                    num += 1;
                }
            }
        }

        num_phonemes[j] = num;
    }

    *text_tokens_num_encoded_phonemes = num_phonemes;

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_orca_phonemizer_encode_phonemes(
        const pv_orca_phonemizer_t *object,
        const int32_t num_text_tokens,
        const int32_t add_terminator_manually,
        bool allow_prepend_bos,
        bool allow_append_eos,
        const int32_t num_encoded_phonemes,
        const int32_t *num_phonemes_buffer,
        const int32_t **phonemes_buffer,
        int32_t **encoded_phonemes) {
    PV_ASSERT(object);
    PV_ASSERT(num_text_tokens >= 0);
    PV_ASSERT(add_terminator_manually >= 0);
    PV_ASSERT(num_encoded_phonemes >= 0);
    PV_ASSERT(num_phonemes_buffer);
    PV_ASSERT(phonemes_buffer);
    PV_ASSERT(encoded_phonemes);

    PV_ASSERT(object->num_phoneme_multiplier == 1);
    PV_ASSERT(object->word_boundary_phoneme_index > 0);

    *encoded_phonemes = NULL;

    int32_t *phonemes = malloc(num_encoded_phonemes * sizeof(int32_t));
    if (!phonemes) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("phonemes"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t l = 0;
    if (allow_prepend_bos && (object->bos_phoneme_index > 0)) {
        phonemes[l++] = object->bos_phoneme_index;
    }

    for (int32_t j = 0; j < num_text_tokens; j++) {
        for (int32_t k = 0; k < num_phonemes_buffer[j]; k++) {
            if (phonemes_buffer[j][k] == object->word_boundary_phoneme_index) {
                phonemes[l++] = object->word_boundary_phoneme_index;
            } else {
                phonemes[l++] = phonemes_buffer[j][k];
            }
        }

        if ((object->word_boundary_phoneme_index > 0) && (j < (num_text_tokens - 1))) {
            bool previous_word_is_word_boundary = (phonemes[l - 1] == object->word_boundary_phoneme_index);
            PV_ASSERT(num_phonemes_buffer[j + 1] >= 1);
            bool next_word_is_word_boundary = (phonemes_buffer[j + 1][0] == object->word_boundary_phoneme_index);
            if (!previous_word_is_word_boundary && !next_word_is_word_boundary) {
                phonemes[l++] = object->word_boundary_phoneme_index;
            }
        }
    }

    PV_ASSERT(l <= num_encoded_phonemes);

    if (add_terminator_manually) {
        if (object->word_boundary_phoneme_index > 0) {
            phonemes[l++] = object->word_boundary_phoneme_index;
            PV_ASSERT(l <= num_encoded_phonemes);
        }

        int32_t offset = (object->eos_phoneme_index > 0) ? 2 : 1;
        phonemes[num_encoded_phonemes - offset] = object->terminator_index;
    }

    if (allow_append_eos && (object->eos_phoneme_index > 0)) {
        phonemes[num_encoded_phonemes - 1] = object->eos_phoneme_index;
    }

    *encoded_phonemes = phonemes;

    return PV_STATUS_SUCCESS;
}

static pv_status_t is_valid_verbalized_tokens(
        int32_t num_text_tokens,
        const pv_normalizer_token_t **text_tokens,
        bool *is_valid) {
    PV_ASSERT(num_text_tokens > 0);
    PV_ASSERT(text_tokens);
    PV_ASSERT(is_valid);

    *is_valid = false;

    for (int32_t i = 0; i < num_text_tokens; i++) {
        const pv_normalizer_token_t *current = text_tokens[i];

        bool is_space = (strlen(current->string) == 1) && (current->string[0] == ' ');
        if (is_space) {
            continue;
        }

        if (!current->verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_null,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("text_tokens[i]->verbalized"));
            return PV_STATUS_INVALID_ARGUMENT;
        }

        if (current->tag_language_agnostic == PV_NORMALIZER_TAG_PUNCTUATION) {
            continue;
        }

        if (current->tag_language_agnostic == PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
            continue;
        }

        size_t length = strlen(current->verbalized);

        size_t char_offset = 0;
        while (char_offset < length) {
            int32_t num_bytes_character = 0;
            pv_status_t status = pv_language_num_bytes_character(
                    (unsigned char) current->verbalized[char_offset],
                    &num_bytes_character);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_language_num_bytes_character,
                        pv_status_to_string(status));
                return status;
            }

            char_offset += num_bytes_character;
        }
    }

    *is_valid = true;
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_phonemize)(
        pv_orca_phonemizer_t *object,
        int32_t num_text_tokens,
        const pv_normalizer_token_t **text_tokens,
        int32_t num_prev_text_tokens,
        const pv_normalizer_token_t **prev_text_tokens,
        bool allow_append_terminator,
        bool allow_prepend_bos,
        bool allow_append_eos,
        int32_t *num_encoded_phonemes,
        int32_t **encoded_phonemes,
        int32_t **text_tokens_num_encoded_phonemes) {
    PV_ASSERT(object);
    PV_ASSERT(num_text_tokens);
    PV_ASSERT(text_tokens);
    PV_ASSERT(num_encoded_phonemes);
    PV_ASSERT(encoded_phonemes);
    PV_ASSERT(text_tokens_num_encoded_phonemes);
    PV_ASSERT(num_prev_text_tokens >= 0);
    if (num_prev_text_tokens != 0) {
        PV_ASSERT(prev_text_tokens);
    }

    *num_encoded_phonemes = 0;
    *encoded_phonemes = NULL;
    *text_tokens_num_encoded_phonemes = NULL;

    bool is_valid = false;
    pv_status_t status = is_valid_verbalized_tokens(num_text_tokens, text_tokens, &is_valid);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                is_valid_verbalized_tokens,
                pv_status_to_string(status));
        return PV_STATUS_INVALID_ARGUMENT;
    } else if (!is_valid) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("text_tokens"));
        return PV_STATUS_INVALID_ARGUMENT;
    }

    int32_t **phonemes_buffer = calloc(num_text_tokens, sizeof(int32_t *));
    if (!phonemes_buffer) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("phoneme_tokens_buffer"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    int32_t *num_phonemes_buffer = calloc(num_text_tokens, sizeof(int32_t));
    if (!num_phonemes_buffer) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("num_phonemes_buffer"));
        free(phonemes_buffer);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    bool last_is_punctuation = false;
    for (int32_t i = 0; i < num_text_tokens; i++) {
        const pv_normalizer_token_t *current = text_tokens[i];
        last_is_punctuation = (current->tag_language_agnostic == PV_NORMALIZER_TAG_PUNCTUATION);

        bool is_space =
                (strlen(current->string) == 1) &&
                ((current->string[0] == ' ') && (current->tag_language_agnostic != PV_NORMALIZER_TAG_PUNCTUATION));
        if (is_space) {
            num_phonemes_buffer[i] = 1;
            phonemes_buffer[i] = calloc(1, sizeof(int32_t));
            if (!phonemes_buffer[i]) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc_array_index,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("phonemes_buffer[i]", i));
                for (int32_t j = 0; j < i; j++) {
                    free(phonemes_buffer[j]);
                }
                free(num_phonemes_buffer);
                free(phonemes_buffer);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            // Because `pv_language_info_phoneme_index_from_string()` plus 1 internally,
            // so we will plus 1 here too to be consistent.
            phonemes_buffer[i][0] = object->word_boundary_phoneme_index + 1;
        } else if (current->tag_language_agnostic == PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
            status = pv_orca_phonemizer_get_phonemes_pronunciation(
                    object,
                    current->pronunciation,
                    num_phonemes_buffer + i,
                    phonemes_buffer + i);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT(
                        &pv_error_msg_orca_invalid_custom_pronunciation,
                        PV_ERROR_ARGS_PUBLIC(current->verbalized),
                        PV_ERROR_ARGS_PRIVATE_EMPTY());
                for (int32_t j = 0; j < i; j++) {
                    free(phonemes_buffer[j]);
                }
                free(num_phonemes_buffer);
                free(phonemes_buffer);
                return status;
            }
        } else if (current->tag_language_agnostic == PV_NORMALIZER_TAG_LETTER_SPELL_OUT) {
            status = pv_orca_phonemizer_get_phonemes_spell_out(
                    object,
                    (pv_normalizer_token_t *) current,
                    num_phonemes_buffer + i,
                    phonemes_buffer + i);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_orca_phonemizer_get_phonemes_spell_out,
                        pv_status_to_string(status));
                for (int32_t j = 0; j < i; j++) {
                    free(phonemes_buffer[j]);
                }
                free(num_phonemes_buffer);
                free(phonemes_buffer);
                return status;
            }
        } else if (current->tag_language_agnostic == PV_NORMALIZER_TAG_PUNCTUATION) {
            num_phonemes_buffer[i] = 1;
            phonemes_buffer[i] = calloc(1, sizeof(int32_t));
            if (!phonemes_buffer[i]) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc_array_index,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("phoneme_tokens_buffer", i));
                for (int32_t j = 0; j < i; j++) {
                    free(phonemes_buffer[j]);
                }
                free(num_phonemes_buffer);
                free(phonemes_buffer);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            status = pv_language_info_phoneme_index_from_string(
                    object->language_info,
                    current->verbalized,
                    phonemes_buffer[i]);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_language_info_phoneme_index_from_string,
                        pv_status_to_string(status));
                for (int32_t j = 0; j <= i; j++) {
                    free(phonemes_buffer[j]);
                }
                free(num_phonemes_buffer);
                free(phonemes_buffer);
                return status;
            }
        } else {
            // currently for JA only - context-based readings are more accurate than what the speaker dictionary gives us.
            if (current->reading != NULL) {
                status = pv_orca_phonemizer_get_phonemes_hippo(
                        object,
                        current->reading,
                        num_phonemes_buffer + i,
                        phonemes_buffer + i);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_orca_phonemizer_get_phonemes_hippo,
                            pv_status_to_string(status));
                    for (int32_t j = 0; j < i; j++) {
                        free(phonemes_buffer[j]);
                    }
                    free(num_phonemes_buffer);
                    free(phonemes_buffer);
                    return status;
                }
            } else if (pv_lexicon_encode(object->lexicon, current->verbalized) == -1) {
                bool spell_out = false;
                bool is_word = false;
                pv_status_t status = pv_normalizer_util_is_word_token(object->language_info, current, &is_word);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_is_word_token,
                            pv_status_to_string(status));
                    for (int32_t j = 0; j < i; j++) {
                        free(phonemes_buffer[j]);
                    }
                    free(num_phonemes_buffer);
                    free(phonemes_buffer);
                    return status;
                }
                bool is_same_length_string_and_original = (strlen(current->string) == strlen(current->verbalized));
                if (is_word && is_same_length_string_and_original) {
                    status = pv_normalizer_util_is_spellout(
                            object->language_info,
                            current->original_string,
                            &spell_out);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_util_is_spellout,
                                pv_status_to_string(status));
                        for (int32_t j = 0; j < i; j++) {
                            free(phonemes_buffer[j]);
                        }
                        free(num_phonemes_buffer);
                        free(phonemes_buffer);
                        return status;
                    }
                }

                if (spell_out) {
                    status = pv_orca_phonemizer_get_phonemes_spell_out(
                            object,
                            (pv_normalizer_token_t *) current,
                            num_phonemes_buffer + i,
                            phonemes_buffer + i);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_orca_phonemizer_get_phonemes_spell_out,
                                pv_status_to_string(status));
                        for (int32_t j = 0; j < i; j++) {
                            free(phonemes_buffer[j]);
                        }
                        free(num_phonemes_buffer);
                        free(phonemes_buffer);
                        return status;
                    }
                } else {
                    char *new_verbalized = NULL;
                    if (strlen(current->verbalized) > 1 && current->verbalized[0] == '\'') {
                        new_verbalized = current->verbalized + 1;
                        if (pv_lexicon_encode(object->lexicon, new_verbalized) != -1) {
                            status = pv_orca_phonemizer_get_phonemes_lexicon(
                                    object,
                                    new_verbalized,
                                    num_phonemes_buffer + i,
                                    phonemes_buffer + i);
                            if (status != PV_STATUS_SUCCESS) {
                                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                        pv_orca_phonemizer_get_phonemes_lexicon,
                                        pv_status_to_string(status));
                                for (int32_t j = 0; j < i; j++) {
                                    free(phonemes_buffer[j]);
                                }
                                free(num_phonemes_buffer);
                                free(phonemes_buffer);
                                return status;
                            }
                            continue;
                        }
                    } else {
                        new_verbalized = current->verbalized;
                    }

                    status = pv_orca_phonemizer_get_phonemes_hippo(
                            object,
                            new_verbalized,
                            num_phonemes_buffer + i,
                            phonemes_buffer + i);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_orca_phonemizer_get_phonemes_hippo,
                                pv_status_to_string(status));
                        for (int32_t j = 0; j < i; j++) {
                            free(phonemes_buffer[j]);
                        }
                        free(num_phonemes_buffer);
                        free(phonemes_buffer);
                        return status;
                    }
                }
            } else if (pv_heteronym_tree_is_heteronym(
                               object->heteronym_tree,
                               pv_lexicon_encode(object->lexicon, current->verbalized))) {
                status = pv_orca_phonemizer_get_phonemes_heteronym(
                        object,
                        i,
                        text_tokens,
                        num_prev_text_tokens,
                        prev_text_tokens,
                        num_phonemes_buffer + i,
                        phonemes_buffer + i);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_orca_phonemizer_get_phonemes_heteronym,
                            pv_status_to_string(status));
                    for (int32_t j = 0; j < i; j++) {
                        free(phonemes_buffer[j]);
                    }
                    free(num_phonemes_buffer);
                    free(phonemes_buffer);
                    return status;
                }
            } else {
                status = pv_orca_phonemizer_get_phonemes_lexicon(
                        object,
                        current->verbalized,
                        num_phonemes_buffer + i,
                        phonemes_buffer + i);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_orca_phonemizer_get_phonemes_lexicon,
                            pv_status_to_string(status));
                    for (int32_t j = 0; j < i; j++) {
                        free(phonemes_buffer[j]);
                    }
                    free(num_phonemes_buffer);
                    free(phonemes_buffer);
                    return status;
                }
            }
        }
    }

    int32_t add_terminator_manually = 0;
    if (allow_append_terminator && !last_is_punctuation) {
        add_terminator_manually = 1;
    }

    int32_t *num_phonemes_buffer_with_pronunciation = calloc(num_text_tokens, sizeof(int32_t *));
    if (!num_phonemes_buffer_with_pronunciation) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("num_phonemes_buffer_with_pronunciation"));
        for (int32_t j = 0; j < num_text_tokens; j++) {
            free(phonemes_buffer[j]);
        }
        free(num_phonemes_buffer);
        free(phonemes_buffer);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    int32_t **phonemes_buffer_with_pronunciation = calloc(num_text_tokens, sizeof(int32_t *));
    if (!phonemes_buffer_with_pronunciation) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("phonemes_buffer_with_pronunciation"));
        for (int32_t j = 0; j < num_text_tokens; j++) {
            free(phonemes_buffer[j]);
        }
        free(num_phonemes_buffer);
        free(phonemes_buffer);
        free(num_phonemes_buffer_with_pronunciation);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    int32_t curr = 0;
    for (int32_t i = 0; i < num_text_tokens; ++i) {
        if (num_phonemes_buffer[i] >= 1) {
            num_phonemes_buffer_with_pronunciation[curr] = num_phonemes_buffer[i];
            for (int32_t j = 0; j < num_phonemes_buffer[i]; ++j) {
                PV_ASSERT(phonemes_buffer[i][j] - 1 >= 0);
                // Because `pv_language_info_phoneme_index_from_string()` plus 1 internally,
                // so need to minus 1 to cancel out.
                --(phonemes_buffer[i][j]);
            }
            phonemes_buffer_with_pronunciation[curr] = phonemes_buffer[i];
            ++curr;
        }
    }
    int32_t num_text_tokens_with_pronunciation = curr;

    int32_t *text_tokens_num_encoded_phonemes_internal = NULL;
    status = pv_orca_phonemizer_get_num_phonemes_per_text_token(
            object,
            num_text_tokens_with_pronunciation,
            add_terminator_manually,
            allow_prepend_bos,
            allow_append_eos,
            num_phonemes_buffer_with_pronunciation,
            (const int32_t **) phonemes_buffer_with_pronunciation,
            &text_tokens_num_encoded_phonemes_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_get_num_phonemes_per_text_token,
                pv_status_to_string(status));
        for (int32_t j = 0; j < num_text_tokens; j++) {
            free(phonemes_buffer[j]);
        }
        free(num_phonemes_buffer);
        free(phonemes_buffer);
        free(num_phonemes_buffer_with_pronunciation);
        free(phonemes_buffer_with_pronunciation);
        return status;
    }

    *num_encoded_phonemes = 0;
    for (int32_t j = 0; j < num_text_tokens_with_pronunciation + add_terminator_manually; j++) {
        *num_encoded_phonemes += text_tokens_num_encoded_phonemes_internal[j];
    }

    int32_t *encoded_phonemes_internal = NULL;
    status = pv_orca_phonemizer_encode_phonemes(
            object,
            num_text_tokens_with_pronunciation,
            add_terminator_manually,
            allow_prepend_bos,
            allow_append_eos,
            *num_encoded_phonemes,
            num_phonemes_buffer_with_pronunciation,
            (const int32_t **) phonemes_buffer_with_pronunciation,
            &encoded_phonemes_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_encode_phonemes,
                pv_status_to_string(status));
        for (int32_t j = 0; j < num_text_tokens; j++) {
            free(phonemes_buffer[j]);
        }
        free(num_phonemes_buffer);
        free(phonemes_buffer);
        free(num_phonemes_buffer_with_pronunciation);
        free(phonemes_buffer_with_pronunciation);
        free(text_tokens_num_encoded_phonemes_internal);
        return status;
    }

    int32_t *text_tokens_num_encoded_phonemes_final = calloc(num_text_tokens, sizeof(int32_t));
    if (!text_tokens_num_encoded_phonemes_final) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("text_tokens_num_encoded_phonemes_final"));
        for (int32_t j = 0; j < num_text_tokens; j++) {
            free(phonemes_buffer[j]);
        }
        free(num_phonemes_buffer);
        free(phonemes_buffer);
        free(num_phonemes_buffer_with_pronunciation);
        free(phonemes_buffer_with_pronunciation);
        free(text_tokens_num_encoded_phonemes_internal);
        free(encoded_phonemes_internal);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    curr = 0;
    for (int32_t i = 0; i < num_text_tokens; ++i) {
        if (num_phonemes_buffer[i] >= 1) {
            text_tokens_num_encoded_phonemes_final[i] = text_tokens_num_encoded_phonemes_internal[curr];
            ++curr;
        } else {
            text_tokens_num_encoded_phonemes_final[i] = 0;
        }
    }

    free(text_tokens_num_encoded_phonemes_internal);
    free(num_phonemes_buffer);
    free(num_phonemes_buffer_with_pronunciation);
    for (int32_t j = 0; j < num_text_tokens; j++) {
        free(phonemes_buffer[j]);
    }
    free(phonemes_buffer);
    free(phonemes_buffer_with_pronunciation);

    *encoded_phonemes = encoded_phonemes_internal;
    *text_tokens_num_encoded_phonemes = text_tokens_num_encoded_phonemes_final;

#ifdef __ORCA_PHONEMIZER_LOG_DEBUG__

    LOG_DEBUG_INLINE("[Phonemizer] Passing `%d` phonemes to model\n", *num_encoded_phonemes);
    LOG_DEBUG_INLINE_SIMPLE("[Phonemizer] PHONEME SEQUENCE\n[Phonemizer] ");
    for (int32_t i = 0; i < *num_encoded_phonemes; i++) {
        LOG_DEBUG_INLINE("%d ", (*encoded_phonemes)[i]);
    }
    LOG_DEBUG_INLINE_SIMPLE("\n");
    LOG_DEBUG_INLINE_SIMPLE("[Phonemizer] TOKEN LENGTHS\n[Phonemizer] ");
    for (int32_t i = 0; i < num_text_tokens + add_terminator_manually; i++) {
        LOG_DEBUG_INLINE("%d ", (*text_tokens_num_encoded_phonemes)[i]);
    }
    LOG_DEBUG_INLINE_SIMPLE("\n");

#endif

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_remove_invalid_custom_pronunciation_tokens)(
        const pv_orca_phonemizer_t *object,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);

    pv_normalizer_token_t *current = token_list->head;
    while (current) {
        pv_normalizer_token_t *next = current->next;
        if (current->tag_language_agnostic == PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
            int32_t num_phonemes_buffer = 0;
            int32_t *phonemes_buffer = NULL;
            pv_status_t status = pv_orca_phonemizer_get_phonemes_pronunciation(
                    object,
                    current->pronunciation,
                    &num_phonemes_buffer,
                    &phonemes_buffer);
            free(phonemes_buffer);
            if (status == PV_STATUS_OUT_OF_MEMORY) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_orca_phonemizer_get_phonemes_pronunciation,
                        pv_status_to_string(status));
                return status;
            } else if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_orca_phonemizer_get_phonemes_pronunciation,
                        pv_status_to_string(status));
                pv_normalizer_token_list_remove_token(token_list, current);
            }
        }
        current = next;
    }

    return PV_STATUS_SUCCESS;
}
