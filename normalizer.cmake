include(cmake/speakers.cmake)

add_subdirectory(lib/mecab)

pv_module_register(
  MODULE_NAME pv_normalizer
  INCLUDE_DIRECTORIES include
  HEADERS
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_abbreviation_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_acronym_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_currency_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_date_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_measurement_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_normalizable_character_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_number_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_punctuation_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_special_character_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_top_level_domain_data_en.h
    include/orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_word_character_data_en.h

    include/orca/normalizer/en/pv_normalizer_stream_context_scanner_en.h
    include/orca/normalizer/en/pv_normalizer_tagger_en.h
    include/orca/normalizer/en/pv_normalizer_tags_en.h
    include/orca/normalizer/en/pv_normalizer_use_cases_en.h
    include/orca/normalizer/en/pv_normalizer_util_en.h
    include/orca/normalizer/en/pv_normalizer_verbalizer_en.h

    include/orca/normalizer/de/pv_normalizer_data_de/pv_normalizer_data_de.h
    include/orca/normalizer/de/pv_normalizer_stream_context_scanner_de.h
    include/orca/normalizer/de/pv_normalizer_tagger_de.h
    include/orca/normalizer/de/pv_normalizer_verbalizer_de.h
    include/orca/normalizer/de/pv_normalizer_tags_de.h
    include/orca/normalizer/de/pv_normalizer_use_cases_de.h
    include/orca/normalizer/de/pv_normalizer_util_de.h

    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_abbreviation_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_acronym_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_currency_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_date_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_measurement_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_normalizable_character_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_number_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_punctuation_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_special_character_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_top_level_domain_data_fr.h
    include/orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_word_character_data_fr.h

    include/orca/normalizer/fr/pv_normalizer_stream_context_scanner_fr.h
    include/orca/normalizer/fr/pv_normalizer_tagger_fr.h
    include/orca/normalizer/fr/pv_normalizer_verbalizer_fr.h
    include/orca/normalizer/fr/pv_normalizer_tags_fr.h
    include/orca/normalizer/fr/pv_normalizer_use_cases_fr.h
    include/orca/normalizer/fr/pv_normalizer_util_fr.h

    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_abbreviation_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_acronym_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_currency_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_date_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_measurement_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_normalizable_character_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_number_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_punctuation_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_special_character_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_top_level_domain_data_es.h
    include/orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_word_character_data_es.h

    include/orca/normalizer/es/pv_normalizer_stream_context_scanner_es.h
    include/orca/normalizer/es/pv_normalizer_tagger_es.h
    include/orca/normalizer/es/pv_normalizer_verbalizer_es.h
    include/orca/normalizer/es/pv_normalizer_tags_es.h
    include/orca/normalizer/es/pv_normalizer_use_cases_es.h
    include/orca/normalizer/es/pv_normalizer_util_es.h

    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_abbreviation_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_acronym_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_currency_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_date_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_measurement_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_normalizable_character_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_number_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_punctuation_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_special_character_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_top_level_domain_data_it.h
    include/orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_word_character_data_it.h

    include/orca/normalizer/it/pv_normalizer_stream_context_scanner_it.h
    include/orca/normalizer/it/pv_normalizer_tagger_it.h
    include/orca/normalizer/it/pv_normalizer_verbalizer_it.h
    include/orca/normalizer/it/pv_normalizer_tags_it.h
    include/orca/normalizer/it/pv_normalizer_use_cases_it.h
    include/orca/normalizer/it/pv_normalizer_util_it.h

    include/orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_abbreviation_data_pt.h
    include/orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_acronym_data_pt.h
    include/orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_currency_data_pt.h
    include/orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_data_pt.h
    include/orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_date_data_pt.h
    include/orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_normalizable_character_data_pt.h
    include/orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_punctuation_data_pt.h
    include/orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_special_character_data_pt.h
    include/orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_top_level_domain_data_pt.h
    include/orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_word_character_data_pt.h

    include/orca/normalizer/pt/pv_normalizer_stream_context_scanner_pt.h
    include/orca/normalizer/pt/pv_normalizer_tagger_pt.h
    include/orca/normalizer/pt/pv_normalizer_verbalizer_pt.h
    include/orca/normalizer/pt/pv_normalizer_tags_pt.h
    include/orca/normalizer/pt/pv_normalizer_use_cases_pt.h
    include/orca/normalizer/pt/pv_normalizer_util_pt.h

    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_acronym_data_ko.h
    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_alphabet_data_ko.h
    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_currency_data_ko.h
    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_data_ko.h
    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_measurement_data_ko.h
    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_normalizable_character_data_ko.h
    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_number_data_ko.h
    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_punctuation_data_ko.h
    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_special_character_data_ko.h
    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_top_level_domain_data_ko.h
    include/orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_word_character_data_ko.h

    include/orca/normalizer/ko/pv_normalizer_stream_context_scanner_ko.h
    include/orca/normalizer/ko/pv_normalizer_tagger_ko.h
    include/orca/normalizer/ko/pv_normalizer_verbalizer_ko.h
    include/orca/normalizer/ko/pv_normalizer_tags_ko.h
    include/orca/normalizer/ko/pv_normalizer_use_cases_ko.h
    include/orca/normalizer/ko/pv_normalizer_util_ko.h

    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_abbreviation_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_acronym_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_alphanum_spell_out_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_currency_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_date_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_measurement_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_normalizable_character_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_number_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_punctuation_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_special_character_data_ja.h
    include/orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_top_level_domain_data_ja.h

    include/orca/normalizer/ja/pv_normalizer_stream_context_scanner_ja.h
    include/orca/normalizer/ja/pv_normalizer_tagger_ja.h
    include/orca/normalizer/ja/pv_normalizer_verbalizer_ja.h
    include/orca/normalizer/ja/pv_normalizer_tags_ja.h
    include/orca/normalizer/ja/pv_normalizer_use_cases_ja.h
    include/orca/normalizer/ja/pv_normalizer_util_ja.h
    include/orca/normalizer/ja/pv_normalizer_tokenizer_ja.h

    include/orca/normalizer/pv_normalizer_data/pv_normalizer_shared_data.h

    include/orca/normalizer/pv_normalizer.h
    include/orca/normalizer/pv_normalizer_language_data.h
    include/orca/normalizer/pv_normalizer_stream.h
    include/orca/normalizer/pv_normalizer_stream_context_scanner.h
    include/orca/normalizer/pv_normalizer_tagger.h
    include/orca/normalizer/pv_normalizer_tags.h
    include/orca/normalizer/pv_normalizer_token.h
    include/orca/normalizer/pv_normalizer_tokenizer.h
    include/orca/normalizer/pv_normalizer_tokenizer_generic.h
    include/orca/normalizer/pv_normalizer_use_cases.h
    include/orca/normalizer/pv_normalizer_util.h
    include/orca/normalizer/pv_normalizer_verbalizer.h
  SOURCES
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_abbreviation_data_en.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_acronym_data_en.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_currency_data_en.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_date_data_en.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_measurement_data_en.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_normalizable_character_data_en.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_number_data_en.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_punctuation_data_en.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_special_character_data_en.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_top_level_domain_data_en.c
    src/normalizer/en/pv_normalizer_data_en/pv_normalizer_word_character_data_en.c

    src/normalizer/en/pv_normalizer_stream_context_scanner_en.c
    src/normalizer/en/pv_normalizer_tagger_en.c
    src/normalizer/en/pv_normalizer_use_cases_en.c
    src/normalizer/en/pv_normalizer_util_en.c
    src/normalizer/en/pv_normalizer_verbalizer_en.c

    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_abbreviation_data_de.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_acronym_data_de.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_currency_data_de.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_date_data_de.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_measurement_data_de.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_normalizable_character_data_de.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_number_data_de.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_punctuation_data_de.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_special_character_data_de.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_top_level_domain_data_de.c
    src/normalizer/de/pv_normalizer_data_de/pv_normalizer_word_character_data_de.c

    src/normalizer/de/pv_normalizer_stream_context_scanner_de.c
    src/normalizer/de/pv_normalizer_tagger_de.c
    src/normalizer/de/pv_normalizer_use_cases_de.c
    src/normalizer/de/pv_normalizer_util_de.c
    src/normalizer/de/pv_normalizer_verbalizer_de.c

    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_abbreviation_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_acronym_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_character_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_currency_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_date_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_measurement_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_normalizable_character_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_number_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_punctuation_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_special_character_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_top_level_domain_data_fr.c
    src/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_word_character_data_fr.c

    src/normalizer/fr/pv_normalizer_stream_context_scanner_fr.c
    src/normalizer/fr/pv_normalizer_tagger_fr.c
    src/normalizer/fr/pv_normalizer_use_cases_fr.c
    src/normalizer/fr/pv_normalizer_util_fr.c
    src/normalizer/fr/pv_normalizer_verbalizer_fr.c

    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_abbreviation_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_acronym_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_character_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_currency_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_date_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_measurement_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_normalizable_character_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_number_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_punctuation_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_special_character_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_top_level_domain_data_es.c
    src/normalizer/es/pv_normalizer_data_es/pv_normalizer_word_character_data_es.c

    src/normalizer/es/pv_normalizer_stream_context_scanner_es.c
    src/normalizer/es/pv_normalizer_tagger_es.c
    src/normalizer/es/pv_normalizer_use_cases_es.c
    src/normalizer/es/pv_normalizer_util_es.c
    src/normalizer/es/pv_normalizer_verbalizer_es.c

    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_abbreviation_data_it.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_acronym_data_it.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_currency_data_it.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_date_data_it.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_measurement_data_it.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_normalizable_character_data_it.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_number_data_it.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_punctuation_data_it.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_special_character_data_it.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_top_level_domain_data_it.c
    src/normalizer/it/pv_normalizer_data_it/pv_normalizer_word_character_data_it.c

    src/normalizer/it/pv_normalizer_stream_context_scanner_it.c
    src/normalizer/it/pv_normalizer_tagger_it.c
    src/normalizer/it/pv_normalizer_use_cases_it.c
    src/normalizer/it/pv_normalizer_util_it.c
    src/normalizer/it/pv_normalizer_verbalizer_it.c

    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_abbreviation_data_pt.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_acronym_data_pt.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_currency_data_pt.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_date_data_pt.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_measurement_data_pt.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_normalizable_character_data_pt.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_number_data_pt.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_punctuation_data_pt.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_special_character_data_pt.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_top_level_domain_data_pt.c
    src/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_word_character_data_pt.c

    src/normalizer/pt/pv_normalizer_stream_context_scanner_pt.c
    src/normalizer/pt/pv_normalizer_tagger_pt.c
    src/normalizer/pt/pv_normalizer_use_cases_pt.c
    src/normalizer/pt/pv_normalizer_util_pt.c
    src/normalizer/pt/pv_normalizer_verbalizer_pt.c

    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_acronym_data_ko.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_alphabet_data_ko.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_character_data_ko.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_currency_data_ko.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_measurement_data_ko.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_normalizable_character_data_ko.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_number_data_ko.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_punctuation_data_ko.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_special_character_data_ko.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_top_level_domain_data_ko.c
    src/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_word_character_data_ko.c

    src/normalizer/ko/pv_normalizer_stream_context_scanner_ko.c
    src/normalizer/ko/pv_normalizer_tagger_ko.c
    src/normalizer/ko/pv_normalizer_use_cases_ko.c
    src/normalizer/ko/pv_normalizer_util_ko.c
    src/normalizer/ko/pv_normalizer_verbalizer_ko.c

    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_abbreviation_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_acronym_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_alphanum_spell_out_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_currency_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_date_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_measurement_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_normalizable_character_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_number_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_punctuation_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_special_character_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_top_level_domain_data_ja.c
    src/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_word_character_data_ja.c

    src/normalizer/ja/pv_normalizer_stream_context_scanner_ja.c
    src/normalizer/ja/pv_normalizer_tagger_ja.c
    src/normalizer/ja/pv_normalizer_use_cases_ja.c
    src/normalizer/ja/pv_normalizer_util_ja.c
    src/normalizer/ja/pv_normalizer_verbalizer_ja.c
    src/normalizer/ja/pv_normalizer_tokenizer_ja.c

    src/normalizer/pv_normalizer.c
    src/normalizer/pv_normalizer_stream.c
    src/normalizer/pv_normalizer_stream_context_scanner.c
    src/normalizer/pv_normalizer_tagger.c
    src/normalizer/pv_normalizer_token.c
    src/normalizer/pv_normalizer_tokenizer.c
    src/normalizer/pv_normalizer_tokenizer_generic.c
    src/normalizer/pv_normalizer_util.c
    src/normalizer/pv_normalizer_verbalizer.c

  PV_DEPENDENCIES
    pv_core
    pv_file
    pv_io
    pv_language_json
    pv_util
    pv_lm
    pv_hippo_normalizer
  SYSTEM_INCLUDE_DIRECTORIES
    lib/mecab/src
  UNMOCKED_DEPENDENCIES
    mecab
)

