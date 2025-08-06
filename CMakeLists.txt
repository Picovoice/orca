cmake_minimum_required(VERSION 3.21)

if (NOT PV_PEER_MODULE_DIR)
  set(PV_PEER_MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif ()

include(peer-module.cmake)
include(peer/zoo-dev.cmake)

include ("${zoo-dev_SOURCE_DIR}/preload.cmake")

project(orca LANGUAGES C CXX)

include("${zoo-dev_SOURCE_DIR}/config.cmake")
pv_util_func_hash_gen_add_dir(MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(${zoo-dev_SOURCE_DIR} ${zoo-dev_BINARY_DIR} EXCLUDE_FROM_ALL)

set(MODULE_VERSION 1.2.X)

message(CHECK_START "Generating clean public header file for ${MODULE}")
execute_process(
  COMMAND ${PV_PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/script/clean_public_header.py
  -o ${PROJECT_BINARY_DIR}/include
  -p ${zoo-dev_BINARY_DIR}/include/picovoice.h)
message(CHECK_PASS "done")

include(cmake/speakers.cmake)
include(cmake/languages.cmake)
include(normalizer.cmake)

set(pv_orca_additional_pv_deps "")
if (NOT ${PV_TARGET_PLATFORM} STREQUAL "wasm")
  list(APPEND pv_orca_additional_pv_deps pv_audio_private)
endif ()

pv_module_register(
  MODULE_NAME pv_orca
  INCLUDE_DIRECTORIES include
  HEADERS
    include/orca/pv_orca.h
    include/orca/pv_orca_internal.h
    include/orca/pv_orca_synthesizer.h
    include/orca/pv_orca_text_encoder.h
    include/orca/pv_orca_duration_predictor.h
    include/orca/pv_orca_istft.h
    include/orca/pv_orca_fft.h
    include/orca/pv_orca_flow.h
    include/orca/pv_orca_metric_internal.h
    include/orca/pv_orca_phonemizer.h
    include/orca/pv_orca_pqmf.h
    include/orca/pv_orca_stream_state.h
    include/orca/pv_orca_util.h
    include/orca/pv_orca_vocoder.h
    include/orca/pv_affine.h
    include/orca/pv_cnn.h
    include/orca/pv_cnn_matmul.h
    include/orca/pv_cnn_transposed.h
    include/orca/pv_attention.h
    include/orca/pv_buffer.h
    include/orca/pv_buffer_int32.h
    include/orca/pv_buffer_q510.h
    include/orca/pv_embed.h
    include/orca/pv_convnext.h
    include/orca/pv_convnext_transposed.h
    include/orca/pv_layer_norm.h
    include/orca/pv_residual_coupling.h
    include/orca/pv_transformer.h
    include/orca/pv_transformer_ffn.h
    include/orca/pv_vocos_backbone.h
    include/orca/pv_wavenet_resblock.h
    include/orca/pv_profiler.h
    include/orca/normalizer/pv_normalizer_language_data.h
  SOURCES
    src/pv_orca.c
    src/pv_orca_synthesizer.c
    src/pv_orca_text_encoder.c
    src/pv_orca_duration_predictor.c
    src/pv_orca_istft.c
    src/pv_orca_fft.c
    src/pv_orca_flow.c
    src/pv_orca_phonemizer.c
    src/pv_orca_pqmf.c
    src/pv_orca_stream_state.c
    src/pv_orca_util.c
    src/pv_orca_vocoder.c
    src/pv_affine.c
    src/pv_attention.c
    src/pv_buffer.c
    src/pv_buffer_int32.c
    src/pv_buffer_q510.c
    src/pv_cnn.c
    src/pv_cnn_matmul.c
    src/pv_cnn_transposed.c
    src/pv_embed.c
    src/pv_convnext.c
    src/pv_convnext_transposed.c
    src/pv_layer_norm.c
    src/pv_residual_coupling.c
    src/pv_transformer_ffn.c
    src/pv_transformer.c
    src/pv_profiler.c
    src/pv_vocos_backbone.c
    src/pv_wavenet_resblock.c
    test/pv_orca_metric.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_word_character_data_de.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_word_character_data_en.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_word_character_data_es.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_word_character_data_fr.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_word_character_data_it.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_word_character_data_ja.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_word_character_data_ko.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_word_character_data_pt.c
  PV_DEPENDENCIES
    pv_core
    pv_dump
    pv_hippo_private
    pv_io
    pv_lm
    pv_model_online
    pv_normalizer
    pv_file
    pv_resampler_private
    pv_util
    ${pv_orca_additional_pv_deps}
  PV_PRIVATE_DEPENDENCIES
    pv_feature
    pv_language_json
    pv_math
  GATEKEEPER pv_gatekeeper_usage_animal
)

pv_module_build_static(MODULE_NAME pv_orca)
pv_module_build_shared(MODULE_NAME pv_orca)

pv_module_build_shared_jni(MODULE_NAME pv_orca JNI_SOURCES platform/android/src/pv_orca_jni.c)
pv_module_build_shared_android(MODULE_NAME pv_orca JNI_SOURCES platform/android/src/pv_orca_jni.c)
pv_module_build_shared_ios(
  MODULE_NAME pv_orca
  IDENTIFIER PvOrca
  BUNDLE_VERSION ${MODULE_VERSION}
  IOS_PUBLIC_HEADERS
    ${PROJECT_BINARY_DIR}/include/pv_orca.h
    ${PROJECT_BINARY_DIR}/include/picovoice.h
    platform/ios/src/PvOrca.h
  IOS_MODULE_MAP platform/ios/src/module.modulemap
)

pv_module_build_static_wasm(MODULE_NAME pv_orca RELATIVE_PARAM_PATH "orca/param")

pv_module_build_app(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_app
  SOURCES app/pv_orca_app.c
  PV_DEPENDENCIES pv_audio
)

pv_module_build_app(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_input_output_streaming_app
  SOURCES app/pv_orca_input_output_streaming_app.c
  PV_DEPENDENCIES pv_audio
)

# TODO: Below use a loop for all languages:
# ENGLISH
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_en.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_en.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param .c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_en}
)

pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  APP_NAME_SUFFIX _16
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_en.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_en.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param _16.c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_en}
)

