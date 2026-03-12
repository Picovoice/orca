cmake_minimum_required(VERSION 3.21)

if (NOT PV_PEER_MODULE_DIR)
  set(PV_PEER_MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif ()

set(PV_MODULE_SKIP_SETUP_REPO "on")
include(setup.cmake)

set(PV_YPU_DIRECTX_SUPPORT "OFF" CACHE BOOL "Enable support for YPU DirectX backend")
set(PV_YPU_MCU_SUPPORT "OFF" CACHE BOOL "Enable support for YPU MCU backend")

include("${zoo-dev_SOURCE_DIR}/preload.cmake")

set(RELEASE_OPERATORS
  add
  addmv
  add_q510
  convert_f32_i32
  conv1d_q1417_q7_q510
  conv1d_depthwise_q1417_q7_q510
  conv1d_transposed_depthwise_q1417_q7_q510
  dot_add_iadd_q510_q510_q7_q7
  dot_add_q510_q510_q7_q7
  exp
  flip
  gaussian_attention
  gelu_approx
  layer_norm_non_affine
  lstmcell_q510
  memcpy
  memset
  mulmv
  mulmv_iadd
  mulsv
  quantize_q510
  relu
  rope
  sdpa_masked
  silu
  softmax_q31_q510
  sub_mul_q510
  transpose
)

set(ENV{PV_YPU_RELEASE_OPERATORS} "${RELEASE_OPERATORS}")

project(orca LANGUAGES C CXX)

if (PV_CMAKE_ENABLE_ASAN)
  set(PV_ENABLE_RELEASE_TESTS OFF)
endif ()

include("${zoo-dev_SOURCE_DIR}/config.cmake")
pv_util_func_hash_gen_add_dir(MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_peer_subdirectory(zoo-dev)
add_peer_subdirectory(ypu)
add_peer_subdirectory(model)
add_peer_subdirectory(normalizer)
add_peer_subdirectory(hippo)

set(MODULE_VERSION 3.0.0)

message(CHECK_START "Generating clean public header file for ${MODULE}")
execute_process(
  COMMAND ${PV_PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/script/clean_public_header.py
  -o ${PROJECT_BINARY_DIR}/include
  -p ${zoo-dev_BINARY_DIR}/include/picovoice.h)
message(CHECK_PASS "done")

include(cmake/speakers.cmake)
include(cmake/languages.cmake)

set(pv_orca_additional_sources "")
set(pv_orca_additional_system_include_dirs "")
set(pv_orca_additional_pv_deps "")

if (NOT ${PV_TARGET_PLATFORM} STREQUAL "wasm")
  list(APPEND pv_orca_additional_pv_deps pv_audio_private)
endif ()

if (ANDROID_ABI)
  if (${ANDROID_ABI} STREQUAL "armeabi-v7a" OR ${ANDROID_ABI} STREQUAL "arm64-v8a")
    list(APPEND pv_orca_additional_sources ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)
    list(APPEND pv_orca_additional_system_include_dirs ${ANDROID_NDK}/sources/android/cpufeatures)
  endif ()
endif ()

pv_module_register(
  MODULE_NAME pv_orca
  INCLUDE_DIRECTORIES include
  SYSTEM_INCLUDE_DIRECTORIES "${pv_orca_additional_system_include_dirs}"
  HEADERS
    include/orca/pv_adanorm.h
    include/orca/pv_additive_coupling.h
    include/orca/pv_affine.h
    include/orca/pv_buffer.h
    include/orca/pv_buffer_ypu.h
    include/orca/pv_cnn.h
    include/orca/pv_cnn_transposed_depthwise.h
    include/orca/pv_convnext.h
    include/orca/pv_convnext_film.h
    include/orca/pv_convnext_transposed.h
    include/orca/pv_embed.h
    include/orca/pv_layer_norm.h
    include/orca/pv_orca.h
    include/orca/pv_orca_duration_predictor.h
    include/orca/pv_orca_fft.h
    include/orca/pv_orca_gaussian_upsampler.h
    include/orca/pv_orca_internal.h
    include/orca/pv_orca_istft.h
    include/orca/pv_orca_lfm_condition_fuser.h
    include/orca/pv_orca_lfm_film_generator.h
    include/orca/pv_orca_lfm_vf_estimator.h
    include/orca/pv_orca_lfm_vf_estimator_param.h
    include/orca/pv_orca_language_data.h
    include/orca/pv_orca_phonemizer.h
    include/orca/pv_orca_pqmf.h
    include/orca/pv_orca_prior_encoder_film_generator.h
    include/orca/pv_orca_prior_encoder_flow.h
    include/orca/pv_orca_stream_state.h
    include/orca/pv_orca_synthesizer.h
    include/orca/pv_orca_util.h
    include/orca/pv_orca_vocoder.h
    include/orca/pv_profiler.h
    include/orca/pv_rope_attention.h
    include/orca/pv_rope_ffn.h
    include/orca/pv_rope_transformer.h
    include/orca/pv_rope_transformer_film_conditioned.h
    include/orca/pv_vocos_backbone.h
  SOURCES
    src/pv_adanorm.c
    src/pv_additive_coupling.c
    src/pv_affine.c
    src/pv_buffer.c
    src/pv_buffer_ypu.c
    src/pv_cnn.c
    src/pv_cnn_transposed_depthwise.c
    src/pv_convnext.c
    src/pv_convnext_film.c
    src/pv_convnext_transposed.c
    src/pv_embed.c
    src/pv_layer_norm.c
    src/pv_orca.c
    src/pv_orca_duration_predictor.c
    src/pv_orca_fft.c
    src/pv_orca_gaussian_upsampler.c
    src/pv_orca_istft.c
    src/pv_orca_lfm_condition_fuser.c
    src/pv_orca_lfm_film_generator.c
    src/pv_orca_lfm_vf_estimator.c
    src/pv_orca_phonemizer.c
    src/pv_orca_pqmf.c
    src/pv_orca_prior_encoder_film_generator.c
    src/pv_orca_prior_encoder_flow.c
    src/pv_orca_stream_state.c
    src/pv_orca_synthesizer.c
    src/pv_orca_util.c
    src/pv_orca_vocoder.c
    src/pv_profiler.c
    src/pv_rope_attention.c
    src/pv_rope_ffn.c
    src/pv_rope_transformer.c
    src/pv_rope_transformer_film_conditioned.c
    src/pv_vocos_backbone.c
    src/language_data/pv_orca_word_character_data_de.c
    src/language_data/pv_orca_word_character_data_en.c
    src/language_data/pv_orca_word_character_data_es.c
    src/language_data/pv_orca_word_character_data_fr.c
    src/language_data/pv_orca_word_character_data_it.c
    src/language_data/pv_orca_word_character_data_ja.c
    src/language_data/pv_orca_word_character_data_ko.c
    src/language_data/pv_orca_word_character_data_pt.c
    ${pv_orca_additional_sources}
  PV_DEPENDENCIES
    pv_core
    pv_dump
    pv_hippo_private
    pv_io
    pv_lm
    pv_normalizer
    pv_file
    pv_resampler_private
    pv_util
    pv_ypu
    ${pv_orca_additional_pv_deps}
  PV_PRIVATE_DEPENDENCIES
    pv_language_json
    pv_math
  GATEKEEPER pv_gatekeeper_usage_animal
  ENABLE_ASAN
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
    ${hippo_SOURCE_DIR}/src/pv_classifier_en.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_en.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "${PROJECT_SOURCE_DIR}/src/model/pv_orca_synthesizer_param .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_dict .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_lexicon .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_en}
)

# GERMAN
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${hippo_SOURCE_DIR}/src/pv_classifier_de.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_de.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "${PROJECT_SOURCE_DIR}/src/model/pv_orca_synthesizer_param .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_dict .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_lexicon .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_de}
)

# FRENCH
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${hippo_SOURCE_DIR}/src/pv_classifier_fr.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_fr.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "${PROJECT_SOURCE_DIR}/src/model/pv_orca_synthesizer_param .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_dict .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_lexicon .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_fr}
)

# SPANISH
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${hippo_SOURCE_DIR}/src/pv_classifier_es.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_es.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "${PROJECT_SOURCE_DIR}/src/model/pv_orca_synthesizer_param .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_dict .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_lexicon .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_es}
)

