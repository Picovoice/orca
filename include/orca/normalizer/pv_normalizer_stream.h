#ifndef PV_NORMALIZER_STREAM_H
#define PV_NORMALIZER_STREAM_H

#include "core/picovoice.h"
#include "orca/normalizer/pv_normalizer.h"

typedef struct pv_normalizer_stream pv_normalizer_stream_t;

pv_status_t PV_MOCKABLE(pv_normalizer_stream_open)(const pv_normalizer_t *normalizer, pv_normalizer_stream_t **object);

void PV_MOCKABLE(pv_normalizer_stream_close)(pv_normalizer_stream_t *object);

void PV_MOCKABLE(pv_normalizer_stream_reset)(pv_normalizer_stream_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_stream_add_to_input_buffer)(
        pv_normalizer_stream_t *object,
        const char *text,
        pv_normalizer_token_list_t **verbalizable_token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_stream_add)(
        pv_normalizer_stream_t *object,
        const char *text,
        pv_normalizer_token_list_t **phonemizable_token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_stream_flush_verbalizable_tokens)(
        pv_normalizer_stream_t *object,
        pv_normalizer_token_list_t **verbalizable_token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_stream_flush)(
        pv_normalizer_stream_t *object,
        pv_normalizer_token_list_t **phonemizable_token_list);

#endif // PV_NORMALIZER_STREAM_H
