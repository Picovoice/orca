package ai.picovoice.orcademo;

public class ValidateTextHelper {

    public static String decomposeHangul(String input) {
        final int HANGUL_UNICODE_BASE = 0xAC00;
        final String[] HANGUL_DECOMPOSED_ARRAY = new String[]{
            // Initial consonants
            "ㄱ", "ㄲ", "ㄴ", "ㄷ", "ㄸ", "ㄹ", "ㅁ", "ㅂ", "ㅃ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅉ", "ㅊ", "ㅋ",
            "ㅌ", "ㅍ", "ㅎ",
            // Medial vowels
            "ㅏ", "ㅐ", "ㅑ", "ㅒ", "ㅓ", "ㅔ", "ㅕ", "ㅖ", "ㅗ", "ㅘ", "ㅙ", "ㅚ", "ㅛ", "ㅜ", "ㅝ", "ㅞ",
            "ㅟ", "ㅠ", "ㅡ", "ㅢ", "ㅣ",
            // Final consonants
            "", "ㄱ", "ㄲ", "ㄳ", "ㄴ", "ㄵ", "ㄶ", "ㄷ", "ㄹ", "ㄺ", "ㄻ", "ㄼ", "ㄽ", "ㄾ", "ㄿ", "ㅀ",
            "ㅁ", "ㅂ", "ㅄ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ",
        };

        StringBuilder decomposed = new StringBuilder();

        for (int i = 0; i < input.length(); ) {
            int codePoint = input.codePointAt(i);
            i += Character.charCount(codePoint);

            if (codePoint < HANGUL_UNICODE_BASE) {
                decomposed.appendCodePoint(codePoint);
                continue;
            }

            int curr = codePoint - HANGUL_UNICODE_BASE;
            int initial = curr / 588;

            curr %= 588;
            int medial = (curr / 28) + 19;

            curr %= 28;
            int finalConsonant = curr + 19 + 21;

            if (initial > 18) {
                decomposed.appendCodePoint(codePoint);
                continue;
            }

            decomposed.append(HANGUL_DECOMPOSED_ARRAY[initial]);
            decomposed.append(HANGUL_DECOMPOSED_ARRAY[medial]);
            decomposed.append(HANGUL_DECOMPOSED_ARRAY[finalConsonant]);
        }

        return decomposed.toString();
    }

    public static String filterValidCharsJa(String input) {
        StringBuilder invalidChars = new StringBuilder();

        for (int i = 0; i < input.length(); ) {
            int codePoint = input.codePointAt(i);
            i += Character.charCount(codePoint);

            boolean isJapanese =
                (codePoint >= 0x3001 && codePoint <= 0x301F) || // punctuation
                (codePoint >= 0x3040 && codePoint <= 0x309F) || // hiragana
                (codePoint >= 0x30A0 && codePoint <= 0x30FF) || // katakana
                (codePoint >= 0x4E00 && codePoint <= 0x9FFF);   // kanji

            if (!isJapanese) {
                invalidChars.appendCodePoint(codePoint);
            }
        }

        return invalidChars.toString();
    }
}