# ITALIAN
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${hippo_SOURCE_DIR}/src/pv_classifier_it.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_it.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "${PROJECT_SOURCE_DIR}/src/model/pv_orca_synthesizer_param .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_dict .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_lexicon .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_it}
)

# PORTUGUESE
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${hippo_SOURCE_DIR}/src/pv_classifier_pt.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_pt.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "${PROJECT_SOURCE_DIR}/src/model/pv_orca_synthesizer_param .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_dict .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_lexicon .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_pt}
)

# KOREAN
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${hippo_SOURCE_DIR}/src/pv_classifier_ko.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_ko.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "${PROJECT_SOURCE_DIR}/src/model/pv_orca_synthesizer_param .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_dict .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_lexicon .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_ko}
)

# JAPANESE
pv_module_build_app_additional_sources(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_serializer
  SOURCES
    ${hippo_SOURCE_DIR}/src/pv_classifier_ja.c
    app/pv_orca_serializer.c
    src/dict/pv_noun_gender_dict_ja.c
  PV_DEPENDENCIES pv_language_json pv_model_offline pv_hippo
  ADDITIONAL_SOURCES_TEMPLATES
    "${PROJECT_SOURCE_DIR}/src/model/pv_orca_synthesizer_param .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_dict .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_lexicon .c"
    "${PROJECT_SOURCE_DIR}/src/dict/pv_heteronym_tree .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_ja}
)

