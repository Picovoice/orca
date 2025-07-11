#include <stdlib.h>
#include <string.h>

#include "orca/normalizer/pv_normalizer_stream.h"
#include "orca/normalizer/test_pv_normalizer_cases_helper.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"
#include "picollm/mock/pv_picollm_tokenizer_mock.h"

#endif

const int32_t EN_IGNORED_CASES[] = {
        77, 125, 155, 156, 267, 294, 338, 357, 363, 366,
        380, 434, 485, 550, 569, 576, 580, 628, 635, 642,
        649, 664, 668, 682, 709, 725, 787, 819, 821, 918,
        929, 953, 990, 1017, 1033, 1060, 1112, 1115, 1139, 1144,
        1195, 1200, 1241, 1262, 1297, 1349, 1417, 1431, 1449, 1464,
        1467, 1478, 1480, 1511, 1525, 1537, 1615, 1673, 1714, 1739,
        1743, 1788, 1865, 1869, 1901, 1910, 1917, 1944, 1986, 1995,
        2000, 2008, 2041, 2135, 2150, 2155, 2182, 2200, 2282, 2288,
        2314, 2340, 2359, 2372, 2374, 2380, 2405, 2428, 2443, 2452,
        2456, 2482, 2517, 2522, 2528, 2548, 2563, 2574, 2600, 2603,
        2630, 2649, 2651, 2730, 2815, 2861, 2872, 2911, 2963, 3012,
        3052, 3067, 3096, 3116, 3149, 3161, 3168, 3200, 3250, 3316,
        3319, 3328, 3374, 3452, 3454, 3462, 3564, 3576, 3587, 3612,
        3644, 3654, 3669, 3670, 3682, 3725, 3747, 3809, 3822, 3839,
        3870, 3875, 3881, 3918, 3919, 3949, 3954, 4003, 4035, 4044,
        4065, 4155, 4212, 4262, 4294, 4343, 4387, 4450, 4489, 4501,
        4540, 4572, 4720, 4724, 4740, 4745, 4834, 4846, 4874, 4926,
        4938, 4941, 4970, 4999, 5019, 5027, 5063, 5170, 5179, 5325,
        5458, 5550, 5555, 5637, 5649, 5702, 5706, 5708, 5779, 5798,
        5802, 5824, 5828, 5831, 5931, 5944, 5974, 5991, 6024, 6039,
        6060, 6147, 6176, 6206, 6256, 6333, 6354, 6356, 6394, 6396,
        6442, 6458, 6582, 6600, 6603, 6639, 6641, 6649, 6684, 6688,
        6709, 6712, 6726, 6734, 6743, 6790, 6810, 6819, 6901, 6924,
        6950, 6989, 7001, 7031, 7048, 7087, 7108, 7146, 7157, 7163,
        7230, 7256, 7268, 7295, 7338, 7432, 7433, 7502, 7512, 7650,
        12037, 12570, 13097, 13656, 14572};

const int32_t ES_IGNORED_CASES[] = {
        195, // roman
        230, // roman
        261, // roman
        387, // musical hashtag
        675, // c#
        703, // roman
        761, // roman
        797, // musical hashtag
        921, // c#
        974, // roman
        1130, // roman
        1240, // roman
        1251, // roman
        1264, // math
        1866, // roman
        1876, // roman
        1943, // roman
        1990, // roman
        2017, // roman
        2075, // math
        2285, // roman
        2319, // c#
        2347, // math
        2388, // invalid char '`'
        2473, // roman
        2488, // math
        2511, // math
        2524, // math
        2657, // math
        2674, // roman
        2684, // math
        2691, // roman
        2894, // context issue
        2950, // math
        3155, // invalid char '`'
        3198, // roman
        3528, // math
        3532, // roman
        3547, // roman
        3634, // roman
        3637, // roman
        3940, // roman
        3950, // roman
        4035, // music
        4084, // too many hashtag
        4162, // invalid char '«'
        4405, // math
        4495, //  Character `“` is invalid.
        4548, // roman
        4555, // roman
        4594, // roman
        4614, // roman
        4682, // roman
        4695, // roman
        4877, // roman
        4929, // roman
        5058, // invalid char '`'
        5255, // roman
        5309, // roman
        5630, // math
        5769, // invalid char '–'
        5843, // roman
        5937, // roman
        6020, // roman
        6099, // roman
        6156, // roman
        7628, // invalid char '“'
        8204, // roman
        8415, // invalid char '–'
        8898, // invalid char '–'
        9498, // roman
        9521, // roman
        9709, // roman
        10020, // roman
        10500, // invalid char '‘’'
        11027, // roman
        11839, // roman
        12271, // roman
        13291, // invalid char '“'
        13432, // roman
        13523, // roman
        14230, // roman
        14371, // roman
        15104, // roman
        15139, // roman
        16914, // invalid char '‘’'
        17282, // roman
};