pv_module_build_static(MODULE_NAME pv_normalizer)

pv_module_register_tests(
  MODULE_NAME pv_normalizer
  UNIT_TEST_SOURCES
    test/normalizer/en/test_pv_normalizer_tagger_en.c
    test/normalizer/en/test_pv_normalizer_verbalizer_en.c
    test/normalizer/en/test_pv_normalizer_stream_en.c

    test/normalizer/de/test_pv_normalizer_tagger_de.c
    test/normalizer/de/test_pv_normalizer_verbalizer_de.c
    test/normalizer/de/test_pv_normalizer_stream_de.c
    test/normalizer/de/test_pv_normalizer_util_de.c

    test/normalizer/fr/test_pv_normalizer_tagger_fr.c
    test/normalizer/fr/test_pv_normalizer_verbalizer_fr.c
    test/normalizer/fr/test_pv_normalizer_stream_fr.c
    test/normalizer/fr/test_pv_normalizer_util_fr.c

    test/normalizer/es/test_pv_normalizer_tagger_es.c
    test/normalizer/es/test_pv_normalizer_verbalizer_es.c
    test/normalizer/es/test_pv_normalizer_stream_es.c
    test/normalizer/es/test_pv_normalizer_util_es.c

    test/normalizer/it/test_pv_normalizer_tagger_it.c
    test/normalizer/it/test_pv_normalizer_verbalizer_it.c
    test/normalizer/it/test_pv_normalizer_stream_it.c
    test/normalizer/it/test_pv_normalizer_util_it.c

    test/normalizer/pt/test_pv_normalizer_tagger_pt.c
    test/normalizer/pt/test_pv_normalizer_verbalizer_pt.c
    test/normalizer/pt/test_pv_normalizer_stream_pt.c
    test/normalizer/pt/test_pv_normalizer_util_pt.c

    test/normalizer/ko/test_pv_normalizer_tagger_ko.c
    test/normalizer/ko/test_pv_normalizer_verbalizer_ko.c
    test/normalizer/ko/test_pv_normalizer_stream_ko.c
    test/normalizer/ko/test_pv_normalizer_util_ko.c

    test/normalizer/ja/test_pv_normalizer_tokenizer_ja.c
    test/normalizer/ja/test_pv_normalizer_tagger_ja.c
    test/normalizer/ja/test_pv_normalizer_verbalizer_ja.c
    test/normalizer/ja/test_pv_normalizer_stream_ja.c
    test/normalizer/ja/test_pv_normalizer_util_ja.c

    test/normalizer/test_pv_normalizer.c
    test/normalizer/test_pv_normalizer_token.c
    test/normalizer/test_pv_normalizer_tokenizer.c
    test/normalizer/test_pv_normalizer_util.c

    test/normalizer/test_pv_normalizer_cases.c
    test/normalizer/test_pv_normalizer_cases_helper.c
    test/normalizer/test_pv_normalizer_stream_helper.c
  PV_DEPENDENCIES
    pv_tokenizer
)
pv_module_build_tests(MODULE_NAME pv_normalizer)

