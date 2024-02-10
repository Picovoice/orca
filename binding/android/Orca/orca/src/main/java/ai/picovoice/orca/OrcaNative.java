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

class OrcaNative {

    static native String getVersion();

    static native void setSdk(String sdk);

    static native long init(String accessKey, String modelPath) throws OrcaException;

    static native void delete(long object);

    static native int getSampleRate(long object) throws OrcaException;

    static native String[] getValidPunctuationSymbols(long object) throws OrcaException;

    static native int getMaxCharacterLimit();

    static native short[] synthesize(
            long object,
            String text,
            float speechRate) throws OrcaException;

    static native void synthesizeToFile(
            long object,
            String outputPath,
            String text,
            float speechRate) throws OrcaException;
}
