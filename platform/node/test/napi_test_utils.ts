import * as fs from "fs";

import {distance} from "fastest-levenshtein";
import {WaveFile} from "wavefile";

export const ACCESS_KEY = process.argv
    .filter((x) => x.startsWith('--access_key='))[0]?.split('--access_key=')[1];
if (!ACCESS_KEY) {
    throw Error("Required argument `--access_key` is missing")
}

export const PLATFORM = process.argv
    .filter((x) => x.startsWith('--platform='))[0]?.split('--platform=')[1];

if (!PLATFORM) {
    throw Error("Required argument `--platform` is missing")
}

export function readAudioFile(audioFilePath: string) {
    const waveFile = new WaveFile(fs.readFileSync(audioFilePath));
    return waveFile.getSamples(false, Int16Array);
}

export const measureCharacterErrorRate = (transcript: string, expectedTranscript: string) => {
    return distance(transcript, expectedTranscript) / expectedTranscript.length
}