# ENGLISH
pv_module_build_app_additional_sources(
  MODULE_NAME pv_normalizer
  APP_NAME pv_normalizer_app
  SOURCES
    app/normalizer/pv_normalizer_app.c
    src/dict/pv_noun_gender_dict_en.c
  PV_DEPENDENCIES
    pv_core
    pv_language_json
    pv_lm
  ADDITIONAL_SOURCES_TEMPLATES
    "src/dict/pv_lexicon .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_en}
)

# GERMAN
pv_module_build_app_additional_sources(
  MODULE_NAME pv_normalizer
  APP_NAME pv_normalizer_app
  SOURCES
    app/normalizer/pv_normalizer_app.c
    src/dict/pv_noun_gender_dict_de.c
  PV_DEPENDENCIES
    pv_core
    pv_language_json
    pv_lm
  ADDITIONAL_SOURCES_TEMPLATES
    "src/dict/pv_lexicon .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_de}
)

# FRENCH
pv_module_build_app_additional_sources(
  MODULE_NAME pv_normalizer
  APP_NAME pv_normalizer_app
  SOURCES
    app/normalizer/pv_normalizer_app.c
    src/dict/pv_noun_gender_dict_fr.c
  PV_DEPENDENCIES
    pv_core
    pv_language_json
    pv_lm
  ADDITIONAL_SOURCES_TEMPLATES
    "src/dict/pv_lexicon .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_fr}
)

