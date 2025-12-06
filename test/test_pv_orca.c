#include <errno.h>
#include <math.h>
#include <string.h>

#include "io/pv_log.h"
#include "test/pv_test.h"

#include "../../gatekeeper/test/test_pv_gatekeeper_usage_helper.h"
#include "gatekeeper/pv_gatekeeper_usage_animal.h"
#include "hippo/pv_hippo_internal.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_util.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_stream_state.h"
#include "orca/pv_orca_synthesizer.h"
#include "orca/pv_orca_util.h"
#include "orca/normalizer/pv_normalizer_data/pv_normalizer_shared_data.h"
#include "util/pv_string.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif


static const char *MODEL_PATH = "param/orca_params_en_female.pv";
static const int32_t TEST_SAMPLING_RATE = 22050;
static const int32_t MAX_CHARACTER_LIMIT = 2000;

extern const pv_orca_phonemizer_param_t PV_ORCA_PHONEMIZER_PARAM;
extern const pv_orca_synthesizer_param_t PV_ORCA_SYNTHESIZER_PARAM;

static pv_ypu_t *ypu = NULL;
static pv_orca_t *orca_object = NULL;
static pv_orca_synthesize_params_t *synthesize_params_object = NULL;
static pv_orca_stream_state_t *stream_state_object = NULL;

static const char *DEFAULT_SENTENCE = "Welcome, to our demonstration of streaming text-to-speech technology!";
static const char *DEFAULT_SENTENCE_TOKENS[] = {
        "Welcome", ",", " to", " our", " demonstration", " of", " streaming", " text", "-", "to", "-", "speech",
        " technology", "!"};

static const char *TEST_ROBUSTNESS_SENTENCES[27][2] = {
        {"hello",                      " a8"},
        {"yo",                         " }"},
        {"}}}}",                       "|{|WHAT}"},
        {"{}",                         " hi {hi|AY}"},
        {"55",                         " 55"},
        {"%",                          "@"},
        {"\n",                         "\n"},
        {" \n-",                       "{\n|~."},
        {" --- ",                      "you -- are---"},
        {"hi{you_r|Y_AO_R}",           "{**your|}"},
        {"hi{your|Y_AO_R}life",        "hi{your|Y UH R}"},
        {"*^",                         "^"},
        {"🌟🦄",                         "🚀"},
        {"###@$%sde87-#$%uwe(&jfkj.#", " 2%"},
        {"      ",                     "   )  "},
        {" ",                          " 1"},
        {"\t",                         " - "},
        {"\"hi\n<*|end/>\"",           "\"h%n<*|end/>\""},
        {"hi -",                       "-"},
        {"###@$%s7813-a3/sefi.#",      "!"},
        {"1",                          ".5%"},
        {"5th",                        " place"},
        {"Life's soundtrack is a symphony of laughter, tears, and the occasional scream. Life's soundtrack is a "
         "symphony of laughter, tears, and the occasional scream. Life's soundtrack is a symphony of laughter, tears, "
         "and the occasional scream. Life's soundtrack is a symphony of laughter, tears, and the occasional "
         "scream.",                    " "},
        {
         "1 2 3 4 5 {|WY} {6 7 8 9 10 11 12 13 14\n-$15 16 17 18 19 20 21 22 23 24 25 26 27 28 29"
         "30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55",
                                       "--56 57 58 59 60 61 62 63 64 }65 66}}\n 67 68 69 70 71 72 73 74 "},
        {
         "1 2 3 4 5 6 7 8 9 10 11 12 13, 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29"
         "30 31 32 33 34 35 36 37, 38 39 40 41 42 43 44 45 46 47, 48 49 50 51 52 53 54 55",
                                       "56 57 58, 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 "},
        {"",                           " "},
        {"_______\n|       |",         " "},
};
static const pv_status_t TEST_ROBUSTNESS_STATUS[27][2] = {
        {PV_STATUS_SUCCESS,             PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,             PV_STATUS_INVALID_ARGUMENT},
        {PV_STATUS_INVALID_ARGUMENT,    PV_STATUS_INVALID_ARGUMENT},
        {PV_STATUS_INVALID_ARGUMENT,    PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,             PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,             PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,             PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,             PV_STATUS_INVALID_ARGUMENT},
        {PV_STATUS_SUCCESS,             PV_STATUS_SUCCESS},
        {PV_STATUS_INVALID_ARGUMENT,    PV_STATUS_INVALID_ARGUMENT},
        {PV_STATUS_INVALID_ARGUMENT,   PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_INVALID_ARGUMENT,   PV_STATUS_INVALID_ARGUMENT},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
        {PV_STATUS_SUCCESS,            PV_STATUS_SUCCESS},
};

static const char *TEST_ROBUSTNESS_NORMALIZER_CASES[13][8] = {
        {"2",     "-",      "10",      "apples",  "7182",  "st",            "",             ""},
        {"-",     "22",     ".5",      "m",       "/",     "s",             "CS3",          "1"},
        {"400",   ",",      "000",     "th",      "1",     "/",             "3",            "08"},
        {"400,0", "000",    "th",      "1/",      "3",     "08",            ",",            "1"},
        {"14",    "a",      ".",       "m",       " .",    "e",             ".",            "g"},
        {"+",     "1",      "(",       "778",     ")",     "482",           "-",            "3829"},
        {"(",     "7",      "7",       "9",       ")",     " ",             "182",          "-1020"},

        {"1999",  "-",      "02",      "-",       "19",    " 28-Apr-1987 ", "28-Apr-1987 ", "1999"},
        {"28",    "-",      "Apr-198", "7",       " ",     "2",             "-",            "Apr-1987"},
        {"$",     "6",      " ",       "8.12",    " ",     "EUR",           " ",            "USD"},
        {"1/2",   "1.019 ", "https:",  "/",       "/",     ".com",          ".us",          "www. "},
        {"I'm",   " J. ",   "K.",      "Rowling", " and",  " John F.",      "Kennedy",      "J. R."},
        {"xav",   " XAV ",  " BPAM",   " TELL",   " pbam", " YOU",          " ARE",         " "},
};

static const char *SHORT_SENTENCE = "Pico";
static const char *VALID_TEST_SENTENCES[] = {
        "55, 2%, & 7th place",
        "Six, seven, eight.",
        "Five hundred",
        "This! Is \"Unbelievable\"?",
        "A {custom|K AH S T AH M} pronunciation",
        "The third-best game",
};

