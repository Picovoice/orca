from enum import Enum


class Speakers(Enum):
    EN_MALE = 'en_male'
    EN_FEMALE = 'en_female'
    DE_MALE = 'de_male'
    DE_FEMALE = 'de_female'
    FR_MALE = 'fr_male'
    FR_FEMALE = 'fr_female'
    ES_FEMALE = 'es_female'
    ES_MALE = 'es_male'
    IT_FEMALE = 'it_female'
    PT_MALE = 'pt_male'
    IT_MALE = 'it_male'
    PT_FEMALE = 'pt_female'
    JA_FEMALE = 'ja_female'
    JA_MALE = 'ja_male'
    KO_FEMALE = 'ko_female'


class Languages(Enum):
    ENGLISH = 'en'
    GERMAN = 'de'
    FRENCH = 'fr'
    SPANISH = 'es'
    ITALIAN = 'it'
    PORTUGUESE = 'pt'
    JAPANESE = 'ja'
    KOREAN = 'ko'


speaker_language_dict = {
            Speakers.EN_MALE:   Languages.ENGLISH,
            Speakers.EN_FEMALE: Languages.ENGLISH, 
            Speakers.DE_MALE:   Languages.GERMAN,
            Speakers.DE_FEMALE:   Languages.GERMAN,
            Speakers.FR_MALE: Languages.FRENCH,
            Speakers.FR_FEMALE: Languages.FRENCH,
            Speakers.ES_FEMALE: Languages.SPANISH,
            Speakers.ES_MALE:   Languages.SPANISH,
            Speakers.IT_FEMALE: Languages.ITALIAN,
            Speakers.IT_MALE:   Languages.ITALIAN,
            Speakers.PT_MALE: Languages.PORTUGUESE,
            Speakers.PT_FEMALE: Languages.PORTUGUESE,
            Speakers.JA_FEMALE: Languages.JAPANESE,
            Speakers.JA_MALE:   Languages.JAPANESE,
            Speakers.KO_FEMALE: Languages.KOREAN,
        }


__all__ = [
    'Languages',
    'Speakers',
]
