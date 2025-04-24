import Foundation

class ValidateTextHelper {

    static func decomposeHangul(_ input: String) -> String {
        let HANGUL_UNICODE_BASE = 0xAC00
        let HANGUL_DECOMPOSED_ARRAY: [String] = [
            // Initial consonants
            "ㄱ", "ㄲ", "ㄴ", "ㄷ", "ㄸ", "ㄹ", "ㅁ", "ㅂ", "ㅃ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅉ", "ㅊ", "ㅋ",
            "ㅌ", "ㅍ", "ㅎ",
            // Medial vowels
            "ㅏ", "ㅐ", "ㅑ", "ㅒ", "ㅓ", "ㅔ", "ㅕ", "ㅖ", "ㅗ", "ㅘ", "ㅙ", "ㅚ", "ㅛ", "ㅜ", "ㅝ", "ㅞ",
            "ㅟ", "ㅠ", "ㅡ", "ㅢ", "ㅣ",
            // Final consonants
            "", "ㄱ", "ㄲ", "ㄳ", "ㄴ", "ㄵ", "ㄶ", "ㄷ", "ㄹ", "ㄺ", "ㄻ", "ㄼ", "ㄽ", "ㄾ", "ㄿ", "ㅀ",
            "ㅁ", "ㅂ", "ㅄ", "ㅅ", "ㅆ", "ㅇ", "ㅈ", "ㅊ", "ㅋ", "ㅌ", "ㅍ", "ㅎ"
        ]

        var decomposed = ""

        for scalar in input.unicodeScalars {
            let codePoint = Int(scalar.value)

            if codePoint < HANGUL_UNICODE_BASE {
                decomposed.unicodeScalars.append(scalar)
                continue
            }

            var curr = codePoint - HANGUL_UNICODE_BASE
            let initial = curr / 588
            curr %= 588
            let medial = (curr / 28) + 19
            curr %= 28
            let finalConsonant = curr + 19 + 21

            if initial > 18 {
                decomposed.unicodeScalars.append(scalar)
                continue
            }

            decomposed += HANGUL_DECOMPOSED_ARRAY[initial]
            decomposed += HANGUL_DECOMPOSED_ARRAY[medial]
            decomposed += HANGUL_DECOMPOSED_ARRAY[finalConsonant]
        }

        return decomposed
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
