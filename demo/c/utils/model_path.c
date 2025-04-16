#include "model_path.h"
#include <dirent.h>
#include <libgen.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH 512
#define MAX_MODEL_NAME 128
#define MAX_NUM_LANGUAGES 100
#define LANGUAGE_CODE_LEN 2
#define NUM_GENDERS 2

static void get_abs_models_dir(char *models_dir, size_t size) {
    char file_path[MAX_PATH];
    strncpy(file_path, __FILE__, MAX_PATH);
    file_path[MAX_PATH - 1] = '\0';
    char *current_dir = dirname(file_path);
    snprintf(models_dir, size, "%s/../../../lib/common", current_dir);
}

char** get_available_languages(int* languages_count) {
    char** languages = malloc(MAX_NUM_LANGUAGES * sizeof(char*));
    if (!languages) {
        fprintf(stderr, "Failed to get available languages.\n");
        exit(EXIT_FAILURE);
    }

    regex_t regex;
    if (regcomp(&regex, "^orca_params_([a-z]{2})_(male|female)\\.pv$", REG_EXTENDED)) {
        fprintf(stderr, "Failed to get available languages.\n");
        free(languages);
        exit(EXIT_FAILURE);
    }

    char models_dir[MAX_PATH];
    get_abs_models_dir(models_dir, sizeof(models_dir));
    DIR* dir = opendir(models_dir);
    if (!dir) {
        fprintf(stderr, "Failed to get available languages.\n");
        regfree(&regex);
        free(languages);
        exit(EXIT_FAILURE);
    }

    struct dirent* entry;
    regmatch_t matches[3]; // whole match, language, gender
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (regexec(&regex, entry->d_name, 3, matches, 0) == 0) {
            int len = matches[1].rm_eo - matches[1].rm_so;
            char language[LANGUAGE_CODE_LEN + 1] = {0};
            strncpy(language, entry->d_name + matches[1].rm_so, len);
            language[len] = '\0';

            bool is_already_exists = false;
            for (int i = 0; i < count; i++) {
                if (strcmp(languages[i], language) == 0) {
                    is_already_exists = true;
                    break;
                }
            }

            if (is_already_exists == false && count < MAX_NUM_LANGUAGES) {
                languages[count] = strdup(language);
                if (!languages[count]) {
                    fprintf(stderr, "Failed to get available languages.\n");
                    exit(EXIT_FAILURE);
                }
                count++;
            }
        }
    }

    closedir(dir);
    regfree(&regex);

    *languages_count = count;
    return languages;
}

char **get_available_genders(int *genders_count) {
    *genders_count = NUM_GENDERS;
    char **genders = malloc(sizeof(char *) * (*genders_count));
    if (!genders) {
        fprintf(stderr, "Failed to get available genders.\n");
        exit(EXIT_FAILURE);
    }
    genders[0] = strdup("male");
    genders[1] = strdup("female");
    return genders;
}

char *get_model_path(const char *language, const char *gender) {
    char model_name[MAX_MODEL_NAME];
    snprintf(model_name, sizeof(model_name), "orca_params_%s_%s.pv", language, gender);

    char models_dir[MAX_PATH];
    get_abs_models_dir(models_dir, sizeof(models_dir));

    char model_path[MAX_PATH + MAX_MODEL_NAME];
    snprintf(model_path, MAX_PATH + MAX_MODEL_NAME, "%s/%s", models_dir, model_name);

    if (access(model_path, F_OK) == 0) {
        return strdup(model_path);
    }

    DIR *dir = opendir(models_dir);
    if (!dir) {
        fprintf(stderr, "Failed to get model path.\n");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    char available_gender[8] = {0};

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "orca_params_", 12) == 0) {
            char *dot = strrchr(entry->d_name, '.');
            if (dot) {
                *dot = '\0';
                char *last_underscore = strrchr(entry->d_name, '_');
                if (last_underscore && strstr(entry->d_name, language)) {
                    strncpy(available_gender, last_underscore + 1, sizeof(available_gender) - 1);
                    break;
                }
            }
        }
    }
    closedir(dir);

    fprintf(stderr,
            "Gender '%s' is not available with language '%s'. Please use gender '%s'.\n",
            gender, language, available_gender);

    return NULL;
}