static const char *ALIGNMENT_TEST_SENTENCE = "One! 2%";
static const pv_orca_phoneme_alignment_t phonemes1 = {.phoneme="W", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t phonemes2 = {.phoneme="AH", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t phonemes3 = {.phoneme="N", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t *phoneme_first_word[] = {&phonemes1, &phonemes2, &phonemes3};
static const pv_orca_word_alignment_t alignment1 = {
        .word = "One",
        .start_sec = 0.0f,
        .end_sec = 0.1f,
        .num_phonemes = 3,
        .phonemes = (pv_orca_phoneme_alignment_t **) phoneme_first_word};
static const pv_orca_phoneme_alignment_t phonemes4 = {.phoneme="!", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t *phoneme_second_word[] = {&phonemes4};
static const pv_orca_word_alignment_t alignment2 = {
        .word = "!",
        .start_sec = 0.0f,
        .end_sec = 0.1f,
        .num_phonemes = 1,
        .phonemes = (pv_orca_phoneme_alignment_t **) phoneme_second_word};
static const pv_orca_phoneme_alignment_t phonemes5 = {.phoneme="T", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t phonemes6 = {.phoneme="UW", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t phonemes7 = {.phoneme="P", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t phonemes8 = {.phoneme="ER", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t phonemes9 = {.phoneme="S", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t phonemes10 = {.phoneme="EH", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t phonemes11 = {.phoneme="N", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t phonemes12 = {.phoneme="T", .start_sec = 0.0f, .end_sec = 0.1f};
static const pv_orca_phoneme_alignment_t *phoneme_third_word[] = {
        &phonemes5, &phonemes6, &phonemes7, &phonemes8, &phonemes9, &phonemes10, &phonemes11, &phonemes12};
static const pv_orca_word_alignment_t alignment3 = {
        .word = "2%",
        .start_sec = 0.0f,
        .end_sec = 0.1f,
        .num_phonemes = 8,
        .phonemes = (pv_orca_phoneme_alignment_t **) phoneme_third_word};

static const pv_orca_word_alignment_t *SENTENCE_ALIGNMENTS[] = {&alignment1, &alignment2, &alignment3};
static const int32_t ALIGNMENTS_NUM_PHONEMES[] = {3, 1, 8};

static const float DEFAULT_SPEECH_RATE = 1.0f;

static struct init_args {
    char *access_key;
    pv_https_client_factory_t *https_client_factory;
    char *model_path;
    const char *device;
    pv_orca_t **object;
} default_init_args = {
        .access_key = FAKE_ACCESS_KEY,
        .device = "cpu:1"
};

static struct synthesize_args {
    pv_orca_t *object;
    char *text;
    pv_orca_synthesize_params_t *params;
    int32_t *num_samples;
    int16_t **pcm;
    int32_t *num_alignments;
    pv_orca_word_alignment_t ***alignments;
} default_synthesize_args = {};

static int32_t curr_fread = 0;
static int32_t fread_count = 0;

#if !defined(__PV_TARGET_NO_FILE_INTERFACE__) && !defined(__PV_TARGET_NO_FILE_SYSTEM__)

static void get_tmp_wav_path(char **path) {
    *path = NULL;

    const char *output_file_ending = ".wav";
    char tmp_path[L_tmpnam];
    char *output_path = tmpnam(tmp_path);

#ifndef __PV_TARGET_PLATFORM_ANDROID__

    *path = pv_string_format("%s%s", output_path, output_file_ending);

#else

    char *file_name = strrchr(output_path, '/');
    *path = pv_string_format("%s%s%s", "/data/data/ai.picovoice.zoo", file_name, output_file_ending);

#endif
}

#endif

static pv_status_t test_pv_orca_setup_helper(
        const char *param_name,
        pv_orca_t **object) {
    char *model_path = pv_test_module_res_path(param_name);
    if (!model_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    char *access_key = NULL;
    pv_status_t status = pv_access_serialize(&BYPASS_ACCESS, &access_key);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_https_client_factory_t *factory = NULL;
    status = get_https_client_factory_usage_success(&factory);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_internal_init(
            access_key,
            factory,
            model_path,
            pv_ypu_clone(ypu),
            object);
    free(access_key);
    free(model_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_synthesize_params_init(&synthesize_params_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    stream_state_object = pv_orca_stream_state_get(orca_object);

    return status;
}

static pv_status_t test_pv_orca_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = test_pv_orca_setup_helper(
            MODEL_PATH,
            &orca_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_https_client_factory_t *factory = NULL;
    status = get_https_client_factory_usage_success(&factory);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    default_init_args.https_client_factory = factory;

    default_init_args.model_path = pv_test_module_res_path(MODEL_PATH);
    if (!default_init_args.model_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    default_init_args.object = calloc(1, sizeof(pv_orca_t *));
    if (!default_init_args.object) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    default_synthesize_args.params = calloc(1, sizeof(pv_orca_synthesize_params_t *));
    if (!default_synthesize_args.params) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    default_synthesize_args.num_samples = calloc(1, sizeof(int32_t));
    if (!default_synthesize_args.num_samples) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    default_synthesize_args.pcm = calloc(1, sizeof(int16_t *));
    if (!default_synthesize_args.pcm) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    default_synthesize_args.num_alignments = calloc(1, sizeof(int32_t));
    if (!default_synthesize_args.num_alignments) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    default_synthesize_args.alignments = calloc(1, sizeof(pv_orca_word_alignment_t **));
    if (!default_synthesize_args.alignments) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    default_synthesize_args.text = (char *) DEFAULT_SENTENCE;

    default_synthesize_args.object = orca_object;

    return status;
}

static void test_pv_orca_teardown(void) {
    pv_orca_synthesize_params_delete(synthesize_params_object);
    pv_orca_delete(orca_object);
    free(default_init_args.model_path);
    free(default_init_args.object);
    free(default_synthesize_args.num_samples);
    free(default_synthesize_args.pcm);
    free(default_synthesize_args.num_alignments);
    free(default_synthesize_args.alignments);
    pv_ypu_delete(ypu);
}

// TODO(Jaeger): This test needs to become multi-lingual
static void test_pv_orca_valid_characters(void) {
    int32_t num_characters = 0;
    const char *const *characters = NULL;

    pv_status_t status = pv_orca_valid_characters(orca_object, &num_characters, &characters);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to get valid characters");

    const int32_t expected_num_characters = 113;
    pv_test_true(
            num_characters == expected_num_characters,
            "number of characters incorrect, got `%d` expected `%d`",
            num_characters,
            expected_num_characters);

    const char *valid_characters[] = {
            ".", ":", ",", "\"", "?", "!",
            "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n",
            "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
            "O", "P", "Q", "R", "S", "T", "U", "V", "W",
            "X", "Y", "Z",
            APOSTROPHE, "{", "}", "|", " ", HYPHEN, "/",
            "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
            "@", "%", "&", "\n", "_", "(", ")",
            "°", "º", "²", "³",
            "$", "€", "¥", "₪", "£", "₩", "₺", "₱", "₽", "฿", "₴", "₹", "¢", "+", "=",
            "#", MINUS_SIGN, EN_DASH, FIGURE_DASH, EM_DASH, HORIZONTAL_BAR, RIGHT_SINGLE_QUOTATION_MARK,
            LEFT_SINGLE_QUOTATION_MARK, RIGHT_DOUBLE_QUOTATION_MARK, LEFT_DOUBLE_QUOTATION_MARK, FRACTION_SLASH,
            HORIZONTAL_ELLIPSIS,
    };
    for (int32_t i = 0; i < num_characters; i++) {
        pv_test_true(
                strcmp(characters[i], valid_characters[i]) == 0,
                "incorrect characters, got `%s` expected `%s`",
                characters[i],
                valid_characters[i]);
    }
    pv_orca_valid_characters_delete(characters);
}

static void test_pv_orca_valid_characters_sentences(void) {
    int32_t num_characters = 0;
    const char *const *characters = NULL;
    pv_status_t status = pv_orca_valid_characters(orca_object, &num_characters, &characters);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to get valid characters");

    for (int32_t i = 0; i < PV_ARRAY_LEN(VALID_TEST_SENTENCES); i++) {

        const char *sentence = VALID_TEST_SENTENCES[i];
        for (size_t j = 0; j < strlen(sentence); j++) {

            int32_t num_bytes_character = 0;
            status = pv_language_num_bytes_character((unsigned char) sentence[i], &num_bytes_character);
            pv_test_true(status == PV_STATUS_SUCCESS, "failed to get number of bytes for character `%c`", sentence[j]);

            char character[5] = {0};
            for (int32_t k = 0; k < num_bytes_character; k++) {
                character[k] = sentence[j + k];
            }
            character[num_bytes_character] = '\0';

            for (int32_t l = 0; l < num_characters; l++) {
                if (strcmp(character, characters[l]) == 0) {
                    break;
                }
                if (l == num_characters - 1) {
                    pv_test_true(false, "invalid character `%s` in sentence `%s`", character, sentence);
                }
            }
        }
    }
    pv_orca_valid_characters_delete(characters);
}

static void test_pv_orca_synthesize_params_set_and_get_speech_rate(void) {
    pv_status_t status = pv_orca_synthesize_params_set_speech_rate(synthesize_params_object, 0.5f);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to set speech rate");

    status = pv_orca_synthesize_params_set_speech_rate(synthesize_params_object, 1.5f);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to set speech rate");

    status = pv_orca_synthesize_params_set_speech_rate(synthesize_params_object, DEFAULT_SPEECH_RATE);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to set speech rate");

    float speech_rate;
    status = pv_orca_synthesize_params_get_speech_rate(synthesize_params_object, &speech_rate);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to get speech rate");
    pv_test_true(speech_rate == DEFAULT_SPEECH_RATE, "failed to get speech rate");
}

static void test_pv_orca_get_sample_rate(void) {
    int32_t sample_rate = 0;
    pv_status_t status = pv_orca_sample_rate(orca_object, &sample_rate);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to get sampling rate");
    pv_test_true(sample_rate == TEST_SAMPLING_RATE, "incorrect sampling rate");

    status = pv_orca_sample_rate(NULL, &sample_rate);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail when getting sampling rate");
}

static void test_pv_orca_stream_invalid_stream_state(void) {
    int32_t num_samples = 0;
    int16_t *pcm = NULL;
    pv_orca_stream_t *orca_stream = NULL;
    pv_status_t status = pv_orca_stream_open(orca_object, synthesize_params_object, &orca_stream);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to open stream, got status `%s`",
            pv_status_to_string(status));

    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;
    status = pv_orca_synthesize(
            orca_object,
            "Hi",
            synthesize_params_object,
            &num_samples,
            &pcm,
            &num_alignments,
            &alignments);
    pv_test_true(
            status == PV_STATUS_INVALID_STATE,
            "failed to fail with invalid state in *synthesize(), got status `%s`",
            pv_status_to_string(status));
    if (pcm != NULL) {
        free(pcm);
        pcm = NULL;
    }
#if !defined(__PV_TARGET_NO_FILE_INTERFACE__) && !defined(__PV_TARGET_NO_FILE_SYSTEM__)

    char *output_path = NULL;
    get_tmp_wav_path(&output_path);
    pv_test_true(output_path != NULL, "failed to create tmp file");
    if (!output_path) {
        return;
    }

    status = pv_orca_synthesize_to_file(
            orca_object,
            "Hi",
            synthesize_params_object,
            output_path,
            &num_alignments,
            &alignments);

    pv_test_true(
            status == PV_STATUS_INVALID_STATE,
            "failed to fail with invalid state in *synthesize_to_file(), got status `%s`",
            pv_status_to_string(status));

    free(output_path);
#endif

    status = pv_orca_stream_open(orca_object, synthesize_params_object, &orca_stream);
    pv_test_true(
            status == PV_STATUS_INVALID_STATE,
            "failed to fail opening stream with invalid state, got status `%s`",
            pv_status_to_string(status));
    status = pv_orca_stream_synthesize(
            orca_stream,
            "Hi, long sentence so that first audio chunk should be produced for sure!",
            &num_samples,
            &pcm);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to synthesize, got status `%s`",
            pv_status_to_string(status));
    pv_test_true((num_samples > 0) && (pcm != NULL), "failed to produce audio chunk");
    if (pcm != NULL) {
        free(pcm);
        pcm = NULL;
    }

    status = pv_orca_stream_flush(orca_stream, &num_samples, &pcm);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to flush, got status `%s`", pv_status_to_string(status));
    pv_test_true((num_samples > 0) && (pcm != NULL), "failed to return audio");
    if (pcm != NULL) {
        free(pcm);
        pcm = NULL;
    }

    pv_orca_stream_close(orca_stream);
}

static void test_pv_orca_stream_invalid_args(void) {
    pv_orca_stream_t *orca_stream = NULL;
    pv_status_t status = pv_orca_stream_open(NULL, synthesize_params_object, &orca_stream);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    status = pv_orca_stream_open(orca_object, NULL, &orca_stream);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    status = pv_orca_stream_open(orca_object, synthesize_params_object, &orca_stream);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed open stream");

    int32_t num_samples = 0;
    int16_t *pcm = NULL;
    status = pv_orca_stream_synthesize(NULL, "hi", &num_samples, &pcm);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    status = pv_orca_stream_synthesize(orca_stream, NULL, &num_samples, &pcm);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    status = pv_orca_stream_synthesize(orca_stream, "hi", NULL, &pcm);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    status = pv_orca_stream_synthesize(orca_stream, "hi", &num_samples, NULL);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    status = pv_orca_stream_flush(NULL, &num_samples, &pcm);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    status = pv_orca_stream_flush(orca_stream, NULL, &pcm);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    status = pv_orca_stream_flush(orca_stream, &num_samples, NULL);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    pv_orca_stream_close(orca_stream);
}

static void test_pv_orca_streaming_synthesize_robustness(void) {
    pv_orca_stream_t *orca_stream = NULL;

    pv_status_t status = pv_orca_stream_open(orca_object, synthesize_params_object, &orca_stream);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to open stream");
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_close(orca_stream);
        return;
    }

    for (int32_t i = 0; i < PV_ARRAY_LEN(TEST_ROBUSTNESS_SENTENCES); i++) {
        for (int32_t j = 0; j < PV_ARRAY_LEN(TEST_ROBUSTNESS_SENTENCES[i]); j++) {
            const char *token = TEST_ROBUSTNESS_SENTENCES[i][j];

            int32_t num_samples_chunk = 0;
            int16_t *pcm_chunk = NULL;
            status = pv_orca_stream_synthesize_internal(
                    orca_stream,
                    false,
                    true,
                    token,
                    false,
                    &num_samples_chunk,
                    &pcm_chunk);
            pv_test_true(
                    status == TEST_ROBUSTNESS_STATUS[i][j],
                    "failed to add token `%s`, expected status `%s` got `%s`",
                    token,
                    pv_status_to_string(TEST_ROBUSTNESS_STATUS[i][j]),
                    pv_status_to_string(status));
        }

        int32_t num_samples_chunk = 0;
        int16_t *pcm_chunk = NULL;

        status = pv_orca_stream_flush_internal(
                orca_stream,
                true,
                true,
                false,
                &num_samples_chunk,
                &pcm_chunk);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to flush, got status `%s`", pv_status_to_string(status));
    }

    for (int32_t i = 0; i < PV_ARRAY_LEN(TEST_ROBUSTNESS_NORMALIZER_CASES); i++) {
        for (int32_t j = 0; j < PV_ARRAY_LEN(TEST_ROBUSTNESS_NORMALIZER_CASES[i]); j++) {
            const char *token = TEST_ROBUSTNESS_NORMALIZER_CASES[i][j];

            int32_t num_samples_chunk = 0;
            int16_t *pcm_chunk = NULL;
            status = pv_orca_stream_synthesize_internal(
                    orca_stream,
                    false,
                    true,
                    token,
                    false,
                    &num_samples_chunk,
                    &pcm_chunk);
            pv_test_true(
                    status == PV_STATUS_SUCCESS,
                    "failed to add token `%s`, status `%s`",
                    token,
                    pv_status_to_string(status));
            if (status != PV_STATUS_SUCCESS) {
                pv_orca_stream_close(orca_stream);
                return;
            }
        }

        int32_t num_samples_chunk = 0;
        int16_t *pcm_chunk = NULL;

        status = pv_orca_stream_flush_internal(
                orca_stream,
                true,
                true,
                false,
                &num_samples_chunk,
                &pcm_chunk);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to flush");
    }

    pv_orca_stream_close(orca_stream);
}

typedef struct pcm_chunk_node pcm_chunk_node_t;

struct pcm_chunk_node {
    int32_t num_samples;
    int16_t *pcm;
    pcm_chunk_node_t *next;
};

static pv_status_t pcm_chunk_init(
        int32_t num_samples,
        int16_t *pcm,
        pcm_chunk_node_t **chunk) {
    PV_ASSERT(pcm);
    PV_ASSERT(chunk);

    *chunk = NULL;

    pcm_chunk_node_t *c = calloc(1, sizeof(pcm_chunk_node_t));
    if (!c) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    c->pcm = pcm;
    c->num_samples = num_samples;
    c->next = NULL;

    *chunk = c;

    return PV_STATUS_SUCCESS;
}

static pv_status_t pcm_chunk_delete(pcm_chunk_node_t *chunk) {
    if (chunk) {
        free(chunk->pcm);
        free(chunk);
    }
    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_synthesize(void) {
    int32_t num_samples = 0;
    int16_t *pcm = NULL;
    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;

    pv_status_t status = pv_orca_synthesize_internal(
            orca_object,
            DEFAULT_SENTENCE,
            synthesize_params_object,
            true,
            true,
            &num_samples,
            &pcm,
            &num_alignments,
            &alignments);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to synthesize; expected `%s` got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));

    for (int32_t i = 0; i < num_alignments; i++) {
        pv_orca_word_alignment_t *word = alignments[i];
        pv_test_true(word->start_sec >= 0, "invalid alignment start");
        pv_test_true(word->end_sec >= 0, "invalid alignment end");
        pv_test_true(word->num_phonemes > 0, "invalid number of phonemes");
        pv_test_true(word->start_sec <= word->end_sec, "invalid alignment start and end");

        for (int32_t j = 0; j < word->num_phonemes; j++) {
            pv_orca_phoneme_alignment_t *phoneme = word->phonemes[j];

            pv_test_true(phoneme->start_sec >= 0, "invalid phoneme start");
            pv_test_true(phoneme->end_sec >= 0, "invalid phoneme end");
            pv_test_true(phoneme->start_sec <= phoneme->end_sec, "invalid phoneme start and end");
        }
    }
    pv_orca_word_alignments_delete(num_alignments, alignments);

    pcm_chunk_node_t *pcm_chunk_prev = NULL;
    pcm_chunk_node_t *pcm_chunk_head = NULL;

    pv_orca_stream_t *orca_stream = NULL;
    status = pv_orca_stream_open(orca_object, synthesize_params_object, &orca_stream);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to open stream");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    for (int32_t i = 0; i < PV_ARRAY_LEN(DEFAULT_SENTENCE_TOKENS); i++) {
        const char *token = DEFAULT_SENTENCE_TOKENS[i];

        int32_t num_samples_chunk = 0;
        int16_t *pcm_chunk = NULL;
        status = pv_orca_stream_synthesize_internal(
                orca_stream,
                true,
                false,
                token,
                true,
                &num_samples_chunk,
                &pcm_chunk);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to add token");
        if (status != PV_STATUS_SUCCESS) {
            return;
        }

        if (num_samples_chunk > 0) {
            if (pcm_chunk_prev == NULL) {
                pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev);
                pcm_chunk_head = pcm_chunk_prev;
            } else {
                pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev->next);
                pcm_chunk_prev = pcm_chunk_prev->next;
            }
        }
    }

    int32_t num_samples_chunk = 0;
    int16_t *pcm_chunk = NULL;
    status = pv_orca_stream_flush_internal(
            orca_stream,
            true,
            false,
            false,
            &num_samples_chunk,
            &pcm_chunk);
    pv_orca_stream_close(orca_stream);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to flush");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    if (num_samples_chunk > 0) {
        if (pcm_chunk_prev == NULL) {
            pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev);
            pcm_chunk_head = pcm_chunk_prev;
        } else {
            pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev->next);
            pcm_chunk_prev = pcm_chunk_prev->next;
        }
    }

    int32_t num_samples_streaming = 0;
    pcm_chunk_node_t *pcm_chunk_iter = pcm_chunk_head;
    while (pcm_chunk_iter != NULL) {
        num_samples_streaming += pcm_chunk_iter->num_samples;
        pcm_chunk_iter = pcm_chunk_iter->next;
    }

    int16_t *pcm_streaming = malloc(num_samples_streaming * sizeof(int16_t));
    int32_t offset = 0;
    pcm_chunk_iter = pcm_chunk_head;
    while (pcm_chunk_iter != NULL) {
        memcpy(&pcm_streaming[offset], pcm_chunk_iter->pcm, pcm_chunk_iter->num_samples * sizeof(int16_t));
        offset += pcm_chunk_iter->num_samples;
        pcm_chunk_prev = pcm_chunk_iter;
        pcm_chunk_iter = pcm_chunk_iter->next;
        pcm_chunk_delete(pcm_chunk_prev);
    }

    // The length of the audio from single synthesis and streaming synthesis is exactly the same because
    // everything up to the duration prediction is deterministic. However, we don't have infinite numerical precision,
    // so that a duration predicted to be 2.5 (before rounding), may be resolved to 3 or to 2 frames.
    // This was seen for Orca v1.0 models.
    // We can't add a tolerance here because below we compare the pcm sample by sample. Our remedy is to
    // change the test sentence when these numerical failures occur!
    pv_test_true(
            num_samples == num_samples_streaming,
            "different number of samples with streaming synthesis, got `%d` expected `%d`",
            num_samples_streaming,
            num_samples);
    if (num_samples != num_samples_streaming) {
        return;
    }

    float threshold = 5000;
    int32_t num_differences_above_threshold = 0;
    for (int32_t i = 0; i < num_samples; i++) {
        float sample_difference = fabsf((float) pcm_streaming[i] - (float) pcm[i]);
        if (sample_difference > threshold) {
            num_differences_above_threshold++;
        }
    }

    float percentage_outliers = (float) num_differences_above_threshold / (float) num_samples;
    pv_test_true(
            percentage_outliers < 0.05,
            "too many outliers, got `%d`, maximum allowed: `%d`",
            num_differences_above_threshold,
            (int32_t) (num_samples * 0.1));

    pv_orca_pcm_delete(pcm_streaming);
    pv_orca_pcm_delete(pcm);
}

static void test_pv_orca_synthesize_custom_pron(void) {
    int32_t num_samples = 0;
    int16_t *pcm = NULL;
    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;

    char *text = "I {custom|K AH S T AH M} 55";
    pv_status_t status = pv_orca_synthesize_internal(
            orca_object,
            text,
            synthesize_params_object,
            true,
            false,
            &num_samples,
            &pcm,
            &num_alignments,
            &alignments);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to synthesize text `%s`; expected status `%s` got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));

    pv_orca_pcm_delete(pcm);
    pv_orca_word_alignments_delete(num_alignments, alignments);
}

#ifdef __PV_ENABLE_RELEASE_TESTS__

static void test_pv_orca_stream_synthesize_long(void) {
    pv_orca_stream_t *orca_stream = NULL;
    pv_status_t status = pv_orca_stream_open(orca_object, synthesize_params_object, &orca_stream);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to open stream");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    for (int32_t k = 0; k < 2; k++) {
        for (int32_t i = 0; i < 3; i++) {
            for (int32_t j = 0; j < PV_ARRAY_LEN(DEFAULT_SENTENCE_TOKENS); j++) {
                const char *token = DEFAULT_SENTENCE_TOKENS[j];

                int32_t num_samples_chunk = 0;
                int16_t *pcm_chunk = NULL;
                status = pv_orca_stream_synthesize(
                        orca_stream,
                        token,
                        &num_samples_chunk,
                        &pcm_chunk);
                pv_test_true(
                        status == PV_STATUS_SUCCESS,
                        "failed to add token, status `%s`",
                        pv_status_to_string(status));
                if (status != PV_STATUS_SUCCESS) {
                    return;
                }

                pv_orca_pcm_delete(pcm_chunk);
            }
        }

        int32_t num_samples_chunk = 0;
        int16_t *pcm_chunk = NULL;
        status = pv_orca_stream_flush(orca_stream, &num_samples_chunk, &pcm_chunk);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to flush");
        if (status != PV_STATUS_SUCCESS) {
            return;
        }

        pv_test_true(num_samples_chunk > 0, "failed to flush");

        pv_orca_pcm_delete(pcm_chunk);
    }

    pv_orca_stream_close(orca_stream);
}

#endif

static void test_pv_orca_synthesize_test_alignments(void) {
    int32_t num_samples = 0;
    int16_t *pcm = NULL;
    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;

    pv_status_t status = pv_orca_synthesize(
            orca_object,
            ALIGNMENT_TEST_SENTENCE,
            synthesize_params_object,
            &num_samples,
            &pcm,
            &num_alignments,
            &alignments);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to synthesize; expected '%s' got '%s'",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));

    float prev_word_end_sec = -1.f;
    for (int32_t i = 0; i < num_alignments; i++) {
        pv_orca_word_alignment_t *word = alignments[i];
        pv_test_true(word->start_sec >= 0, "invalid word start");
        pv_test_true(word->end_sec >= 0, "invalid word end");
        pv_test_true(word->start_sec <= word->end_sec, "invalid alignment start and end.");

        pv_test_true(word->num_phonemes == ALIGNMENTS_NUM_PHONEMES[i], "invalid number of phonemes");
        pv_test_true(
                strcmp(word->word, SENTENCE_ALIGNMENTS[i]->word) == 0,
                "invalid word in alignment. got `%s` expected `%s`",
                word->word,
                SENTENCE_ALIGNMENTS[i]->word);

        if (prev_word_end_sec > -1.f) {
            pv_test_close_float(word->start_sec, prev_word_end_sec, 0.0001f, "invalid word start");
        }
        prev_word_end_sec = word->end_sec;

        float prev_phoneme_end_sec = -1.f;
        for (int32_t j = 0; j < word->num_phonemes; j++) {
            pv_orca_phoneme_alignment_t *phoneme = word->phonemes[j];
            pv_test_true(phoneme->start_sec >= 0, "invalid phoneme start");
            pv_test_true(phoneme->end_sec >= 0, "invalid phoneme end");
            pv_test_true(phoneme->start_sec <= phoneme->end_sec, "invalid phoneme start and end");

            pv_test_true(
                    strcmp(phoneme->phoneme, SENTENCE_ALIGNMENTS[i]->phonemes[j]->phoneme) == 0,
                    "invalid phoneme. got `%s` expected `%s`",
                    phoneme->phoneme,
                    SENTENCE_ALIGNMENTS[i]->phonemes[j]->phoneme);

            if (prev_phoneme_end_sec > -1.f) {
                pv_test_close_float(phoneme->start_sec, prev_phoneme_end_sec, 0.0001f, "invalid phoneme start");
            }
            prev_phoneme_end_sec = phoneme->end_sec;
        }
    }

    pv_orca_pcm_delete(pcm);
    pv_orca_word_alignments_delete(num_alignments, alignments);
}

static void test_pv_orca_synthesize_speech_rate(void) {
    int32_t num_samples_slow = 0;
    int32_t num_samples_streaming_slow = 0;
    int32_t num_samples_fast = 0;
    int32_t num_samples_streaming_fast = 0;

    int16_t *pcm = NULL;

    float speech_rate_slow = 0.7f;
    float speech_rate_fast = 1.3f;

    pv_status_t status = pv_orca_synthesize_params_set_speech_rate(synthesize_params_object, speech_rate_slow);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to set speech rate");

    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;
    status = pv_orca_synthesize(
            orca_object,
            SHORT_SENTENCE,
            synthesize_params_object,
            &num_samples_slow,
            &pcm,
            &num_alignments,
            &alignments);
    pv_orca_pcm_delete(pcm);
    pv_orca_word_alignments_delete(num_alignments, alignments);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to synthesize");

    pv_orca_stream_t *stream_slow = NULL;
    status = pv_orca_stream_open(orca_object, synthesize_params_object, &stream_slow);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to open stream");

    status = pv_orca_stream_synthesize(stream_slow, SHORT_SENTENCE, &num_samples_streaming_slow, &pcm);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to synthesize");
    pv_orca_pcm_delete(pcm);
    status = pv_orca_stream_flush(stream_slow, &num_samples_streaming_slow, &pcm);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to flush");
    pv_orca_pcm_delete(pcm);
    pv_orca_stream_close(stream_slow);

    status = pv_orca_synthesize_params_set_speech_rate(synthesize_params_object, speech_rate_fast);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to set speech rate");

    status = pv_orca_synthesize(
            orca_object,
            SHORT_SENTENCE,
            synthesize_params_object,
            &num_samples_fast,
            &pcm,
            &num_alignments,
            &alignments);
    pv_orca_pcm_delete(pcm);
    pv_orca_word_alignments_delete(num_alignments, alignments);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to synthesize");

    pv_orca_stream_t *stream_fast = NULL;
    status = pv_orca_stream_open(orca_object, synthesize_params_object, &stream_fast);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to open stream");

    status = pv_orca_stream_synthesize(stream_fast, SHORT_SENTENCE, &num_samples_streaming_fast, &pcm);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to synthesize");
    pv_orca_pcm_delete(pcm);
    status = pv_orca_stream_flush(stream_fast, &num_samples_streaming_fast, &pcm);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to flush");
    pv_orca_pcm_delete(pcm);
    pv_orca_stream_close(stream_fast);

    pv_test_true(
            num_samples_slow > num_samples_fast,
            "Did not get more samples when using a slower speech rate. got `%d` expected it to be lower than `%d`",
            num_samples_slow,
            num_samples_fast);

    pv_test_true(
            num_samples_streaming_slow > num_samples_streaming_fast,
            "Did not get more samples when using a slower speech rate in streaming synthesis. "
            "got `%d` expected it to be lower than `%d`",
            num_samples_streaming_slow,
            num_samples_streaming_fast);
}

static void test_pv_orca_synthesize_random_state(void) {
    int32_t num_samples = 0;
    int32_t num_samples_same_state = 0;
    int32_t num_samples_different_state = 0;

    int16_t *pcm = NULL;
    int16_t *pcm_same_state = NULL;
    int16_t *pcm_different_state = NULL;

    int64_t state = 17;
    int64_t state_different = 777777;

    pv_status_t status = pv_orca_synthesize_params_set_random_state(synthesize_params_object, state);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to set random state");

    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;

    status = pv_orca_synthesize(
            orca_object,
            SHORT_SENTENCE,
            synthesize_params_object,
            &num_samples,
            &pcm,
            &num_alignments,
            &alignments);
    pv_orca_word_alignments_delete(num_alignments, alignments);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to synthesize");

    status = pv_orca_synthesize(
            orca_object,
            SHORT_SENTENCE,
            synthesize_params_object,
            &num_samples_same_state,
            &pcm_same_state,
            &num_alignments,
            &alignments);
    pv_orca_word_alignments_delete(num_alignments, alignments);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to synthesize");
    pv_test_true(
            num_samples == num_samples_same_state,
            "different number of samples with same state, got `%d` expected `%d`",
            num_samples_same_state,
            num_samples);
    pv_test_close_int16_array(pcm, pcm_same_state, num_samples, 0.0001f, 0, "different pcm with same state");

    status = pv_orca_synthesize_params_set_random_state(synthesize_params_object, state_different);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to set random state");

    status = pv_orca_synthesize(
            orca_object,
            SHORT_SENTENCE,
            synthesize_params_object,
            &num_samples_different_state,
            &pcm_different_state,
            &num_alignments,
            &alignments);
    pv_orca_word_alignments_delete(num_alignments, alignments);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to synthesize");
    pv_test_true(
            num_samples == num_samples_different_state,
            "different number of samples with different state, got `%d` expected `%d`",
            num_samples_same_state,
            num_samples);
    for (int32_t i = 0; i < num_samples; i++) {
        if (pcm[i] != pcm_different_state[i]) {
            break;
        }
        if (i == num_samples - 1) {
            pv_test_true(false, "same pcm with different states!");
        }
    }

    pv_orca_pcm_delete(pcm);
    pv_orca_pcm_delete(pcm_same_state);
    pv_orca_pcm_delete(pcm_different_state);
}

static void test_pv_orca_version(void) {
    pv_test_true(strlen(pv_orca_version()) > 0, "empty version string");
}

static void test_pv_orca_max_characters(void) {
    int32_t max_character_limit = 0;

    pv_status_t status = pv_orca_max_character_limit(NULL, &max_character_limit);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    status = pv_orca_max_character_limit(orca_object, NULL);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument");

    status = pv_orca_max_character_limit(orca_object, &max_character_limit);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to get max character limit");
    pv_test_true(max_character_limit > 0, "max character limit is not positive");
}

#if !defined(__PV_TARGET_NO_FILE_INTERFACE__) && !defined(__PV_TARGET_NO_FILE_SYSTEM__)

static void test_pv_orca_synthesize_to_file_invalid_argument(void) {
    char *output_path = NULL;
    get_tmp_wav_path(&output_path);
    pv_test_true(output_path != NULL, "failed to create tmp file");
    if (!output_path) {
        return;
    }

    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;

    pv_status_t status = pv_orca_synthesize_to_file(
            NULL,
            DEFAULT_SENTENCE,
            synthesize_params_object,
            output_path,
            &num_alignments,
            &alignments);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument in synthesize_to_file");

    status = pv_orca_synthesize_to_file(
            orca_object,
            NULL,
            synthesize_params_object,
            output_path,
            &num_alignments,
            &alignments);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument in synthesize_to_file");

    status = pv_orca_synthesize_to_file(
            orca_object,
            DEFAULT_SENTENCE,
            NULL,
            output_path,
            &num_alignments,
            &alignments);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument in synthesize_to_file");

    status = pv_orca_synthesize_to_file(
            orca_object,
            DEFAULT_SENTENCE,
            synthesize_params_object,
            NULL,
            &num_alignments,
            &alignments);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument in synthesize_to_file");

    status = pv_orca_synthesize_to_file(
            orca_object,
            DEFAULT_SENTENCE,
            synthesize_params_object,
            "",
            &num_alignments,
            &alignments);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with invalid argument in synthesize_to_file");

    if (status != PV_STATUS_INVALID_ARGUMENT) {
        remove(output_path);
    }
    free(output_path);
}

static void test_pv_orca_synthesize_to_file_success(void) {
    char *output_path = NULL;
    get_tmp_wav_path(&output_path);
    pv_test_true(output_path != NULL, "failed to create tmp file");
    if (!output_path) {
        return;
    }

    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;

    pv_status_t status = pv_orca_synthesize_to_file(
            orca_object,
            DEFAULT_SENTENCE,
            synthesize_params_object,
            output_path,
            &num_alignments,
            &alignments);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to run synthesize_to_file; expected '%s' got '%s'",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));

    remove(output_path);
    free(output_path);
    pv_orca_word_alignments_delete(num_alignments, alignments);
}

#endif

#if defined(__PV_TARGET_NO_FILE_INTERFACE__)

static void test_pv_orca_synthesize_to_file_failure(void) {
    pv_log_disable();
    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;
    pv_status_t status = pv_orca_synthesize_to_file(
            orca_object,
            DEFAULT_SENTENCE,
            synthesize_params_object,
            "invalid_path",
            &num_alignments,
            &alignments);
    pv_log_enable();

    pv_test_true(
            status == PV_STATUS_IO_ERROR,
            "expected '%s', got '%s'",
            pv_status_to_string(PV_STATUS_IO_ERROR),
            pv_status_to_string(status));
}

#endif

#ifdef __PV_MOCKS__

static void test_pv_orca_init_internal_helper(
        struct init_args *args,
        pv_status_t expected,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    pv_log_disable();
    pv_status_t status = pv_orca_internal_init(
            args->access_key,
            args->https_client_factory,
            args->model_path,
            pv_ypu_clone(ypu),
            args->object);
    pv_log_enable();
    reset_mocks();
    pv_test_true(
            status == expected,
            "init internal error, expected '%s' got '%s'",
            pv_status_to_string(expected),
            pv_status_to_string(status));
    
    if (expected != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__
        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }
#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "init internal error message mismatch, expected '%s'",
                expected_message);
    }
    
    if (status == PV_STATUS_SUCCESS) {
        pv_orca_delete(*args->object);
    }
}