const int32_t DE_IGNORED_CASES[] = {
    73, // cardinal case
    129, // measurement range
    131, // cardinal range
    144, // cardinal list
    164, // year
    218, // cardinal case
    219, // #
    226, // measurement range
    304, // cardinal list
    325, // cardinal case date
    466, // year
    473, // cardinal list
    493, // cardinal list
    591, // cardinal case date
    639, // cardinal case date
    671, // cardinal case time
    733, // phone number
    782, // year
    825, // cardinal count
    883, // cardinal list
    965, // cardinal list
    966, // year
    1027, // phone number
    1036, // cardinal list
    1137, // cardinal list
    1176, // cardinal case date
    1200, // cardinal case date
    1361, // cardinal case date
    1381, // cardinal case
    1429, // cardinal list
    1463, // cardinal list
    1598, // cardinal case date
    1650, // cardinal range
    1690, // ]
    1700, // cardinal case date
    1710, // ]
    1766, // cardinal list
    1821, // cardinal list
    1850, // cardinal case date
    1893, // cardinal list
    1908, // 5G
    1917, // cardinal case date
    1967, // #
    2017, // [
    2018, // year
    2030, // cardinal list
    2065, // roman numerals
    2108, // cardinal case
    2159, // cardinal case date
    2230, // cardinal case
    2314, // cardinal case date
    2375, // cardinal case date
    2517, // cardinal list
    2538, // cardinal case date
    2562, // year
    2675, // cardinal range
    2718, // phone number
    2737, // cardinal list
    2760, // cardinal case date
    2761, // cardinal case date
    2769, // cardinal case
    2782, // cardinal list
    2792, // cardinal case date
    2804, // cardinal case date
    2876, // cardinal list
    2883, // cardinal list
    2947, // year
    3052, // year
    3109, // cardinal case date
    3120, // 3G
    3138, // year
    3154, // year
    3340, // year
    3440, // cardinal list
    3485, // cardinal case date
    3508, // year
    3552, // year
    3557, // 3G
    3573, // phone number
    3683, // case app strips `-`
    3805, // cardinal range
    3818, // cardinal case
    3901, // cardinal case date
    3924, // cardinal list
    3928, // cardinal case date
    3949, // cardinal list
    4030, // abbreviation gender
    4039, // time range
    4051, // #
    4053, // cardinal count
    4067, // year
    4069, // cardinal case date
    4076, // [
    4077, // cardinal case date
    4107, // time range
    4131, // cardinal list
    4166, // cardinal case date
    4207, // cardinal list
    4245, // year
    4256, // cardinal list
    4296, // cardinal case date
    4310, // case app uppers `eszett`
    4319, // cardinal case date
    4335, // measurement range
    4341, // cardinal list
    4379, // year
    4426, // clean cases failure
    4451, // cardinal case date
    4525, // cardinal case date
    4541, // cardinal case date
    4621, // cardinal case date
    4724, // [
    4729, // cardinal list
    4874, // phone number
    4909, // cardinal case date
    4950, // cardinal case date
    4954, // cardinal case date
    5021, // cardinal list
    5023, // cardinal range
    5081, // cardinal case date
    5125, // cardinal case date
    5174, // #
    5202, // year
    5223, // #
    5303, // phone number
    5338, // cardinal case date
    5448, // cardinal list
    5548, // cardinal case date
    5561, // cardinal list
    5568, // #
    5629, // #
    5648, // cardinal case date
    5665, // phone number
    5717, // cardinal list
    5748, // #
    5780, // cardinal not eos
    5791, // year
    5809, // year
    5952, // cardinal case date
    6003, // cardinal case
    6019, // cardinal case date
    6040, // cardinal list
    6121, // cardinal case date
    6144, // cardinal case date
    6172, // phone number
    6232, // ordinal too many zeros
    6253, // phone number
    6290, // ordinal mit punctuation
    6364, // cardinal not eos
    6371, // cardinal case date
    6387, // measurement range
    6420, // cardinal case date
    6477, // cardinal list
    6496, // year
    6535, // year
    6666, // cardinal list
    6683, // year
    6689, // cardinal case
    6693, // cardinal not eos
    6765, // cardinal case
    6782, // year
    6901, // year
    6961, // phone number
    7006, // cardinal case date
    7074, // cardinal not eos
    7082, // phone number
    7095, // year
    7154, // cardinal list
    7160, // cardinal list
    7188, // year
    7191, // cardinal case date
    7200, // [
    7204, // cardinal case date
    7331, // time range
    7373, // cardinal case date
    7379, // year
    7811, // roman numeral
    8142, // name not month
    8416, // name not month
    9583, // ;
    9590, // name not month
    9664, // non dot abbreviation
    11247, // range date
    11449, // non dot abbreviation
    12742, // ;
    15746, // non dot abbreviation
};

