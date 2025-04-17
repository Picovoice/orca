#include "model_path.h"
#include <dirent.h>
#include <libgen.h>
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
#define MODEL_FILE_PREFIX "orca_params_"

static void get_models_dirpath(char *models_dirpath, size_t size) {
    char file_path[MAX_PATH];
    strncpy(file_path, __FILE__, MAX_PATH);
    file_path[MAX_PATH - 1] = '\0';
    char *current_dir = dirname(file_path);
    snprintf(models_dirpath, size, "%s/../../../lib/common", current_dir);
}

static bool is_language_exists(int count, char** languages, const char *language) {
    for (int i = 0; i < count; i++) {
        if (strcmp(languages[i], language) == 0) {
            return true;
        }
    }
    return false;
}

char** get_available_languages(int* languages_count) {
    char** languages = malloc(MAX_NUM_LANGUAGES * sizeof(char*));
    if (!languages) {
        fprintf(stderr, "Failed to get available languages (out of memory).\n");
        exit(EXIT_FAILURE);
    }

    char models_dirpath[MAX_PATH];
    get_models_dirpath(models_dirpath, sizeof(models_dirpath));
    DIR* models_dir = opendir(models_dirpath);
    if (!models_dir) {
        fprintf(stderr, "Failed to get available languages (unable to open models directory).\n");
        free(languages);
        exit(EXIT_FAILURE);
    }

    struct dirent* entry;
    int count = 0;

    while ((entry = readdir(models_dir))) {
        if (strncmp(entry->d_name, MODEL_FILE_PREFIX, strlen(MODEL_FILE_PREFIX)) != 0) {
            continue;
        }

        const char *lang_code_ptr = entry->d_name + strlen(MODEL_FILE_PREFIX);
        char lang_code[LANGUAGE_CODE_LEN + 1];
        strncpy(lang_code, lang_code_ptr, LANGUAGE_CODE_LEN);
        lang_code[LANGUAGE_CODE_LEN] = '\0';

        if (!is_language_exists(count, languages, lang_code)) {
            if (count < MAX_NUM_LANGUAGES) {
                languages[count++] = strdup(lang_code);
            } else {
                fprintf(stderr, "Failed to get available languages (too many languages).\n");
                free(languages);
                exit(EXIT_FAILURE);
            }
        }
    }

    closedir(models_dir);

    *languages_count = count;
    return languages;
}

char **get_available_genders(int *genders_count) {
    *genders_count = NUM_GENDERS;
    char **genders = malloc(sizeof(char *) * (*genders_count));
    if (!genders) {
        fprintf(stderr, "Failed to get available genders (out of memory).\n");
        exit(EXIT_FAILURE);
    }
    genders[0] = strdup("male");
    genders[1] = strdup("female");
    return genders;
}

char *get_model_path(const char *language, const char *gender) {
    char model_name[MAX_MODEL_NAME];
    snprintf(model_name, sizeof(model_name), "orca_params_%s_%s.pv", language, gender);

    char models_dirpath[MAX_PATH];
    get_models_dirpath(models_dirpath, sizeof(models_dirpath));

    char model_path[MAX_PATH + MAX_MODEL_NAME];
    snprintf(model_path, sizeof(model_path), "%s/%s", models_dirpath, model_name);

    if (access(model_path, F_OK) == 0) {
        return strdup(model_path);
    }

    DIR *models_dir = opendir(models_dirpath);
    if (!models_dir) {
        fprintf(stderr, "Failed to get model path (unable to open models directory).\n");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    char available_gender[sizeof("female")] = {0};

    while ((entry = readdir(models_dir)) != NULL) {
        if (strncmp(entry->d_name, MODEL_FILE_PREFIX, strlen(MODEL_FILE_PREFIX)) == 0) {
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
    closedir(models_dir);

    if (available_gender[0] != '\0') {
        fprintf(
                stderr,
                "Gender '%s' is not available with language '%s'. Please use gender '%s'.\n",
                gender, language, available_gender);
    } else {
        fprintf(
                stderr,
                "Gender '%s' is not available with language '%s'.\n",
                gender, language);
    }

    exit(EXIT_FAILURE);
}
