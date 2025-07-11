import { ACCESS_KEY } from "./napi_test_utils";
import { promises as fs } from 'fs';

const orca = require('bindings')('pv_orca');

async function deleteFile(filePath: string) {
    try {
        await fs.unlink(filePath);
    } catch (err) {
        console.error(`Error deleting the file: ${filePath}`, err);
    }
}

test("Orca", () => {
    const initResult = orca.init(
        ACCESS_KEY,
        "../../res/param/orca_params_en_female.pv");

    expect(initResult.status).toEqual(0)
    expect(initResult.handle).not.toEqual(0)

    expect(orca.version().length).toBeGreaterThan(0)

    const sampleRate = orca.sample_rate(initResult.handle).sample_rate
    expect(sampleRate).toBeGreaterThan(0)

    const validCharacters = orca.valid_characters(initResult.handle).valid_characters
    expect(validCharacters.length).toBeGreaterThan(0)

    const maxCharacterLimit = orca.max_character_limit(initResult.handle).max_character_limit
    expect(maxCharacterLimit).toBeGreaterThan(0)

    const synthesizeParams = orca.synthesize_params_init()
    expect(synthesizeParams.status).toEqual(0)

    const SPEECH_RATE = 1.0
    const setSpeechRateStatus = orca.synthesize_params_set_speech_rate(synthesizeParams.synthesize_params, SPEECH_RATE)
    expect(setSpeechRateStatus).toEqual(0)
    expect(orca.synthesize_params_get_speech_rate(synthesizeParams.synthesize_params).speech_rate).toBeCloseTo(SPEECH_RATE, 1)

    const RANDOM_STATE = 77
    const setRandomStateStatus = orca.synthesize_params_set_random_state(synthesizeParams.synthesize_params, RANDOM_STATE)
    expect(setRandomStateStatus).toEqual(0)
    expect(orca.synthesize_params_get_random_state(synthesizeParams.synthesize_params).random_state).toEqual(RANDOM_STATE)

    const tempAudioFilePath = "./orca-synthesize-to-file-temp.wav"
    const textToSynthesize = "Orca!"

    const validatePhonemes  = (phonemes: any[]) => {
        expect(phonemes.length).toBeGreaterThanOrEqual(0)
        for (let i = 0; i < phonemes.length; i++) {
            expect(phonemes[i].phoneme).toBeDefined()
            expect(phonemes[i].startSec).toBeGreaterThanOrEqual(0)
            expect(phonemes[i].startSec).toBeLessThanOrEqual(phonemes[i].endSec)
            if (i < phonemes.length - 1) {
                expect(phonemes[i].endSec).toBeLessThanOrEqual(phonemes[i + 1].startSec)
            }
        }
    }

    const validateAlignments = (alignments: any[]) => {
        expect(alignments.length).toBeGreaterThanOrEqual(0)
        for (let i = 0; i < alignments.length; i++) {
            expect(alignments[i].word).toBeDefined()
            expect(alignments[i].startSec).toBeGreaterThanOrEqual(0)
            expect(alignments[i].startSec).toBeLessThanOrEqual(alignments[i].endSec)
            if (i < alignments.length - 1) {
                expect(alignments[i].endSec).toBeLessThanOrEqual(alignments[i + 1].startSec)
            }
            validatePhonemes(alignments[i].phonemes)
        }
    }

    const synthResult = orca.synthesize(
        initResult.handle,
        textToSynthesize,
        synthesizeParams.synthesize_params);
    expect(synthResult.status).toEqual(0)
    expect(synthResult.pcm.length).toBeGreaterThan(0)
    validateAlignments(synthResult.alignments)

    const synthToFileResult = orca.synthesize_to_file(
        initResult.handle,
        textToSynthesize,
        synthesizeParams.synthesize_params,
        tempAudioFilePath);
    expect(synthToFileResult.status).toEqual(0)
    expect(synthToFileResult.pcm).toBeUndefined()
    validateAlignments(synthToFileResult.alignments)
    deleteFile(tempAudioFilePath);

    const streamOpenResult = orca.stream_open(
        initResult.handle,
        synthesizeParams.synthesize_params)
    expect(streamOpenResult.status).toEqual(0)
    const pcmOne = orca.stream_synthesize(streamOpenResult.stream, textToSynthesize)
    expect(pcmOne.pcm.length).toEqual(0)
    const pcmTwo = orca.stream_flush(streamOpenResult.stream)
    expect(pcmTwo.pcm.length).toBeGreaterThan(0)
    orca.stream_close(streamOpenResult.stream)

    orca.synthesize_params_delete(synthesizeParams.synthesize_params)
    orca.delete(initResult.handle)
});

test("NAPI Error", () => {
    const initResult = orca.init(
        ACCESS_KEY,
        "../../res/param/orca_params_en_female.pv");
    expect(initResult.status).toEqual(0)
    expect(initResult.handle).not.toEqual(0)

    let synthResult;
    try {
        synthResult = orca.synthesize(initResult.handle);
    } catch (e) {
        expect(e).not.toBeNull();
        expect(e.code).toEqual("INVALID_ARGUMENT")
    } finally {
        expect(synthResult).toEqual(undefined)
        orca.delete(initResult.handle);
    }
});

test("Message Stack", () => {
    try {
        const res = orca.init(
            "invalid",
            "../../res/param/orca_params_en_female.pv");
        expect(res).toBeNull();
    } catch (e) {
        const errObject = orca.get_error_stack();
        expect(errObject.status).toEqual(0);
        expect(errObject.message_stack.length).toBeGreaterThan(0);
    }
});