# SPANISH
pv_module_build_app_additional_sources(
  MODULE_NAME pv_normalizer
  APP_NAME pv_normalizer_app
  SOURCES
    app/normalizer/pv_normalizer_app.c
    src/dict/pv_noun_gender_dict_es.c
  PV_DEPENDENCIES
    pv_core
    pv_language_json
    pv_lm
  ADDITIONAL_SOURCES_TEMPLATES
    "src/dict/pv_lexicon .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_es}
)

# ITALIAN
pv_module_build_app_additional_sources(
  MODULE_NAME pv_normalizer
  APP_NAME pv_normalizer_app
  SOURCES
    app/normalizer/pv_normalizer_app.c
    src/dict/pv_noun_gender_dict_it.c
  PV_DEPENDENCIES
    pv_core
    pv_language_json
    pv_lm
  ADDITIONAL_SOURCES_TEMPLATES
    "src/dict/pv_lexicon .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_it}
)

# KOREAN
pv_module_build_app_additional_sources(
  MODULE_NAME pv_normalizer
  APP_NAME pv_normalizer_app
  SOURCES
    app/normalizer/pv_normalizer_app.c
    src/dict/pv_noun_gender_dict_ko.c
  PV_DEPENDENCIES
    pv_core
    pv_language_json
    pv_lm
  ADDITIONAL_SOURCES_TEMPLATES
    "src/dict/pv_lexicon .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_ko}
)

