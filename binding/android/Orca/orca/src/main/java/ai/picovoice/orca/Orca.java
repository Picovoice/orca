/*
    Copyright 2024 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is
    located in the "LICENSE" file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the
    License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
    express or implied. See the License for the specific language governing permissions and
    limitations under the License.
*/

package ai.picovoice.orca;

import android.content.Context;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Android binding for Orca Text-to-Speech engine.
 */
public class Orca {

    private static String _sdk = "android";

    static {
        System.loadLibrary("pv_orca");
    }

    private long handle;

    /**
     * Constructor.
     *
     * @param accessKey AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)
     * @param modelPath Absolute path to the file containing Orca model parameters.
     * @throws OrcaException if there is an error while initializing Orca.
     */
    private Orca(String accessKey, String modelPath) throws OrcaException {
        OrcaNative.setSdk(Orca._sdk);
        handle = OrcaNative.init(
                accessKey,
                modelPath);
    }

    public static void setSdk(String sdk) {
        Orca._sdk = sdk;
    }

    private static String extractResource(Context context, InputStream srcFileStream, String dstFilename) throws IOException {
        InputStream is = new BufferedInputStream(
                srcFileStream,
                256);
        OutputStream os = new BufferedOutputStream(
                context.openFileOutput(dstFilename, Context.MODE_PRIVATE),
                256);
        int r;
        while ((r = is.read()) != -1) {
            os.write(r);
        }
        os.flush();

        is.close();
        os.close();
        return new File(
                context.getFilesDir(),
                dstFilename).getAbsolutePath();
    }


    /**
     * Releases resources acquired by Orca.
     */
    public void delete() {
        if (handle != 0) {
            OrcaNative.delete(handle);
            handle = 0;
        }
    }

    /**
     * @param text
     * @param params
     * @return
     * @throws OrcaException if there is an error while synthesizing audio.
     */
    public short[] synthesize(String text, OrcaSynthesizeParams params) throws OrcaException {
        if (handle == 0) {
            throw new OrcaInvalidStateException(
                    "Attempted to call Orca synthesize after delete."
            );
        }

        return OrcaNative.synthesize(
                handle,
                text,
                params.getSpeechRate());
    }

    /**
     * @param outputPath
     * @param text
     * @param params
     * @return
     * @throws OrcaException if there is an error while synthesizing audio to file.
     */
    public void synthesizeToFile(String outputPath, String text, OrcaSynthesizeParams params) throws OrcaException {
        if (handle == 0) {
            throw new OrcaInvalidStateException(
                    "Attempted to call Orca synthesize after delete."
            );
        }

        OrcaNative.synthesizeToFile(
                handle,
                outputPath,
                text,
                params.getSpeechRate());
    }

    /**
     * Getter for version.
     *
     * @return Version.
     */
    public String getVersion() {
        return OrcaNative.getVersion();
    }

    /**
     * Getter for the maximum number of characters that can be synthesized at once.
     *
     * @return The maximum number of characters that can be synthesized at once.
     */
    public int getMaxCharacterLimit() {
        return OrcaNative.getMaxCharacterLimit();
    }

    /**
     * Getter for the audio sampling rate of the audio produced by Orca.
     *
     * @return Audio sampling rate of the audio produced by Orca.
     */
    public int getSampleRate() throws OrcaException {
        if (handle == 0) {
            throw new OrcaInvalidStateException(
                    "Attempted to call Orca getSampleRate after delete."
            );
        }

        return OrcaNative.getSampleRate(handle);
    }

    /**
     * Getter for the set of punctuation symbols that are accepted as input to Orca synthesize functions.
     *
     * @return Array of punctuation symbols that are accepted as input to Orca synthesize functions.
     */
    public String[] getValidPunctuationSymbols() throws OrcaException {
        if (handle == 0) {
            throw new OrcaInvalidStateException(
                    "Attempted to call Orca getValidPunctuationSymbols after delete."
            );
        }
        return OrcaNative.getValidPunctuationSymbols(handle);
    }

    /**
     * Builder for creating instance of Orca.
     */
    public static class Builder {

        private String accessKey = null;
        private String modelPath = null;

        public Builder setAccessKey(String accessKey) {
            this.accessKey = accessKey;
            return this;
        }

        public Builder setModelPath(String modelPath) {
            this.modelPath = modelPath;
            return this;
        }


        /**
         * Validates properties and creates an instance of the Orca Speech-to-Text engine.
         *
         * @return An instance Orca Speech-to-Text engine
         * @throws OrcaException if there is an error while initializing Orca.
         */
        public Orca build(Context context) throws OrcaException {
            if (accessKey == null || this.accessKey.equals("")) {
                throw new OrcaInvalidArgumentException("No AccessKey was provided to Orca");
            }

            if (modelPath == null) {
                throw new OrcaInvalidArgumentException("ModelPath must not be null");
            } else {
                File modelFile = new File(modelPath);
                String modelFilename = modelFile.getName();
                if (!modelFile.exists() && !modelFilename.equals("")) {
                    try {
                        modelPath = extractResource(
                                context,
                                context.getAssets().open(modelPath),
                                modelFilename);
                    } catch (IOException ex) {
                        throw new OrcaIOException(ex);
                    }
                }
            }

            return new Orca(
                    accessKey,
                    modelPath);
        }
    }

}