if (PV_BUILD_APPS)
  pv_module_register(
    MODULE_NAME pv_orca_metric
    HEADERS
      include/orca/pv_orca_metric_internal.h
    SOURCES
      test/pv_orca_metric.c
    PV_DEPENDENCIES
      pv_language_json
      pv_model_online
      pv_resampler_private
      pv_ypu
  )

  pv_module_build_app_additional_sources(
    MODULE_NAME pv_orca_metric
    APP_NAME pv_orca_metric_serializer
    SOURCES
      app/pv_orca_metric_serializer.c
    ADDITIONAL_SOURCES_TEMPLATES
      "${PROJECT_SOURCE_DIR}/test/model/pv_orca_metric_classifier .c"
    SOURCE_IDENTIFIER_LIST ${orca_language_code_list}
  )
endif ()

pv_module_build_app(
  MODULE_NAME pv_orca
  APP_NAME pv_orca_synthesizer_bench
  SOURCES app/pv_orca_synthesizer_bench.c
)

set(TEST_DEPENDENCIES "")

if (NOT PV_BUILD_SIMULATED_MCU)
    list(APPEND TEST_DEPENDENCIES "pv_cjson")
endif ()

if (PV_TARGET_FILE_SYSTEM AND (NOT ${PV_HOST_PLATFORM} STREQUAL "wasm"))
  list(APPEND TEST_DEPENDENCIES "pv_audio")
endif ()

pv_module_register_tests(
  MODULE_NAME pv_orca
  UNIT_TEST_SOURCES
    src/pv_orca.c
    src/model/pv_orca_synthesizer_param_en_female.c
    test/pv_orca_metric.c
    test/test_pv_adanorm.c
    test/test_pv_additive_coupling.c
    test/test_pv_cnn.c
    test/test_pv_convnext.c
    test/test_pv_convnext_film.c
    test/test_pv_embed.c
    test/test_pv_gaussian_upsampler_attention.c
    test/test_pv_layer_norm.c
    test/test_pv_orca.c
    test/test_pv_orca_dictionary.c
    test/test_pv_orca_duration_predictor.c
    test/test_pv_orca_gaussian_upsampler.c
    test/test_pv_orca_integration.c
    test/test_pv_orca_istft.c
    test/test_pv_orca_lfm_film_generator.c
    test/test_pv_orca_lfm_noise_prediction.c
    test/test_pv_orca_metric.c
    test/test_pv_orca_multilingual.c
    test/test_pv_orca_phonemizer.c
    test/test_pv_orca_prior_encoder_film_generator.c
    test/test_pv_orca_prior_encoder_flow.c
    test/test_pv_orca_synthesizer.c
    test/test_pv_orca_util.c
    test/test_pv_orca_vocoder.c
    test/test_pv_rope_attention.c
    test/test_pv_rope_ffn.c
    test/test_pv_rope_transformer.c
    test/test_pv_rope_transformer_film_conditioned.c
    test/test_pv_vocos_backbone.c
  PERFORMANCE_TEST_SOURCES test/test_performance_pv_orca.c
  PV_DEPENDENCIES
    pv_tokenizer
    pv_io
    pv_model_online
    pv_ypu
    ${TEST_DEPENDENCIES}
  TEST_RESOURCES
    ${zoo-dev_SOURCE_DIR}/res/ref/shut_off_lights.c
    ENABLE_ASAN
)

pv_module_build_tests(MODULE_NAME pv_orca)
pv_module_build_test_app(MODULE_NAME pv_orca)

set(ORCA_TEST_RESOURCES
  "${PROJECT_SOURCE_DIR}/res"
  "${zoo-dev_SOURCE_DIR}/res/phonebook"
  "${zoo-dev_SOURCE_DIR}/res/hippo"
  "${zoo-dev_SOURCE_DIR}/res/tokenizer"
)
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
