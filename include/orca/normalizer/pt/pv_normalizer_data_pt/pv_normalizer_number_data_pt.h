#ifndef PV_NORMALIZER_NUMBER_DATA_PT_H
#define PV_NORMALIZER_NUMBER_DATA_PT_H

#include <stdint.h>
#include <stdbool.h>

#define PV_NORMALIZER_NUM_DENOMINATOR_ORDINALS_PT (22)
#define PV_NORMALIZER_NUM_VALID_SUPERSCRIPT_PT (2)

extern const int64_t MAX_VERBALIZED_CARDINAL_PT;
extern const int64_t MAX_MULTIPLIER_PT;

extern const char *NUMBER_RANGE_INBETWEEN_STRING_PT;

extern const char *const ONE_TWO_MASCULINE_PT[3];
extern const char *const ONE_TWO_FEMININE_PT[3];
extern const char *const ZERO_TO_NINETEEN_NO_GENDER_PT[20];

extern const char *const TENS_PT[9];

extern const char *const ONE_HUNDREDS_PT[2];
extern const char *const HUNDREDS_TWO_TO_NINE_MASCULINE_PT[10];
extern const char *const HUNDREDS_TWO_TO_NINE_FEMININE_PT[10];

extern const char *const MULTIPLIERS_SINGULAR_PT[5];
extern const char *const MULTIPLIERS_PLURAL_PT[5];

extern const char *DENOMINATOR_STRING_PT;
extern const char *const DENOMINATOR_ORDINAL_KEYS_PT[PV_NORMALIZER_NUM_DENOMINATOR_ORDINALS_PT];
extern const char *const DENOMINATOR_ORDINAL_VALUES_SINGULAR_PT[PV_NORMALIZER_NUM_DENOMINATOR_ORDINALS_PT];
extern const char *const DENOMINATOR_ORDINAL_VALUES_PLURAL_PT[PV_NORMALIZER_NUM_DENOMINATOR_ORDINALS_PT];

extern const char *const ORDINAL_VALID_SUPERSCRIPT_PT[PV_NORMALIZER_NUM_VALID_SUPERSCRIPT_PT];
extern const bool ORDINAL_SUPERSCRIPT_IS_FEMININE_PT[PV_NORMALIZER_NUM_VALID_SUPERSCRIPT_PT];

extern const char *const ORDINAL_ZERO_TO_NINE_MASCULINE_PT[10];
extern const char *const ORDINAL_ZERO_TO_NINE_FEMININE_PT[10];

extern const char *const ORDINAL_TENS_MASCULINE_PT[10];
extern const char *const ORDINAL_TENS_FEMININE_PT[10];

extern const char *const ORDINAL_HUNDREDS_MASCULINE_PT[10];
extern const char *const ORDINAL_HUNDREDS_FEMININE_PT[10];

extern const char *const ORDINAL_MULTIPLIERS_MASCULINE_PT[5];
extern const char *const ORDINAL_MULTIPLIERS_FEMININE_PT[5];

#endif // PV_NORMALIZER_NUMBER_DATA_PT_H