static void test_pv_orca_synthesize_helper(
        struct synthesize_args *args,
        pv_status_t expected,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    pv_log_disable();
    pv_status_t status = pv_orca_synthesize(
            args->object,
            args->text,
            args->params,
            args->num_samples,
            args->pcm,
            args->num_alignments,
            args->alignments);
    pv_log_enable();
    reset_mocks();
    pv_test_true(
            status == expected,
            "synthesize error, expected '%s' got '%s'",
            pv_status_to_string(expected),
            pv_status_to_string(status));
    
    if (expected != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__
        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }
#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "synthesize error message mismatch, expected '%s'",
                expected_message);
    }
    
    if (status == PV_STATUS_SUCCESS) {
        pv_orca_delete(args->object);
    }
}

static void test_pv_orca_internal_param_load_helper(
        struct init_args *args,
        pv_status_t expected) {
    FILE *f = pv_fopen(args->model_path, "rb");
    pv_test_true(
        f != NULL,
        "failed to open %s",
        args->model_path);
    if (f == NULL) {
        return;
    }

    pv_orca_phonemizer_param_t *phonemizer_param = NULL;
    pv_orca_synthesizer_param_t *synthesizer_param = NULL;
    pv_status_t status = pv_orca_internal_param_load(
        ypu,
        f,
        &phonemizer_param,
        &synthesizer_param);
    fclose(f);
    pv_test_true(
            status == expected,
            "expected '%s' but got '%s'",
            pv_status_to_string(expected),
            pv_status_to_string(status));
    if (status != expected) {
        printf("%d\n", curr_fread);
        exit(1);
    }

    if (status == PV_STATUS_SUCCESS) {
        pv_orca_phonemizer_param_delete(phonemizer_param);
        pv_orca_synthesizer_param_delete(ypu, synthesizer_param);
    }
}

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static void *pv_ypu_host_alloc_return_null(pv_ypu_t *ypu, int32_t size_bytes) {
    (void) ypu;
    (void) size_bytes;
    return NULL;
}

