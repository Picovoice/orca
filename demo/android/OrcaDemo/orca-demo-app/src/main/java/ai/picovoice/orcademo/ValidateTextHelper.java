package ai.picovoice.orcademo;

public class ValidateTextHelper {

    public static String filterValidCharsKo(String input) {
        StringBuilder invalidChars = new StringBuilder();

        for (int i = 0; i < input.length(); ) {
            int codePoint = input.codePointAt(i);
            i += Character.charCount(codePoint);

            boolean isStandardJamo = 
                (codePoint >= 0x1100 && codePoint <= 0x11FF) || // Hangul Jamo
                (codePoint >= 0x3130 && codePoint <= 0x318F) || // Hangul Compatibility Jamo
                (codePoint >= 0xAC00 && codePoint <= 0xD7AF) || // Hangul Syllables
                (codePoint >= 0xA960 && codePoint <= 0xA97F) || // Hangul Jamo Extended-A
                (codePoint >= 0xD7B0 && codePoint <= 0xD7FF);   // Hangul Jamo Extended-B

            if (!isStandardJamo) {
                invalidChars.append(codePoint);
            }
        }

        return invalidChars.toString();
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
