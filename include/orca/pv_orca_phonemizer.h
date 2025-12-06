#ifndef PV_ORCA_PHONEMIZER_H
#define PV_ORCA_PHONEMIZER_H

#include "core/pv_language.h"
#include "hippo/pv_hippo_internal.h"
#include "lm/pv_dict.h"
#include "lm/pv_heteronym_tree.h"
#include "model/pv_offline_token_classifier.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_util.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    bool add_eos_punctuation;
    bool add_bos_phoneme;
    bool add_eos_phoneme;
    bool add_word_boundary_phoneme;
    int32_t num_phoneme_multiplier;
} pv_orca_phonemizer_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_param_serialize)(const pv_orca_phonemizer_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_orca_phonemizer_param_delete)(pv_orca_phonemizer_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_param_load)(FILE *f, pv_orca_phonemizer_param_t **param);

typedef struct pv_orca_phonemizer pv_orca_phonemizer_t;

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_init)(
        const pv_orca_phonemizer_param_t *param,
        pv_hippo_t *hippo,
        pv_lexicon_t *lexicon,
        pv_dict_t *dict,
        pv_heteronym_tree_t *heteronym_tree,
        pv_language_info_t *language_info,
        pv_orca_phonemizer_t **object);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_initialize_alphabet)(
        pv_normalizer_language_t language,
        int32_t *num_letters_alphabet,
        const char *const **alphabet_letters,
        const char *const **alphabet_pronunciations);

void PV_MOCKABLE(pv_orca_phonemizer_delete)(pv_orca_phonemizer_t *object);

int32_t PV_MOCKABLE(pv_orca_phonemizer_get_word_boundary_index)(const pv_orca_phonemizer_t *object);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_punctuation_end_indices)(
        const pv_orca_phonemizer_t *object,
        int32_t num_punctuation_chars,
        const char **punctuation_chars,
        int32_t **punctuation_indices);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_terminator_index)(
        const pv_language_info_t *language_info,
        int32_t *terminator_index);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_spell_out)(
        const pv_orca_phonemizer_t *object,
        const char *string,
        char **pronunciation);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_phonemes_hippo)(
        const pv_orca_phonemizer_t *object,
        const char *word,
        int32_t *num_phonemes,
        int32_t **phonemes);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_phonemes_lexicon)(
        const pv_orca_phonemizer_t *object,
        const char *token,
        int32_t *num_phonemes,
        int32_t **phonemes);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_phonemes_heteronym)(
        const pv_orca_phonemizer_t *object,
        int32_t heteronym_token_index,
        const pv_normalizer_token_t **start_text_token,
        int32_t num_prev_text_tokens,
        const pv_normalizer_token_t **prev_text_tokens,
        int32_t *num_phonemes,
        int32_t **phonemes);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_phonemes_pronunciation)(
        const pv_orca_phonemizer_t *object,
        const char *token,
        int32_t *num_phonemes,
        int32_t **phonemes);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_get_phonemes_spell_out)(
        const pv_orca_phonemizer_t *object,
        pv_normalizer_token_t *token,
        int32_t *num_phonemes,
        int32_t **phonemes);

/* Takes text tokens from tokenizer and looks up the phonemes with three methods:
 * 1. Lexicon
 * 2. Custom pronunciation (already attached to the text_tokens)
 * 3. Hippo
 *
 * The phonemes are then encoded.
 *
 * If the phoneme_multiplier is larger than 1, the phoneme indices are multiplied by the phoneme_multiplier.
 * For example: AA becomes _AA and AA_.
 *
 * The following special phonemes are also added to the encoded_phonemes, if enabled:
 * 1. Word boundary phoneme
 * 2. beginning-of-sequence (BOS) phoneme
 * 3. end-of-sequence (EOS) phoneme
 * 4. Terminator phoneme if the sequence does not end with punctuation
 */
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
        int32_t **text_tokens_num_encoded_phonemes);

pv_status_t PV_MOCKABLE(pv_orca_phonemizer_remove_invalid_custom_pronunciation_tokens)(
        const pv_orca_phonemizer_t *object,
        pv_normalizer_token_list_t *token_list);

#endif // PV_ORCA_PHONEMIZER_H