static void *malloc_return_null(size_t arg0) {
    (void) arg0;

    return NULL;
}

static FILE *pv_fopen_return_null(const char *arg0, const char *arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static size_t pv_fread_fail(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (fread_count < curr_fread - 1) {
        fread_count++;
        return pv_fread_real(ptr, size, nmemb, stream);
    }

    return 0;
}

static void test_pv_orca_init_success(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_internal_init, PV_STATUS_SUCCESS)

    pv_orca_t *pv_orca = NULL;
    const pv_status_t status = pv_orca_init(FAKE_ACCESS_KEY, "model_path", "cpu:1", &pv_orca);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to init orca with '%s'",
            pv_status_to_string(status));
}

static void test_pv_orca_init_internal_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_internal_init, PV_STATUS_INVALID_ARGUMENT)

    pv_orca_t *pv_orca = NULL;
    const pv_status_t status = pv_orca_init(FAKE_ACCESS_KEY, "", "cpu:1", &pv_orca);
    pv_test_true(
            status == PV_STATUS_INVALID_ARGUMENT,
            "expected '%s' but got '%s'",
            pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
            pv_status_to_string(status));

    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_orca_internal_init` failed with status `INVALID_ARGUMENT`\\.",
            true,
            "init internal init failure error message mismatch");
}

static void test_pv_orca_init_https_client_factory_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(calloc, calloc_return_null)

    pv_orca_t *pv_orca = NULL;
    const pv_status_t status = pv_orca_init(FAKE_ACCESS_KEY, "", "cpu:1", &pv_orca);
    reset_mocks();
    pv_test_true(
            status == PV_STATUS_OUT_OF_MEMORY,
            "expected '%s' but got '%s'",
            pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
            pv_status_to_string(status));

    pv_test_error_message(
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `https_client_factory`\\.",
            true,
            "init https client factory failure error message mismatch");
}

static void test_pv_orca_init_internal_invalid_args(void) {
    struct init_args args;

    args = default_init_args;
    args.access_key = NULL;
    test_pv_orca_init_internal_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `access_key` is NULL\\.", NULL);

    args = default_init_args;
    args.model_path = NULL;
    test_pv_orca_init_internal_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `model_path` is NULL\\.", NULL);

    args = default_init_args;
    args.object = NULL;
    test_pv_orca_init_internal_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `object` is NULL\\.", NULL);
}

static void test_pv_orca_init_internal_alloc_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_ypu_init_cpu, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_ypu_host_alloc, pv_ypu_host_alloc_return_null)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `o`\\.");
}

static void test_pv_orca_init_internal_fopen_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fopen, pv_fopen_return_null)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_IO_ERROR, "Failed to open file `.*`\\.", NULL);
}

static void test_pv_orca_init_internal_synthesizer_param_load_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_synthesizer_param_load, PV_STATUS_IO_ERROR)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_IO_ERROR, pv_test_function_hash_regex(), "`pv_orca_internal_param_load` failed with status `IO_ERROR`\\.");
}

static void test_pv_orca_init_internal_serialized_section_unpack_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_serialized_deserialize_file, PV_STATUS_IO_ERROR)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_IO_ERROR, pv_test_function_hash_regex(), "`pv_orca_internal_param_load` failed with status `IO_ERROR`\\.");
}

static void test_pv_orca_init_internal_hippo_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_hippo_init2, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Picovoice Error", "`pv_hippo` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_init_internal_fclose_failure(void) {
    PV_SET_MOCK_RETURN_VAL(fclose, 1)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_IO_ERROR, "Failed to close file `.*`\\.", NULL);
}

static void test_pv_orca_init_internal_normalizer_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Picovoice Error", "`pv_normalizer_init` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_init_internal_phonemizer_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_phonemizer_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Picovoice Error", "`pv_orca_phonemizer_init` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_init_internal_stream_state_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_INVALID_ARGUMENT, "Picovoice Error", "`pv_orca_stream_state_init` failed to init with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_orca_init_internal_synthesizer_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_synthesizer_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Picovoice Error", "`pv_orca_synthesizer` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_init_internal_pv_gatekeeper_usage_animal_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_gatekeeper_usage_animal_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_init_internal_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Picovoice Error", "`pv_gatekeeper_usage_animal` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_internal_param_load_fread_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_fail)

    for (curr_fread = 0; curr_fread < 821; curr_fread++) {
        fread_count = 0;

        test_pv_orca_internal_param_load_helper(&default_init_args, PV_STATUS_IO_ERROR);
    }
}

static void test_pv_orca_stream_state_update_z_prior_failure_helper(
        pv_status_t expected,
        bool is_first_chunk,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    int32_t length = 100;

    pv_ypu_mem_t *buffer_z_prior = pv_ypu_mem_alloc(
        ypu,
        length * stream_state_object->num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_z_prior != NULL, "Failed to allocate buffer_z_prior");
    if (buffer_z_prior == NULL) {
        return;
    }

    stream_state_object->is_first_chunk = is_first_chunk;
    stream_state_object->is_flush = false;
    int32_t num_frames_to_flow = length;
    pv_status_t status = pv_orca_stream_state_update_z_prior(
            ypu,
            stream_state_object,
            &buffer_z_prior,
            &num_frames_to_flow);
    reset_mocks();
    pv_test_true(
            status == expected,
            "failed to update z prior, expected '%s' got '%s'",
            pv_status_to_string(expected),
            pv_status_to_string(status));

    if (status != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__
        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }
#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "stream state update z prior error message mismatch, expected '%s'",
                expected_message);
    }

    pv_ypu_mem_free(ypu, buffer_z_prior);
}

static void test_pv_orca_stream_state_update_z_prior_1st_buffer_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_ypu_buffer_get, NULL)

    test_pv_orca_stream_state_update_z_prior_failure_helper(PV_STATUS_OUT_OF_MEMORY, false, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `pv_buffer_get`\\.");
}

static void test_pv_orca_stream_state_update_z_failure_helper(
        pv_status_t expected,
        bool is_first_chunk,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    int32_t length = 10;

    pv_ypu_mem_t *buffer_z = pv_ypu_mem_alloc(
        ypu,
        length * stream_state_object->num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_z != NULL, "Failed to allocate buffer_z");
    if (buffer_z == NULL) {
        return;
    }

    stream_state_object->is_first_chunk = is_first_chunk;
    stream_state_object->is_flush = false;
    int32_t num_frames_to_flow = length;
    pv_status_t status = pv_orca_stream_state_update_z_prior(
            ypu,
            stream_state_object,
            &buffer_z,
            &num_frames_to_flow);
    reset_mocks();
    pv_test_true(
            status == expected,
            "failed to update z prior, expected '%s' got '%s'",
            pv_status_to_string(expected),
            pv_status_to_string(status));

    if (status != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__
        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }
#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "stream state update z error message mismatch, expected '%s'",
                expected_message);
    }

    pv_ypu_mem_free(ypu, buffer_z);
}

static void test_pv_orca_stream_state_update_z_1st_buffer_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_ypu_buffer_get, NULL)

    test_pv_orca_stream_state_update_z_failure_helper(PV_STATUS_OUT_OF_MEMORY, false, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `pv_buffer_get`\\.");
}

static void test_pv_orca_synthesize_invalid_args(void) {
    struct synthesize_args args;

    args = default_synthesize_args;
    args.object = NULL;
    test_pv_orca_synthesize_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `object` is NULL\\.", NULL);

    args = default_synthesize_args;
    args.text = NULL;
    test_pv_orca_synthesize_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `text` is NULL\\.", NULL);

    args = default_synthesize_args;
    args.text = "";
    test_pv_orca_synthesize_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `text` is invalid\\.", NULL);

    args = default_synthesize_args;
    char *long_text = calloc(MAX_CHARACTER_LIMIT + 2, sizeof(char));
    if (!long_text) {
        return;
    }
    for (int32_t i = 0; i < MAX_CHARACTER_LIMIT + 1; i++) {
        long_text[i] = 'a';
    }
    long_text[MAX_CHARACTER_LIMIT + 1] = '\0';
    args.text = long_text;
    test_pv_orca_synthesize_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Maximum character limit of `2000` exceeded\\.", NULL);
    free(long_text);

    args = default_synthesize_args;
    args.params = NULL;
    test_pv_orca_synthesize_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `synthesize_params` is NULL\\.", NULL);

    args = default_synthesize_args;
    args.num_samples = NULL;
    test_pv_orca_synthesize_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `num_samples` is NULL\\.", NULL);

    args = default_synthesize_args;
    args.num_alignments = NULL;
    test_pv_orca_synthesize_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `num_alignments` is NULL\\.", NULL);

    args = default_synthesize_args;
    args.alignments = NULL;
    test_pv_orca_synthesize_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `alignments` is NULL\\.", NULL);

    args = default_synthesize_args;
    args.pcm = NULL;
    test_pv_orca_synthesize_helper(&args, PV_STATUS_INVALID_ARGUMENT, "Argument `pcm` is NULL\\.", NULL);
}

static void test_pv_orca_synthesize_normalizer_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_normalize, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesize_helper(&default_synthesize_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_phonemize_text` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesize_phonemizer_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_phonemizer_phonemize, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesize_helper(&default_synthesize_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_phonemize_text` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_phonemizer_phonemize, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_synthesizer_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesize_helper(&default_synthesize_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_synthesizer_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_create_alignments_failure_helper(
        pv_status_t expected,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    pv_normalizer_token_t text_token_one = {
            .string = "FIVE",
            .original_string = "5%",
            .verbalized = "FIVE",
            .tag = PV_NORMALIZER_TAG_NONE,
            .length_future_context = 1,
            .length_past_context = 0,
            .pronunciation = NULL,
            .next = NULL,
            .previous = NULL};
    pv_normalizer_token_t text_token_two = {
            .string = "PERCENT",
            .original_string = "5%",
            .verbalized = "PERCENT",
            .tag = PV_NORMALIZER_TAG_NONE,
            .length_future_context = 0,
            .length_past_context = 1,
            .pronunciation = NULL,
            .next = NULL,
            .previous = NULL};
    int32_t num_text_tokens = 2;
    pv_normalizer_token_t *text_tokens[2] = {&text_token_one, &text_token_two};
    int32_t text_tokens_num_encoded_phonemes[2] = {7, 12};
    int32_t encoded_phonemes[] = {26, 27, 10, 11, 68, 69, 96, 52, 53, 22, 23, 56, 57, 20, 21, 44, 45, 60, 61};

    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;
    pv_status_t status = pv_orca_create_word_alignments(
            orca_object,
            num_text_tokens,
            text_tokens,
            text_tokens_num_encoded_phonemes,
            encoded_phonemes,
            encoded_phonemes,
            &num_alignments,
            &alignments);
    reset_mocks();
    pv_test_true(
            status == expected,
            "failed to fail. Got '%s' expected '%s'",
            pv_status_to_string(status),
            pv_status_to_string(expected));

    if (expected != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }
#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                false,
                "create alignments error message mismatch, expected '%s'",
                expected_message);
    }
}

static void test_pv_orca_create_alignments_1st_alloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(calloc, calloc_return_null)

    test_pv_orca_create_alignments_failure_helper(PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `aligns`\\.");
}

static void test_pv_orca_create_alignments_2nd_alloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    test_pv_orca_create_alignments_failure_helper(PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `indices_merged`\\.");
}

static void test_pv_orca_create_alignments_3rd_alloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    test_pv_orca_create_alignments_failure_helper(PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `phonemes`\\.");
}

static void test_pv_orca_create_alignments_failure_path_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_sample_rate, PV_STATUS_INVALID_ARGUMENT)

    test_pv_orca_create_alignments_failure_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_orca_sample_rate` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_orca_create_alignments_failure_path_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_language_info_phoneme_index_to_string, PV_STATUS_INVALID_ARGUMENT)

    test_pv_orca_create_alignments_failure_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_language_info_phoneme_index_to_string` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_orca_create_alignments_failure_path_3(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_phoneme_alignment_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_create_alignments_failure_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_phoneme_alignment_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_create_alignments_failure_path_4(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_word_alignment_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_create_alignments_failure_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_word_alignment_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_create_alignments_failure_path_5(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_merge_word_alignments, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_create_alignments_failure_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_merge_word_alignments` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_create_alignments_failure_path_6(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_phoneme_alignment_copy, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_create_alignments_failure_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_phoneme_alignment_copy` failed with status `OUT_OF_MEMORY`\\.");
}

static pv_status_t pv_orca_word_alignment_init_return_oom(
        const char *arg0,
        float arg1,
        float arg2,
        int32_t arg3,
        pv_orca_phoneme_alignment_t **arg4,
        pv_orca_word_alignment_t **arg5) {
    (void) arg0;
    (void) arg1;
    (void) arg2;
    (void) arg3;
    (void) arg4;
    (void) arg5;

    return PV_STATUS_OUT_OF_MEMORY;
}

static void test_pv_orca_create_alignments_failure_path_7(void) {
    pv_status_t (*custom_funcs[])(
            const char *arg0,
            float arg1,
            float arg2,
            int32_t arg3,
            pv_orca_phoneme_alignment_t **arg4,
            pv_orca_word_alignment_t **arg5) = {
            pv_orca_word_alignment_init_real,
            pv_orca_word_alignment_init_real,
            pv_orca_word_alignment_init_return_oom,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_orca_word_alignment_init, custom_funcs)

    test_pv_orca_create_alignments_failure_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_word_alignment_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_valid_characters_invalid_inputs(void) {
    int32_t num_characters = 0;
    const char *const *characters = NULL;

    pv_status_t status = pv_orca_valid_characters(NULL, &num_characters, &characters);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with PV_STATUS_INVALID_ARGUMENT");
    pv_test_error_message(
            "Argument `object` is NULL\\.",
            NULL,
            true,
            "failed to fail with PV_STATUS_INVALID_ARGUMENT for NULL orca_object");

    status = pv_orca_valid_characters(orca_object, NULL, &characters);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with PV_STATUS_INVALID_ARGUMENT");
    pv_test_error_message(
            "Argument `num_characters` is NULL\\.",
            NULL,
            true,
            "failed to fail with PV_STATUS_INVALID_ARGUMENT for NULL num_characters");

    status = pv_orca_valid_characters(orca_object, &num_characters, NULL);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with PV_STATUS_INVALID_ARGUMENT");
    pv_test_error_message(
            "Argument `characters` is NULL\\.",
            NULL,
            true,
            "failed to fail with PV_STATUS_INVALID_ARGUMENT for NULL characters");
}

static void test_pv_orca_valid_characters_failure_helper(
        pv_status_t expected,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    int32_t num_characters = 0;
    const char *const *characters = NULL;
    pv_status_t status = pv_orca_valid_characters(orca_object, &num_characters, &characters);
    reset_mocks();
    pv_test_true(status == expected, "failed to fail with %s", pv_status_to_string(expected));
    if (expected != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }

#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "valid characters error message mismatch, expected '%s'",
                expected_message);
    }
    if (status == PV_STATUS_SUCCESS) {
        pv_orca_valid_characters_delete(characters);
    }
}

static void test_pv_orca_valid_characters_malloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(malloc, malloc_return_null)

    test_pv_orca_valid_characters_failure_helper(PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `characters_internal`\\.");
}

static void test_pv_orca_sample_rate_failure(void) {
    pv_status_t status = pv_orca_sample_rate(orca_object, NULL);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with PV_STATUS_INVALID_ARGUMENT");
}

static void pv_orca_phoneme_alignment_init_failure_helper(
        pv_status_t expected,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    const char *phoneme_string = "AA";
    pv_orca_phoneme_alignment_t *phoneme = NULL;
    pv_status_t status = pv_orca_phoneme_alignment_init(phoneme_string, 0.0f, 0.1f, &phoneme);
    reset_mocks();
    pv_test_true(
            status == expected,
            "failed to fail with `%s`",
            pv_status_to_string(expected));
    if (expected != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }

#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "phoneme alignment init error message mismatch, expected '%s'",
                expected_message);
    }
}

static void pv_orca_phoneme_alignment_init_1st_alloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(calloc, calloc_return_null)

    pv_orca_phoneme_alignment_init_failure_helper(
            PV_STATUS_OUT_OF_MEMORY, 
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `o`\\.");
}

static void pv_orca_phoneme_alignment_init_2nd_alloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    pv_orca_phoneme_alignment_init_failure_helper(
            PV_STATUS_OUT_OF_MEMORY, 
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `o->phoneme`\\.");
}

static void pv_orca_word_alignment_init_failure_helper(
        pv_status_t expected,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    pv_orca_phoneme_alignment_t phoneme1 = {.phoneme = "W", .start_sec = 0.0f, .end_sec = 0.1f};
    pv_orca_phoneme_alignment_t *phonemes[1] = {&phoneme1};

    const char *word_string = "word";
    pv_orca_word_alignment_t *word = NULL;
    pv_status_t status = pv_orca_word_alignment_init(word_string, 0.0f, 0.1f, 1, phonemes, &word);
    reset_mocks();
    pv_test_true(
            status == expected,
            "failed to fail with `%s`",
            pv_status_to_string(expected));

    if (expected != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }

#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "word alignment init error message mismatch, expected '%s'",
                expected_message);
    }
}

static void pv_orca_word_alignment_init_1st_alloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(calloc, calloc_return_null)

    pv_orca_word_alignment_init_failure_helper(
            PV_STATUS_OUT_OF_MEMORY, 
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `o`\\.");
}

static void pv_orca_word_alignment_init_2nd_alloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    pv_orca_word_alignment_init_failure_helper(
            PV_STATUS_OUT_OF_MEMORY,
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `o->word`\\.");
}

static void test_pv_orca_synthesize_params_set_speech_rate_invalid_inputs(void) {
    pv_status_t status = pv_orca_synthesize_params_set_speech_rate(NULL, 0.5f);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with PV_STATUS_INVALID_ARGUMENT");
}

static void test_pv_orca_synthesize_params_get_speech_rate_invalid_inputs(void) {
    float speech_rate = 0.0f;

    pv_status_t status = pv_orca_synthesize_params_get_speech_rate(NULL, &speech_rate);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with PV_STATUS_INVALID_ARGUMENT");
    pv_test_error_message(
            "Argument `object` is NULL\\.",
            NULL,
            true,
            "failed to fail with PV_STATUS_INVALID_ARGUMENT for NULL synthesize_params_object");

    status = pv_orca_synthesize_params_get_speech_rate(synthesize_params_object, NULL);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with PV_STATUS_INVALID_ARGUMENT");
    pv_test_error_message(
            "Argument `speech_rate` is NULL\\.",
            NULL,
            true,
            "failed to fail with PV_STATUS_INVALID_ARGUMENT for NULL speech_rate");
}

static void test_pv_orca_synthesize_params_set_random_state_invalid_inputs(void) {
    pv_status_t status = pv_orca_synthesize_params_set_random_state(NULL, 17);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with PV_STATUS_INVALID_ARGUMENT");
    pv_test_error_message(
            "Argument `object` is NULL\\.",
            NULL,
            true,
            "failed to fail with PV_STATUS_INVALID_ARGUMENT for NULL synthesize_params_object");

    status = pv_orca_synthesize_params_set_random_state(synthesize_params_object, -1);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to set random state");
    pv_test_error_message(
            "Argument `random_state` value `-1` is less than the minimum acceptable value of `0`\\.",
            NULL,
            true,
            "failed to fail with PV_STATUS_INVALID_ARGUMENT for negative random_state");
}

static void test_pv_orca_synthesize_params_get_random_state_invalid_inputs(void) {
    int64_t random_state = 0;

    pv_status_t status = pv_orca_synthesize_params_get_random_state(NULL, &random_state);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with PV_STATUS_INVALID_ARGUMENT");
    pv_test_error_message(
            "Argument `object` is NULL\\.",
            NULL,
            true,
            "failed to fail with PV_STATUS_INVALID_ARGUMENT for NULL synthesize_params_object");

    status = pv_orca_synthesize_params_get_random_state(synthesize_params_object, NULL);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with PV_STATUS_INVALID_ARGUMENT");
    pv_test_error_message(
            "Argument `random_state` is NULL\\.",
            NULL,
            true,
            "failed to fail with PV_STATUS_INVALID_ARGUMENT for NULL random_state");
}

#if !defined(__PV_TARGET_NO_FILE_INTERFACE__) && !defined(__PV_TARGET_NO_FILE_SYSTEM__)

static const char *OUTPUT_FILE_PATH = "test.wav";

static void pv_writer_wav_delete_overwrite(pv_writer_wav_t *object) {
    (void) object;
}

static void test_pv_orca_synthesize_to_file_writer_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_synthesizer_sample_rate, 22050)
    PV_SET_MOCK_RETURN_VAL(pv_writer_wav_init, PV_STATUS_OUT_OF_MEMORY)
    PV_SET_MOCK_RETURN_VAL(pv_writer_wav_write, PV_STATUS_SUCCESS)
    struct synthesize_args args = default_synthesize_args;
    pv_status_t status = pv_orca_synthesize_to_file(
            args.object,
            args.text,
            args.params,
            OUTPUT_FILE_PATH,
            args.num_alignments,
            args.alignments);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to file with OUT_OF_MEMORY");
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_writer_wav_init` failed with status `OUT_OF_MEMORY`\\.",
            true,
            "failed to fail with OUT_OF_MEMORY for writer init");
}

