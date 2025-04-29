import Foundation

class ValidateTextHelper {

    static func filterValidCharsKo(_ input: String) -> String {
        var invalidChars = ""

        for scalar in input.unicodeScalars {
            let codePoint = scalar.value

            let isStandardJamo = 
                (codePoint >= 0x1100 && codePoint <= 0x11FF) || // Hangul Jamo
                (codePoint >= 0x3130 && codePoint <= 0x318F) || // Hangul Compatibility Jamo
                (codePoint >= 0xAC00 && codePoint <= 0xD7AF) || // Hangul Syllables
                (codePoint >= 0xA960 && codePoint <= 0xA97F) || // Hangul Jamo Extended-A
                (codePoint >= 0xD7B0 && codePoint <= 0xD7FF);   // Hangul Jamo Extended-B

            if !isStandardJamo {
                invalidChars.unicodeScalars.append(scalar)
            }
        }

        return invalidChars
    }

    static func filterValidCharsJa(_ input: String) -> String {
        var invalidChars = ""

        for scalar in input.unicodeScalars {
            let codePoint = scalar.value

            let isJapanese =
                (codePoint >= 0x3001 && codePoint <= 0x301F) || // punctuation
                (codePoint >= 0x3040 && codePoint <= 0x309F) || // hiragana
                (codePoint >= 0x30A0 && codePoint <= 0x30FF) || // katakana
                (codePoint >= 0x4E00 && codePoint <= 0x9FFF)    // kanji

            if !isJapanese {
                invalidChars.unicodeScalars.append(scalar)
            }
        }

        return invalidChars
    }
}
