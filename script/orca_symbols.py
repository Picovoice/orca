EXPECTED_ORCA_SYMBOLS = {
    'libpv_orca': [
        'pv_orca_delete',
        'pv_orca_init',
        'pv_orca_valid_characters',
        'pv_orca_valid_characters_delete',
        'pv_orca_sample_rate',
        'pv_orca_max_character_limit',
        'pv_orca_synthesize_params_init',
        'pv_orca_synthesize_params_delete',
        'pv_orca_synthesize_params_set_speech_rate',
        'pv_orca_synthesize_params_get_speech_rate',
        'pv_orca_synthesize_params_set_random_state',
        'pv_orca_synthesize_params_get_random_state',
        'pv_orca_synthesize',
        'pv_orca_synthesize_to_file',
        'pv_orca_stream_open',
        'pv_orca_stream_synthesize',
        'pv_orca_stream_flush',
        'pv_orca_stream_close',
        'pv_orca_pcm_delete',
        'pv_orca_word_alignments_delete',
        'pv_orca_version'
    ]
}

EXPECTED_ORCA_SYMBOLS_JNI = {
    'libpv_orca_jni': [
        'Java_ai_picovoice_orca_OrcaNative_delete',
        'Java_ai_picovoice_orca_OrcaNative_getMaxCharacterLimit',
        'Java_ai_picovoice_orca_OrcaNative_getSampleRate',
        'Java_ai_picovoice_orca_OrcaNative_getValidCharacters',
        'Java_ai_picovoice_orca_OrcaNative_getVersion',
        'Java_ai_picovoice_orca_OrcaNative_init',
        'Java_ai_picovoice_orca_OrcaNative_synthesize',
        'Java_ai_picovoice_orca_OrcaNative_synthesizeToFile',
        'Java_ai_picovoice_orca_OrcaNative_setSdk',
        'Java_ai_picovoice_orca_OrcaNative_streamOpen',
        'Java_ai_picovoice_orca_OrcaNative_streamSynthesize',
        'Java_ai_picovoice_orca_OrcaNative_streamFlush',
        'Java_ai_picovoice_orca_OrcaNative_streamClose',
    ]
}
EXPECTED_ORCA_SYMBOLS_JNI['libpv_orca_jni'] += EXPECTED_ORCA_SYMBOLS['libpv_orca']

EXPECTED_ORCA_SYMBOLS_NODE = {
    'pv_orca.node': EXPECTED_ORCA_SYMBOLS['libpv_orca']
}

EXPECTED_ORCA_SYMBOLS_WASM = {
    'pv_orca.wasm': EXPECTED_ORCA_SYMBOLS['libpv_orca'],
    'pv_orca_simd.wasm': EXPECTED_ORCA_SYMBOLS['libpv_orca']
}

EXPECTED_ORCA_SYMBOLS_ANDROID = {
    'libpv_orca': EXPECTED_ORCA_SYMBOLS_JNI['libpv_orca_jni']
}

EXPECTED_ORCA_SYMBOLS_IOS = {
    'PvOrca.xcframework/ios-arm64/PvOrca.framework/PvOrca': EXPECTED_ORCA_SYMBOLS['libpv_orca'],
    'PvOrca.xcframework/ios-arm64_x86_64-simulator/PvOrca.framework/PvOrca': EXPECTED_ORCA_SYMBOLS['libpv_orca']
}
