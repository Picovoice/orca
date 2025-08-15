package ai.picovoice.orca;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;

import static org.junit.jupiter.api.Assertions.*;

public class OrcaJniTest {
    private static final Path buildPath = Paths.get(System.getProperty("buildPath"));

    private static final Path libExt = Paths.get(System.getProperty("libExt"));

    private final String accessKey = System.getProperty("accessKey");

    private final Path moduleResPath = Paths.get(System.getProperty("moduleResDirectory"));

    private final String modelPath = moduleResPath.resolve("param/orca_params_en_female.pv").toString();

    private long libraryHandle;

    @BeforeAll
    static void setup() {
        System.load(buildPath.resolve(String.format("libpv_orca_jni.%s", libExt)).toString());
    }

    @AfterEach
    void tearDown() {
        if (libraryHandle != 0) {
            OrcaNative.delete(libraryHandle);
            libraryHandle = 0;
        }
    }

    void validateMetadata(OrcaWord[] words, float audioLength) {
        for (int i = 0; i < words.length; i++) {
            assertTrue(words[i].getStartSec() >= 0);
            assertTrue(words[i].getStartSec() < words[i].getEndSec());
            if (i < words.length - 1) {
                assertTrue(words[i].getEndSec() <= words[i + 1].getStartSec());
            } else {
                if (audioLength > 0) {
                    assertTrue(words[i].getEndSec() <= audioLength);
                }
            }
            OrcaPhoneme[] phonemes = words[i].getPhonemeArray();
            for (int j = 0; j < phonemes.length; j ++) {
                assertTrue(phonemes[j].getStartSec() >= 0);
                assertTrue(phonemes[j].getStartSec() < phonemes[j].getEndSec());
                if (j < phonemes.length - 1) {
                    assertTrue(phonemes[j].getEndSec() <= phonemes[j + 1].getStartSec());
                } else {
                    assertTrue(phonemes[j].getEndSec() <= words[i].getEndSec());
                }
            }
        }
    }

     @Test
     void getVersion() throws OrcaException {
         final String version = OrcaNative.getVersion();
         assertTrue(version != null && !version.equals(""));
     }

     @Test
     void setSdk() throws OrcaException {
         OrcaNative.setSdk("java");
     }

     @Test
     void basicInit() throws OrcaException {
         libraryHandle = OrcaNative.init(accessKey, modelPath);
     }

     @Test
     void basicMessageStack() {
         try {
             libraryHandle = OrcaNative.init("invalid", modelPath);
             assertEquals(0, libraryHandle);
         } catch (OrcaException e) {
             assertTrue(e.getMessageStack().length > 0);
         }
     }

     @Test
     void getSampleRate() throws OrcaException {
         libraryHandle = OrcaNative.init(accessKey, modelPath);
         assertTrue(OrcaNative.getSampleRate(libraryHandle) > 0);
     }

     @Test
     void getValidCharacters() throws OrcaException {
         libraryHandle = OrcaNative.init(accessKey, modelPath);
         String[] characters = OrcaNative.getValidCharacters(libraryHandle);
         assertTrue(characters.length > 0);
     }

     @Test
     void getMaxCharacterLimit() throws OrcaException {
         libraryHandle = OrcaNative.init(accessKey, modelPath);
         assertTrue(OrcaNative.getMaxCharacterLimit(libraryHandle) > 0);
     }

     @Test
     void testJniError() throws OrcaException {
         libraryHandle = OrcaNative.init(accessKey, modelPath);
         try {
             OrcaAudio audioObject = OrcaNative.synthesize(libraryHandle, null, 1.0f, 77);
             assertNull(audioObject);
         } catch (OrcaException e) {
             assertTrue(e.getMessage().length() > 0);
             assertNull(e.getMessageStack());
         }
     }

     @Test
     void synthesize() throws Exception, OrcaException {
         libraryHandle = OrcaNative.init(accessKey, modelPath);
         OrcaAudio audioObject = OrcaNative.synthesize(
                 libraryHandle,
                 "Orca!",
                 1.0f,
                 77);
         assertTrue(audioObject.getPcm().length > 0);
         assertTrue(audioObject.getWordArray().length > 0);
         validateMetadata(audioObject.getWordArray(), audioObject.getPcm().length);
     }

    @Test
    void synthesizeToFile() throws Exception, OrcaException {
        File tempFile = File.createTempFile("temp", ".tmp");
        String tempFileLocation = tempFile.getAbsolutePath();

        libraryHandle = OrcaNative.init(accessKey, modelPath);
        OrcaAudio audioObject = OrcaNative.synthesizeToFile(
                libraryHandle,
                "Orca!",
                tempFileLocation,
                1.0f,
                77);
        assertNull(audioObject.getPcm());
        assertTrue(audioObject.getWordArray().length > 0);
        validateMetadata(audioObject.getWordArray(), 0);
        boolean deleted = tempFile.delete();
        assertTrue(deleted);
        tempFile.deleteOnExit();
    }

    @Test
    void streamSynthesize() throws Exception, OrcaException {
        libraryHandle = OrcaNative.init(accessKey, modelPath);
        long streamHandle = OrcaNative.streamOpen(libraryHandle, 1.0f, 77);
        short[] pcmOne = OrcaNative.streamSynthesize(streamHandle, "Orca!");
        assertTrue(pcmOne.length == 0);

        short[] pcmTwo = OrcaNative.streamFlush(streamHandle);
        assertTrue(pcmTwo.length > 0);

        OrcaNative.streamClose(streamHandle);
    }    
}