const int32_t FR_IGNORED_CASES[] = {
        810, // platform unicode diffs
        1334, // roman
        2144, // roman
        3128, // platform unicode diffs
        4926, // platform unicode diffs
        4975, // platform unicode diffs
        9221, // roman
        10941, // roman
        11084, // roman
        11911, // roman
        12327, // roman
        14221, // roman
        14771, // spelled out ordinal
        15778, // roman
        16790, // strange no symbol
};

const int32_t IT_IGNORED_CASES[] = {
        48, 118, 194, 232, 249, 279, 285, 372, 394, 396, 464, 483, 496, 523, 577, 590, 646, 674, 735, 761, 783, 861, 938, 1054, 1061, 1097, 1115, 1178, 1349, 1386, 1429, 1430, 1467, 1597, 1781, 1804, 1886, 1962, 2033, 2063, 2067, 2142, 2166, 2183, 2196, 2252, 2263, 2265, 2274, 2393, 2484, 2594, 2611, 2633, 2635, 2660, 2662, 2788, 2847, 2930, 3247, 3284, 3364, 3387, 3445, 3462, 3558, 3594, 3636, 3731, 3777, 3902, 3971, 4037, 4068, 4097, 4201, 4324, 4585, 4590, 4661, 4896, 4929, 5028, 5251, 5485, 5556, 5743, 6008, 6125, 6194, 6220, 6316, 6492, 6641, 6661, 6768, 6784, 6834, 7147, 7302, 7362, 7502, 7632, 7898, 8159, 8240, 8404, 8544, 9027, 9095, 9108, 9192, 9263, 9312, 9321, 9343, 9523, 10012, 10129, 10374, 10565, 10704, 10732, 10873, 10981, 11231, 11412, 11664, 11769, 11856, 11963, 11990, 12312, 12406, 12723, 12997, 13001, 13207, 13227, 13327, 13839, 13858, 14019, 14110, 14113, 14308, 14372, 14533, 14608, 14822, 15111, 15185, 15235, 15320, 15478, 15676, 15680, 16079, 16156, 16203, 16412, 16507, 16556, 16887, 16976, 17038, 17162, 17198, 17264, 17428, 17450};