static void test_pv_orca_synthesize_to_file_synthesize_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_synthesizer_sample_rate, 22050)
    PV_SET_MOCK_RETURN_VAL(pv_writer_wav_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_writer_wav_delete, pv_writer_wav_delete_overwrite)
    PV_SET_MOCK_RETURN_VAL(pv_orca_synthesize_internal, PV_STATUS_OUT_OF_MEMORY)
    PV_SET_MOCK_RETURN_VAL(pv_writer_wav_write, PV_STATUS_SUCCESS)

    struct synthesize_args args = default_synthesize_args;
    pv_status_t status = pv_orca_synthesize_to_file(
            args.object,
            args.text,
            args.params,
            OUTPUT_FILE_PATH,
            args.num_alignments,
            args.alignments);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with OUT_OF_MEMORY");
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_orca_synthesize_internal` failed with status `OUT_OF_MEMORY`\\.",
            true,
            "failed to fail with OUT_OF_MEMORY for synthesize internal");
}

static void test_pv_orca_synthesize_to_file_writer_write_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_synthesizer_sample_rate, 22050)
    PV_SET_MOCK_RETURN_VAL(pv_writer_wav_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_writer_wav_delete, pv_writer_wav_delete_overwrite)
    PV_SET_MOCK_RETURN_VAL(pv_orca_synthesize_internal, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_writer_wav_write, PV_STATUS_IO_ERROR)

    struct synthesize_args args = default_synthesize_args;
    pv_status_t status = pv_orca_synthesize_to_file(
            args.object,
            args.text,
            args.params,
            OUTPUT_FILE_PATH,
            args.num_alignments,
            args.alignments);
    pv_test_true(status == PV_STATUS_IO_ERROR, "failed to fail with IO_ERROR");
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_writer_wav_write` failed with status `IO_ERROR`\\.",
            true,
            "failed to fail with IO_ERROR for writer write");
}

