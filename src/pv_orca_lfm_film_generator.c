#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "math/pv_math.h"
#include "orca/pv_cnn.h"
#include "orca/pv_orca_lfm_film_generator.h"
#include "orca/pv_rope_transformer.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

struct pv_orca_lfm_film_generator {
    const pv_orca_lfm_film_generator_param_t *param;

    pv_rope_transformer_t **transformers;
};

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_lfm_film_generator_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_lfm_film_generator_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t count = fwrite(&(param->num_blocks), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->dimension), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->sdpa_downsample_factor), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    for (int32_t i = 0; i < param->num_blocks; i++) {
        pv_status_t status = pv_rope_transformer_param_serialize(
                ypu,
                param->transformers_param[i],
                file);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_rope_transformer_param_serialize,
                    pv_status_to_string(status));
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_lfm_film_generator_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_lfm_film_generator_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_lfm_film_generator_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_lfm_film_generator_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_orca_lfm_film_generator_param_t));

    size_t count = pv_fread(&(p->num_blocks), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_orca_lfm_film_generator_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_blocks <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->num_blocks"));
        pv_orca_lfm_film_generator_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->dimension), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_orca_lfm_film_generator_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->dimension <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->dimension"));
        pv_orca_lfm_film_generator_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->sdpa_downsample_factor), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_orca_lfm_film_generator_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->sdpa_downsample_factor <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->sdpa_downsample_factor"));
        pv_orca_lfm_film_generator_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->transformers_param = pv_ypu_host_alloc(
            ypu,
            p->num_blocks * ((int32_t) sizeof(pv_rope_transformer_param_t *)));
    if (!(p->transformers_param)) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->transformers_param"));
        pv_orca_lfm_film_generator_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p->transformers_param, 0, p->num_blocks * sizeof(pv_rope_transformer_param_t *));

    for (int32_t i = 0; i < p->num_blocks; i++) {
        pv_status_t status = pv_rope_transformer_param_load(
                ypu,
                f,
                (pv_rope_transformer_param_t **) &(p->transformers_param[i]));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_rope_transformer_param_load,
                    pv_status_to_string(status));
            pv_orca_lfm_film_generator_param_delete(ypu, p);
            return status;
        }
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_lfm_film_generator_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_lfm_film_generator_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        if (param->transformers_param) {
            for (int32_t i = param->num_blocks - 1; i >= 0; --i) {
                pv_rope_transformer_param_delete(ypu, (pv_rope_transformer_param_t *) (param->transformers_param[i]));
            }

            pv_ypu_host_free(ypu, param->transformers_param);
        }

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_orca_lfm_film_generator_param_is_equal)(
        const pv_orca_lfm_film_generator_param_t *object,
        const pv_orca_lfm_film_generator_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->num_blocks != other->num_blocks) {
        return false;
    }

    if (object->dimension != other->dimension) {
        return false;
    }

    if (object->sdpa_downsample_factor != other->sdpa_downsample_factor) {
        return false;
    }

    for (int32_t i = 0; i < object->num_blocks; i++) {
        if (!pv_rope_transformer_param_is_equal(object->transformers_param[i], other->transformers_param[i])) {
            return false;
        }
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_orca_lfm_film_generator_init)(
        pv_ypu_t *ypu,
        const pv_orca_lfm_film_generator_param_t *param,
        pv_orca_lfm_film_generator_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_lfm_film_generator_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_lfm_film_generator_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_lfm_film_generator_t));

    o->param = param;

    o->transformers = pv_ypu_host_alloc(
            ypu,
            param->num_blocks * ((int32_t) sizeof(pv_rope_transformer_t *)));
    if (!(o->transformers)) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->transformers"));
        pv_orca_lfm_film_generator_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o->transformers, 0, param->num_blocks * sizeof(pv_rope_transformer_t *));

    for (int32_t i = 0; i < param->num_blocks; i++) {
        pv_status_t status = pv_rope_transformer_init(
                ypu,
                param->transformers_param[i],
                &(o->transformers[i]));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_rope_transformer_init,
                    pv_status_to_string(status));
            pv_orca_lfm_film_generator_delete(ypu, o);
            return status;
        }
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_lfm_film_generator_delete)(
        pv_ypu_t *ypu,
        pv_orca_lfm_film_generator_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->transformers) {
            for (int32_t i = object->param->num_blocks - 1; i >= 0; --i) {
                pv_rope_transformer_delete(ypu, object->transformers[i]);
            }
            pv_ypu_host_free(ypu, object->transformers);
        }

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_lfm_film_generator_forward)(
        pv_ypu_t *ypu,
        pv_orca_lfm_film_generator_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *bucket,
        pv_ypu_mem_t *y) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(x);
    PV_ASSERT(bucket);
    PV_ASSERT(y);

    const int32_t num_blocks = object->param->num_blocks;
    for (int32_t i = 0; i < num_blocks; i++) {
        pv_status_t status = pv_rope_transformer_forward(
                ypu,
                object->transformers[i],
                n,
                i == 0 ? x : y,
                bucket,
                y);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_rope_transformer_forward,
                    pv_status_to_string(status));
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}