const int32_t PT_IGNORED_CASES[] = {
        62,
        946,
        1055,
        1120,
        1171,
        1500,
        1603,
        1733,
        1813,
        2067,
        2240,
        2511,
        2558,
        2603,
        2815,
        3493,
        3502,
        3847,
        5156,
        5377,
        5491,
        5585,
        5651,
        5716,
        5816,
        6124,
        7098,
        7480,
        7483,
        7511,
        7672,
        7949,
        8021,
        8291,
        8434,
        8775,
        9607,
        10903,
        11961,
        12309,
        12565,
        12625,
        12816,
        13044,
        13434,
        13714,
        13897,
        14143,
        14330,
        14493,
        14503,
        15575,
        15746,
        16668,
};

const int32_t JA_IGNORED_CASES[] = {
        383, // numeric case not covered in tokenizer dictionary
        452, // item not in tokenizer dictionary
        1143, // unsupported numeric conjugation (*行)
        1429, // LLM verbalization ambiguity
        1506, // unsupported numeric conjugation (*行)
        1641, // unsupported numeric conjugation (*行)
        2334, // unsupported numeric conjugation (*節)
        2840, // unsupported comma cardinal pattern
        2896, // unsupported numeric conjugation (*分)
        3098, // unsupported numeric conjugation (*週)
        3551, // unsupported numeric conjugation (*分)
        3574, // unsupported character (々)
        3793, // unsupported numeric conjugation (*分)
        3954, // unsupported character (々)
        4134, // unsupported comma cardinal pattern
        4261, // unsupported numeric conjugation (*分)
        4377, // unsupported character (々)
        5055, // unsupported character (*章)
        5164, // unsupported character (*品)
        5269, // unsupported character (*人)
        7255, // ambiguous streaming
        7645, // unsupported character (…)
        8203, // ambiguous streaming
        8232, // item not in tokenizer dictionary
        9721, // ambiguous streaming
        10598, // unsupported character (…)
        11280, // unsupported character (※)
        11938, // unsupported comma cardinal pattern
        12798, // unsupported character (＾)
        13209, // unsupported character (^)
        13296, // unsupported character (^)
        13739, // LLM verbalization error
        14987, // unsupported numeric conjugation (*分)
        15239, // LLM verbalization error
        16008, // unsupported character (ﾊｧ)
        16182, // unsupported comma cardinal pattern
        17057, // unsupported character (〔 )
};

struct pv_normalizer_cases_helper {
    uint8_t *buffer;
    int32_t buffer_length;

    int32_t idx;
};