# GERMAN
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_de.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_de.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param .c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_de}
)

pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  APP_NAME_SUFFIX _16
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_de.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_de.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param _16.c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_de}
)

# FRENCH
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_fr.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_fr.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param .c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_fr}
)

pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  APP_NAME_SUFFIX _16
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_fr.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_fr.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param _16.c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_fr}
)

# SPANISH
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_es.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_es.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param .c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_es}
)

pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  APP_NAME_SUFFIX _16
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_es.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_es.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param _16.c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_es}
)

# ITALIAN
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_it.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_it.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param .c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_it}
)

pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  APP_NAME_SUFFIX _16
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_it.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_it.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param _16.c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_it}
)

# PORTUGUESE
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_pt.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_pt.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param .c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_pt}
)

pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  APP_NAME_SUFFIX _16
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_pt.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_pt.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param _16.c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_pt}
)

# KOREAN
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_ko.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_ko.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param .c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_ko}
)

# JAPANESE
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${zoo-dev_SOURCE_DIR}/src/hippo/src/pv_classifier_ja.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_ja.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "src/model/pv_orca_synthesizer_param .c"
    "src/dict/pv_dict .c"
    "src/dict/pv_lexicon .c"
    "src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_ja}
)

pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_metric_serializer
  SOURCES
    app/pv_orca_metric_serializer.c
    test/pv_orca_metric.c
  ADDITIONAL_SOURCES_TEMPLATES
    "test/model/pv_orca_metric_classifier .c"
  SOURCE_IDENTIFIER_LIST ${orca_language_code_list}
)

pv_module_build_app(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_synthesizer_bench
  SOURCES app/pv_orca_synthesizer_bench.c
)

pv_module_register_tests(
  MODULE_NAME pv_orca
  UNIT_TEST_SOURCES
    test/test_pv_affine.c
    test/test_pv_attention.c
    test/test_pv_cnn.c
    test/test_pv_convnext.c
    test/test_pv_layer_norm.c
    test/test_pv_embed.c
    test/test_pv_orca.c
    test/test_pv_orca_dictionary.c
    test/test_pv_orca_duration_predictor.c
    test/test_pv_orca_istft.c
    test/test_pv_orca_ja.c
    test/test_pv_orca_text_encoder.c
    test/test_pv_orca_flow.c
    test/test_pv_orca_integration.c
    test/test_pv_orca_multilingual.c
    test/test_pv_orca_metric.c
    test/test_pv_orca_synthesizer.c
    test/test_pv_orca_phonemizer.c
    test/test_pv_orca_util.c
    test/test_pv_orca_vocoder.c
    test/test_pv_residual_coupling.c
    test/test_pv_transformer_ffn.c
    test/test_pv_vocos_backbone.c
    test/pv_orca_metric.c
    src/pv_orca.c
    src/model/pv_orca_synthesizer_param_en_female.c
  PV_DEPENDENCIES
    pv_tokenizer
  TEST_RESOURCES
    ${zoo-dev_SOURCE_DIR}/res/ref/shut_off_lights.c
)

pv_module_build_tests(MODULE_NAME pv_orca)
pv_module_build_test_app(
  MODULE_NAME pv_orca
  SUB_MODULES pv_normalizer
  PV_DEPENDENCIES pv_picollm)

set(ORCA_TEST_RESOURCES "${PROJECT_SOURCE_DIR}/res")
pv_module_copy_test_resource_dirs(
  MODULE_NAME pv_orca
  TEST_RESOURCE_DIRS ${ORCA_TEST_RESOURCES}
)

option(ORCA_FLOAT_MODE "Use floats precision for some ORCA algorithms" OFF)
option(PV_PROFILING_MODE "enable profiling of code" OFF)

option(ORCA_LOG_LEVEL_VERBOSE "enable verbose printouts specific to Orca" OFF)
option(ORCA_PHONEMIZER_LOG_DEBUG "enable debug printouts specific to Orca phonemizer" OFF)

if(ORCA_FLOAT_MODE)
  add_definitions(-D__ORCA_FLOAT_MODE__)
endif()

if(PV_PROFILING_MODE)
  add_definitions(-D__PV_PROFILING_MODE__)
endif()

if(ORCA_PHONEMIZER_LOG_DEBUG)
  add_definitions(-D__ORCA_PHONEMIZER_LOG_DEBUG__)
endif()

if(ORCA_LOG_LEVEL_VERBOSE)
  add_definitions(-D__ORCA_LOG_LEVEL_VERBOSE__)
endif()

if (PV_BUILD_NODE_LIBS)
  add_subdirectory(platform/node)
endif ()
