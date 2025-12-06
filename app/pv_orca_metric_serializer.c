#include <stdlib.h>

#include "io/pv_log.h"
#include "model/pv_online_token_classifier.h"
#include "orca/pv_orca_metric_internal.h"
#include "util/pv_file.h"

extern const pv_online_token_classifier_param_t *PV_ORCA_METRIC_CLASSIFIER(void);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr,"usage: %s language_info_path param_path\n", argv[0]);
        return 1;
    }

    const char *language_info_path = argv[1];
    const char *param_path = argv[2];

    pv_ypu_t *ypu = NULL;
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("ypu initialization failed with '%s'", pv_status_to_string(status));
        exit(EXIT_FAILURE);
    }

    status = pv_orca_metric_classifier_param_serialize(
            ypu,
            PV_ORCA_METRIC_CLASSIFIER(),
            language_info_path,
            param_path);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("failed with `%s`", pv_status_to_string(status));
        exit(EXIT_FAILURE);
    }

    pv_ypu_delete(ypu);

    return 0;
}
