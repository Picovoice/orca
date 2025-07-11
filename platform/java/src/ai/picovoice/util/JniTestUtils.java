package ai.picovoice.util;

import org.apache.commons.text.similarity.LevenshteinDistance;

import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import java.io.File;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class JniTestUtils {

    public static short[] readAudioFile(String testAudioPath) throws Exception {

        File testAudioFile = new File(testAudioPath);
        AudioInputStream audioInputStream = AudioSystem.getAudioInputStream(testAudioFile);

        int byteDepth = audioInputStream.getFormat().getFrameSize();
        int frameLength = (int) audioInputStream.getFrameLength();
        byte[] bytes = new byte[frameLength * byteDepth];
        short[] pcm = new short[frameLength];

        audioInputStream.read(bytes);
        ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(pcm);
        return pcm;
    }

    public static float getCharacterErrorRate(String transcript, String expectedTranscript) {
        return (float) LevenshteinDistance
                .getDefaultInstance()
                .apply(transcript, expectedTranscript) / (float) expectedTranscript.length();
    }
}
