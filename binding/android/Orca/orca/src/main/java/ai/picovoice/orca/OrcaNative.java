/*
    Copyright 2024-2025 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is
    located in the "LICENSE" file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the
    License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
    express or implied. See the License for the specific language governing permissions and
    limitations under the License.
*/

package ai.picovoice.orca;

class OrcaNative {

    static native String getVersion();

    static native void setSdk(String sdk);

    static native long init(
            String accessKey,
            String modelPath,
            String device) throws OrcaException;

    static native void delete(long object);

    static native int getSampleRate(long object) throws OrcaException;

    static native String[] getValidCharacters(long object) throws OrcaException;

    static native int getMaxCharacterLimit(long object) throws OrcaException;

    static native String[] listHardwareDevices() throws OrcaException;

    static native OrcaAudio synthesize(
            long object,
            String text,
            float speechRate,
            long randomState) throws OrcaException;

    static native OrcaAudio synthesizeToFile(
            long object,
            String text,
            String outputPath,
            float speechRate,
            long randomState) throws OrcaException;

    static native long streamOpen(
            long object,
            float speechRate,
            long randomState) throws OrcaException;

    static native short[] streamSynthesize(
            long object,
            String text) throws OrcaException;

    static native short[] streamFlush(long object) throws OrcaException;

    static native void streamClose(long object);
}
