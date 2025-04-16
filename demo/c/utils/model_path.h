#ifndef MODELS_H
#define MODELS_H

char **get_available_languages(int *languages_count);
char **get_available_genders(int *genders_count);
char *get_model_path(const char *language, const char *gender);

#endif