#endif

static void test_pv_orca_stream_state_append_phonemes(void) {
    int32_t num_samples = 0;
    int16_t *pcm = NULL;
    pv_orca_stream_t *orca_stream = NULL;
    pv_status_t status = pv_orca_stream_open(orca_object, synthesize_params_object, &orca_stream);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to open stream, got status `%s`",
            pv_status_to_string(status));

    pv_orca_stream_synthesize_internal(
            orca_stream,
            false,
            false,
            DEFAULT_SENTENCE,
            false,
            &num_samples,
            &pcm);
    pv_orca_pcm_delete(pcm);

    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_index_past_cutoff, 10)

    pv_orca_stream_state_t *stream_state = pv_orca_stream_state_get(orca_object);
    pv_test_true(stream_state != NULL, "failed to get stream state");

    int32_t num_encoded_phonemes = 5000;
    stream_state->end_index_dp = num_encoded_phonemes - stream_state->config->receptive_field_duration_predictor;
    int32_t end_index_dp_before = stream_state->end_index_dp;
    int32_t *encoded_phonemes = calloc(num_encoded_phonemes, sizeof(int32_t));
    status = pv_orca_stream_state_append_encoded_phonemes(
            stream_state,
            num_encoded_phonemes,
            encoded_phonemes);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to append encoded phonemes");
    pv_test_true(
            stream_state->end_index_dp < end_index_dp_before,
            "end index dp did not decrease, expected `%d` to be smaller than %d",
            stream_state->end_index_dp,
            end_index_dp_before);

    pv_orca_stream_close(orca_stream);
}