pv_status_t pv_normalizer_cases_helper_init(const char *file_path, pv_normalizer_cases_helper_t **object) {
    PV_ASSERT(file_path);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_cases_helper_t *o = (pv_normalizer_cases_helper_t *) calloc(1, sizeof(pv_normalizer_cases_helper_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    FILE *f = pv_fopen(file_path, "rb");
    if (!f) {
        return PV_STATUS_IO_ERROR;
    }

    fseek(f, 0, SEEK_END);
    o->buffer_length = (int32_t) ftell(f);

    o->buffer = (uint8_t *) malloc(o->buffer_length * sizeof(uint8_t));
    if (!(o->buffer)) {
        (void) fclose(f);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    fseek(f, 0, SEEK_SET);

    const int32_t bytes_read = (int32_t) fread(o->buffer, sizeof(uint8_t), o->buffer_length, f);
    (void) fclose(f);
    if (bytes_read != o->buffer_length) {
        return PV_STATUS_IO_ERROR;
    }

    o->idx = 0;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void pv_normalizer_cases_helper_delete(pv_normalizer_cases_helper_t *object) {
    if (object) {
        free(object->buffer);
        free(object);
    }
}

void pv_normalizer_cases_helper_reindex(pv_normalizer_cases_helper_t *object) {
    PV_ASSERT(object);

    object->idx = 0;
}

static bool read_column_text(pv_normalizer_cases_helper_t *object, char *text, int32_t text_length) {
    PV_ASSERT(object);
    PV_ASSERT(text);

    int ci = 0;
    int32_t idx = 0;
    while (object->idx < object->buffer_length && idx < text_length) {
        const char c = (char) object->buffer[(object->idx)++];
        if (c == '"') {
            ci = object->buffer[(object->idx)++];
            if (object->idx >= object->buffer_length) {
                return false;
            }
            if ((char) ci != '"') {
                (object->idx)--;
                break;
            }
        }
        if (c == '\r') {
            continue;
        }
        text[idx++] = c;
    }

    return true;
}

bool pv_normalizer_cases_helper_next_case(
        pv_normalizer_cases_helper_t *object,
        int32_t *case_no,
        char *text_raw,
        int32_t text_raw_length,
        char *text_batch,
        int32_t text_length_batch,
        char *text_stream,
        int32_t text_length_stream) {
    PV_ASSERT(object);
    PV_ASSERT(case_no);
    PV_ASSERT(text_raw);
    PV_ASSERT(text_batch);
    // `text_stream` can be NULL

    int32_t idx = 0;

    char case_no_buffer[256] = {0};
    while (object->idx < object->buffer_length && idx < PV_ARRAY_LEN(case_no_buffer)) {
        char c = (char) object->buffer[(object->idx)++];
        if (c == ',') {
            break;
        }
        case_no_buffer[idx++] = c;
    }
    *case_no = atoi(case_no_buffer);

    while (object->idx < object->buffer_length) {
        char c = (char) object->buffer[(object->idx)++];
        if (c == '"') {
            break;
        }
    }

    if (!read_column_text(object, text_raw, text_raw_length)) {
        return false;
    }

    while (object->idx < object->buffer_length) {
        char c = (char) object->buffer[(object->idx)++];
        if (c == '"') {
            break;
        }
    }

    if (!read_column_text(object, text_batch, text_length_batch)) {
        return false;
    }

    if (text_stream != NULL) {
        if (object->idx < object->buffer_length) {
            char c = (char) object->buffer[(object->idx)++];
            if (c == ',') {
                c = (char) object->buffer[(object->idx)++];
                if (c == '"') {
                    if (!read_column_text(object, text_stream, text_length_stream)) {
                        return false;
                    }
                }
            } else {
                (object->idx)--;
                strcpy(text_stream, text_batch);
            }
        }
    }

    while (object->idx < object->buffer_length) {
        char c = (char) object->buffer[(object->idx)++];
        if (c == '\n') {
            break;
        } else if (c == '\r') {
            int next_char = object->buffer[(object->idx)++];
            if (next_char != '\n') {
                (object->idx)--;
            }
            break;
        }
    }

    return object->idx < object->buffer_length;
}

pv_status_t pv_normalizer_cases_normalize_batch(
        pv_normalizer_t *normalizer_object,
        const char *input,
        char **output) {
    PV_ASSERT(normalizer_object);
    PV_ASSERT(input);
    PV_ASSERT(output);

    return pv_normalizer_normalize(
            normalizer_object,
            input,
            true,
            false,
            output,
            NULL);
}

pv_status_t pv_normalizer_cases_normalize_stream(
        pv_picollm_tokenizer_t *stream_tokenizer,
        pv_normalizer_t *normalizer_object,
        bool add_spaces,
        const char *input,
        char **output) {
    PV_ASSERT(stream_tokenizer);
    PV_ASSERT(normalizer_object);
    PV_ASSERT(input);
    PV_ASSERT(output);

    int32_t num_tokens = 0;
    int32_t *tokens = NULL;

    pv_status_t status = pv_picollm_tokenizer_encode(
            stream_tokenizer,
            input,
            false,
            false,
            &num_tokens,
            &tokens);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_normalizer_stream_t *normalizer_stream = NULL;
    status = pv_normalizer_stream_open(normalizer_object, &normalizer_stream);
    if (status != PV_STATUS_SUCCESS) {
        free(tokens);
        return status;
    }

    char output_buffer[4096] = {0};
    int32_t num_partial = 0;

    for (int32_t i = 0; i < num_tokens; i++) {
        char *decoded_token = NULL;
        bool is_partial = false;

        status = pv_picollm_tokenizer_decode(
                stream_tokenizer,
                tokens + i - num_partial,
                1 + num_partial,
                0,
                &is_partial,
                &decoded_token);
        if (status != PV_STATUS_SUCCESS) {
            free(tokens);
            pv_normalizer_stream_close(normalizer_stream);
            return status;
        }

        if (is_partial) {
            num_partial += 1;
            free(decoded_token);
            continue;
        } else {
            num_partial = 0;
        }

        pv_normalizer_token_list_t *token_list = NULL;
        status = pv_normalizer_stream_add(
                normalizer_stream,
                decoded_token,
                &token_list);
        free(decoded_token);
        if (status != PV_STATUS_SUCCESS) {
            free(tokens);
            pv_normalizer_stream_close(normalizer_stream);
            return status;
        }

        if ((token_list != NULL) && (token_list->size > 0)) {
            pv_normalizer_token_t *current = token_list->head;
            while ((current != NULL)) {
                if (current->verbalized && strcmp(current->verbalized, " ") != 0) {
                    strcat(output_buffer, current->verbalized);
                    if (add_spaces) {
                        strcat(output_buffer, " ");
                    }
                }
                current = current->next;
            }
        }
        pv_normalizer_token_list_delete(token_list);
    }

    free(tokens);

    pv_normalizer_token_list_t *token_list = NULL;
    status = pv_normalizer_stream_flush(
            normalizer_stream,
            &token_list);
    if (status != PV_STATUS_SUCCESS) {
        pv_normalizer_stream_close(normalizer_stream);
        return status;
    }

    if ((token_list != NULL) && (token_list->size > 0)) {
        pv_normalizer_token_t *current = token_list->head;
        while ((current != NULL)) {
            if (current->verbalized && strcmp(current->verbalized, " ") != 0) {
                strcat(output_buffer, current->verbalized);
                if (add_spaces) {
                    strcat(output_buffer, " ");
                }
            }
            current = current->next;
        }
    }
    pv_normalizer_token_list_delete(token_list);

    pv_normalizer_stream_close(normalizer_stream);

    if (add_spaces) {
        // trim last space
        output_buffer[strlen(output_buffer) - 1] = '\0';
    }

    *output = calloc(strlen(output_buffer) + 1, sizeof(char));
    strcpy(*output, output_buffer);

    return PV_STATUS_SUCCESS;
}

static bool is_ignored(int32_t case_number, const int32_t ignored_cases[], const int32_t length) {
    PV_ASSERT(case_number >= 0);
    PV_ASSERT(length >= 0);

    int32_t left = 0;
    int32_t right = length - 1;
    while (left <= right) {
        int32_t mid = left + (right - left) / 2;
        if (ignored_cases[mid] == case_number) {
            return true;
        }
        if (ignored_cases[mid] < case_number) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return false;
}

bool pv_normalizer_cases_is_ignored(int32_t case_number, const char *language_code) {
    PV_ASSERT(case_number >= 0);
    PV_ASSERT(language_code);

    if (strcmp(language_code, "en") == 0) {
        return is_ignored(case_number, EN_IGNORED_CASES, PV_ARRAY_LEN(EN_IGNORED_CASES));
    } else if (strcmp(language_code, "de") == 0) {
        return is_ignored(case_number, DE_IGNORED_CASES, PV_ARRAY_LEN(DE_IGNORED_CASES));
    } else if (strcmp(language_code, "es") == 0) {
        return is_ignored(case_number, ES_IGNORED_CASES, PV_ARRAY_LEN(ES_IGNORED_CASES));
    } else if (strcmp(language_code, "fr") == 0) {
        return is_ignored(case_number, FR_IGNORED_CASES, PV_ARRAY_LEN(FR_IGNORED_CASES));
    } else if (strcmp(language_code, "it") == 0) {
        return is_ignored(case_number, IT_IGNORED_CASES, PV_ARRAY_LEN(IT_IGNORED_CASES));
    } else if (strcmp(language_code, "ja") == 0) {
        return is_ignored(case_number, JA_IGNORED_CASES, PV_ARRAY_LEN(JA_IGNORED_CASES));
    } else if (strcmp(language_code, "pt") == 0) {
        return is_ignored(case_number, PT_IGNORED_CASES, PV_ARRAY_LEN(PT_IGNORED_CASES));
    }
    return false;
}
