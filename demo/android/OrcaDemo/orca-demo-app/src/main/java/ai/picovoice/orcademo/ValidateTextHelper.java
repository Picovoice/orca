package ai.picovoice.orcademo;

import java.text.Normalizer;

public class ValidateTextHelper {
    public static String filterValidCharsJa(String input) {
        StringBuilder invalidChars = new StringBuilder();

        for (int i = 0; i < input.length(); ) {
            int codePoint = input.codePointAt(i);

            boolean isJapanese =
                (codePoint >= 0x3001 && codePoint <= 0x301F) || // punctuation
                (codePoint >= 0x3040 && codePoint <= 0x309F) || // hiragana
                (codePoint >= 0x30A0 && codePoint <= 0x30FF) || // katakana
                (codePoint >= 0x4E00 && codePoint <= 0x9FFF);   // kanji

            if (!isJapanese) {
                invalidChars.appendCodePoint(codePoint);
            }

            i += Character.charCount(codePoint);
        }

        return invalidChars.toString();
    }

    public static String decomposeAndFilterStandardJamoKo(String input) {
        StringBuilder invalidChars = new StringBuilder();
        String decomposed = Normalizer.normalize(input, Normalizer.Form.NFD);

        for (char c : decomposed.toCharArray()) {
            boolean isStandardJamo = c >= 0x1100 && c <= 0x11FF;
            if (!isStandardJamo) {
                invalidChars.append(c);
            }
        }

        return invalidChars.toString();
    }
}