static void test_pv_orca_stream_state_append_phonemes_failure_helper(
        pv_status_t expected,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    pv_orca_stream_state_t *stream_state = pv_orca_stream_state_get(orca_object);
    pv_test_true(stream_state != NULL, "failed to get stream state");

    int32_t num_encoded_phonemes = 5000;
    int32_t *encoded_phonemes = calloc(num_encoded_phonemes, sizeof(int32_t));
    pv_status_t status = pv_orca_stream_state_append_encoded_phonemes(
            stream_state,
            num_encoded_phonemes,
            encoded_phonemes);
    reset_mocks();
    pv_test_true(
            status == expected,
            "failed to append encoded phonemes. got `%s`, expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
    if (expected != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }

#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "stream state append phonemes error message mismatch, expected '%s'",
                expected_message);
    }
}

static void test_pv_orca_stream_state_append_phonemes_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_buffer_int32_append, PV_STATUS_OUT_OF_MEMORY);

    test_pv_orca_stream_state_append_phonemes_failure_helper(
            PV_STATUS_OUT_OF_MEMORY, 
            pv_test_function_hash_regex(),
            "`pv_buffer_int32_append` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_stream_state_append_phonemes_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_index_past_cutoff, 10)
    PV_SET_MOCK_RETURN_VAL(pv_buffer_int32_remove_from_start, PV_STATUS_INVALID_ARGUMENT);

    test_pv_orca_stream_state_append_phonemes_failure_helper(
            PV_STATUS_INVALID_ARGUMENT,
            pv_test_function_hash_regex(),
            "`pv_buffer_int32_remove_from_start` failed with status `INVALID_ARGUMENT`\\.");
}

