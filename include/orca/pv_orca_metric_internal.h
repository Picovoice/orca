#ifndef PV_ORCA_METRIC_H
#define PV_ORCA_METRIC_H

#include "core/picovoice.h"
#include "model/pv_online_token_classifier.h"

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_metric_classifier_param_serialize)(
        pv_ypu_t *ypu,
        const pv_online_token_classifier_param_t *param,
        const char *language_info_path,
        const char *param_path);

#endif

typedef struct pv_orca_metric pv_orca_metric_t;

pv_status_t PV_MOCKABLE(pv_orca_metric_init)(
        pv_ypu_t *ypu,
        const char *model_path,
        int32_t sample_rate,
        pv_orca_metric_t **object);

void PV_MOCKABLE(pv_orca_metric_delete)(pv_ypu_t *ypu, pv_orca_metric_t *object);

pv_status_t PV_MOCKABLE(pv_orca_metric_get_posterior_frame)(
        pv_ypu_t *ypu,
        pv_orca_metric_t *object,
        const int16_t *pcm,
        q31_t *posterior);

pv_status_t PV_MOCKABLE(pv_orca_metric_process)(
        pv_ypu_t *ypu,
        pv_orca_metric_t *object,
        int32_t pcm_length,
        const int16_t *pcm,
        int32_t num_target_char_labels,
        const char **target_char_labels,
        float *per);

pv_status_t PV_MOCKABLE(pv_orca_metric_pcm_frame_level_error_evaluation)(
        pv_ypu_t *ypu,
        pv_orca_metric_t *object,
        int32_t pcm_length,
        const int16_t *pcm,
        const char *truth_phoneme_sequence,
        float threshold_worst_case,
        float threshold_average_case,
        bool *passed);

#endif // PV_ORCA_METRIC_H