# PORTUGUESE
pv_module_build_app_additional_sources(
  MODULE_NAME pv_normalizer
  APP_NAME pv_normalizer_app
  SOURCES
    app/normalizer/pv_normalizer_app.c
    src/dict/pv_noun_gender_dict_pt.c
  PV_DEPENDENCIES
    pv_core
    pv_language_json
    pv_lm
  ADDITIONAL_SOURCES_TEMPLATES
    "src/dict/pv_lexicon .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_pt}
)

# JAPANESE
pv_module_build_app_additional_sources(
  MODULE_NAME pv_normalizer
  APP_NAME pv_normalizer_app
  SOURCES
    app/normalizer/pv_normalizer_app.c
    src/dict/pv_noun_gender_dict_ja.c
  PV_DEPENDENCIES
    pv_core
    pv_language_json
    pv_lm
  ADDITIONAL_SOURCES_TEMPLATES
    "src/dict/pv_lexicon .c"
  SOURCE_IDENTIFIER_LIST ${speaker_identifier_list_ja}
)

pv_module_build_app(
  MODULE_NAME pv_normalizer
  APP_NAME pv_normalizer_tokenizer_app
  SOURCES app/normalizer/pv_normalizer_tokenizer_app.c
  PV_DEPENDENCIES
    pv_core
    pv_language_json
)

pv_module_build_app(
  MODULE_NAME pv_normalizer
  APP_NAME pv_normalizer_cases_app
  SOURCES
    app/normalizer/pv_normalizer_cases_app.c
    test/normalizer/test_pv_normalizer_cases_helper.c
  PV_DEPENDENCIES
    pv_core
    pv_language_json
    pv_tokenizer
    pv_lm
)