#endif

#ifdef __PV_BUILD_APPS__

static void test_pv_orca_serializer_loop(void) {
    char tmp_path[L_tmpnam];
    char *result = tmpnam(tmp_path);
    pv_test_true(result != NULL, "failed to create tmp file");
    if (!result) {
        return;
    }

    pv_status_t status = pv_orca_internal_param_serialize(
            ypu,
            &PV_ORCA_PHONEMIZER_PARAM,
            &PV_ORCA_SYNTHESIZER_PARAM,
            tmp_path);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to serialize");
    if (status != PV_STATUS_SUCCESS) {
        remove(tmp_path);
        return;
    }

    FILE *f = pv_fopen(tmp_path, "rb");
    if (!f) {
        LOG_ERROR("failed to load file at path `%s`", tmp_path);
        return;
    }

    pv_orca_phonemizer_param_t *loaded_phonemizer_param = NULL;
    pv_orca_synthesizer_param_t *loaded_synthesizer_param = NULL;
    status = pv_orca_internal_param_load(ypu, f, &loaded_phonemizer_param, &loaded_synthesizer_param);
    remove(tmp_path);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to load");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_test_true(
            pv_orca_synthesizer_param_is_equal(loaded_synthesizer_param, &PV_ORCA_SYNTHESIZER_PARAM),
            "loaded synthesizer param is not equal to the original");

    pv_orca_synthesizer_param_delete(ypu, loaded_synthesizer_param);
    pv_orca_phonemizer_param_delete(loaded_phonemizer_param);
}

#endif

static void test_pv_orca_get_normalizer(void) {
    pv_normalizer_t *normalizer = pv_orca_get_normalizer(orca_object);
    pv_test_true(normalizer != NULL, "failed to get normalizer");
}


static void test_pv_orca_list_hardware_devices(void) {
    char **hardware_devices = NULL;
    int32_t num_hardware_devices = 0;
    pv_status_t status = pv_orca_list_hardware_devices(
        &hardware_devices,
        &num_hardware_devices);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "`pv_orca_list_hardware_devices` failed with `%s`", pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_test_true(
            num_hardware_devices > 0,
            "`num_hardware_devices` should be a positive number, got %d", num_hardware_devices);
    if (num_hardware_devices <= 0) {
        return;
    }

    pv_test_true(hardware_devices, "`hardware_devices` is NULL");
    if (!hardware_devices) {
        return;
    }

    for (int32_t i = 0; i < num_hardware_devices; i++) {
        pv_test_true(hardware_devices[i], "`hardware_devices[%d]` is NULL", i);
        if (!hardware_devices[i]) {
            return;
        }
        pv_test_true(strlen(hardware_devices[i]) > 0, "`hardware_devices[%d]` is an empty string", i);
    }

    pv_orca_free_hardware_devices(hardware_devices, num_hardware_devices);
}

static const pv_test_case_t PV_ORCA_TEST_CASES[] = {

#ifdef __PV_MOCKS__

        {"init success", test_pv_orca_init_success},
        {"init internal init failure", test_pv_orca_init_internal_init_failure},
        {"init https client failure", test_pv_orca_init_https_client_factory_failure},
        {"init internal invalid args", test_pv_orca_init_internal_invalid_args},
        {"init internal alloc failure", test_pv_orca_init_internal_alloc_failure},
        {"init internal fopen failure", test_pv_orca_init_internal_fopen_failure},
        {"init internal synthesizer_param_load failure", test_pv_orca_init_internal_synthesizer_param_load_failure},
        {"init internal streaming synthesis state init failure",
         test_pv_orca_init_internal_stream_state_init_failure},
        {"init internal synthesizer_init failure", test_pv_orca_init_internal_synthesizer_init_failure},
        {"init internal serialized section unpack", test_pv_orca_init_internal_serialized_section_unpack_failure},
        {"init internal hippo_init2 failure", test_pv_orca_init_internal_hippo_init_failure},
        {"init internal normalizer_init failure", test_pv_orca_init_internal_normalizer_init_failure},
        {"init internal phonemizer_init failure", test_pv_orca_init_internal_phonemizer_init_failure},
        {"init internal fclose_failure", test_pv_orca_init_internal_fclose_failure},
        {"init internal pv_gatekeeper_usage_animal_init failure",
         test_pv_orca_init_internal_pv_gatekeeper_usage_animal_init_failure},
        {"internal param load fread failure", test_pv_orca_internal_param_load_fread_failure},
        {"stream state update z_prior failure 1st buffer",
         test_pv_orca_stream_state_update_z_prior_1st_buffer_failure},
        {"stream state update z failure 1st buffer",
         test_pv_orca_stream_state_update_z_1st_buffer_failure},
        {"synthesize invalid args", test_pv_orca_synthesize_invalid_args},
        {"synthesize normalizer failure", test_pv_orca_synthesize_normalizer_failure},
        {"synthesize phonemizer failure", test_pv_orca_synthesize_phonemizer_failure},
        {"synthesize synthesizer forward failure", test_pv_orca_synthesizer_forward_failure},
        {"create alignments 1st alloc failure",
         test_pv_orca_create_alignments_1st_alloc_failure},
        {"create alignments 2nd alloc failure",
         test_pv_orca_create_alignments_2nd_alloc_failure},
        {"create alignments 3rd alloc failure",
         test_pv_orca_create_alignments_3rd_alloc_failure},
        {"create alignments failure path 1", test_pv_orca_create_alignments_failure_path_1},
        {"create alignments failure path 2", test_pv_orca_create_alignments_failure_path_2},
        {"create alignments failure path 3", test_pv_orca_create_alignments_failure_path_3},
        {"create alignments failure path 4", test_pv_orca_create_alignments_failure_path_4},
        {"create alignments failure path 5", test_pv_orca_create_alignments_failure_path_5},
        {"create alignments failure path 6", test_pv_orca_create_alignments_failure_path_6},
        {"create alignments failure path 7", test_pv_orca_create_alignments_failure_path_7},
        {"valid characters invalid inputs", test_pv_orca_valid_characters_invalid_inputs},
        {"valid characters malloc failure", test_pv_orca_valid_characters_malloc_failure},
        {"sample rate failure", test_pv_orca_sample_rate_failure},
        {"phoneme alignment init 1st alloc failure", pv_orca_phoneme_alignment_init_1st_alloc_failure},
        {"phoneme alignment init 2nd alloc failure", pv_orca_phoneme_alignment_init_2nd_alloc_failure},
        {"word alignment init 1st alloc failure", pv_orca_word_alignment_init_1st_alloc_failure},
        {"word alignment init 2nd alloc failure", pv_orca_word_alignment_init_2nd_alloc_failure},
        {"synthesize params set speech rate invalid inputs",
         test_pv_orca_synthesize_params_set_speech_rate_invalid_inputs},
        {"synthesize params get speech rate invalid inputs",
         test_pv_orca_synthesize_params_get_speech_rate_invalid_inputs},
        {"synthesize params set random state invalid inputs",
         test_pv_orca_synthesize_params_set_random_state_invalid_inputs},
        {"synthesize params get random state invalid inputs",
         test_pv_orca_synthesize_params_get_random_state_invalid_inputs},
        {"stream state append phonemes", test_pv_orca_stream_state_append_phonemes},
        {"stream state append phonemes failure 1", test_pv_orca_stream_state_append_phonemes_failure_1},
        {"stream state append phonemes failure 2", test_pv_orca_stream_state_append_phonemes_failure_2},

#if !defined(__PV_TARGET_NO_FILE_INTERFACE__) && !defined(__PV_TARGET_NO_FILE_SYSTEM__)

        {"synthesize to file synthesize failure", test_pv_orca_synthesize_to_file_synthesize_failure},
        {"synthesize to file writer init failure", test_pv_orca_synthesize_to_file_writer_init_failure},
        {"synthesize to file writer write failure", test_pv_orca_synthesize_to_file_writer_write_failure},

#endif

#endif

        {"get valid characters", test_pv_orca_valid_characters},
        {"get normalizer", test_pv_orca_get_normalizer},
        {"test valid characters sentences", test_pv_orca_valid_characters_sentences},
        {"set and get speech rate", test_pv_orca_synthesize_params_set_and_get_speech_rate},
        {"get sample rate", test_pv_orca_get_sample_rate},
        {"stream invalid streaming state", test_pv_orca_stream_invalid_stream_state},
        {"streaming synthesis input robustness", test_pv_orca_streaming_synthesize_robustness},
        {"synthesize success", test_pv_orca_synthesize},
        {"synthesize custom pron success", test_pv_orca_synthesize_custom_pron},
        {"synthesize alignments", test_pv_orca_synthesize_test_alignments},
        {"synthesize speech rate", test_pv_orca_synthesize_speech_rate},
        {"synthesize random state", test_pv_orca_synthesize_random_state},
        {"test version", test_pv_orca_version},
        {"test max characters", test_pv_orca_max_characters},
        {"stream invalid arguments", test_pv_orca_stream_invalid_args},

#if !defined(__PV_TARGET_NO_FILE_INTERFACE__) && !defined(__PV_TARGET_NO_FILE_SYSTEM__)

        {"synthesize to file invalid arguments", test_pv_orca_synthesize_to_file_invalid_argument},
        {"synthesize to file success", test_pv_orca_synthesize_to_file_success},

#endif

#if defined(__PV_TARGET_NO_FILE_INTERFACE__)

        {"synthesize to file failure", test_pv_orca_synthesize_to_file_failure},

#endif

#ifdef __PV_BUILD_APPS__

        {"test serializer loop", test_pv_orca_serializer_loop},

#endif

#ifdef __PV_ENABLE_RELEASE_TESTS__

        {"test stream synthesize long", test_pv_orca_stream_synthesize_long},

#endif

        {"list hardware devices", test_pv_orca_list_hardware_devices},
};

const pv_test_suite_t PV_ORCA_TEST_SUITE = {
        .name = "orca",
        .setup = test_pv_orca_setup,
        .teardown = test_pv_orca_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_TEST_CASES),
        .test_cases = PV_ORCA_TEST_CASES,
};
