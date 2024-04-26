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

public class OrcaAudio {

    private final short[] pcm;
    private final OrcaWord[] wordArray;

    /**
     * Constructor.
     *
     * @param pcm       Synthesized audio.
     * @param wordArray Synthesized words and their associated metadata.
     */
    public OrcaAudio(short[] pcm, OrcaWord[] wordArray) {
        this.pcm = pcm;
        this.wordArray = wordArray;
    }

    /**
     * Getter for the synthesized audio.
     *
     * @return Synthesized audio.
     */
    public short[] getPcm() {
        return pcm;
    }

    /**
     * Getter for synthesized words and their associated metadata.
     *
     * @return Synthesized words and their associated metadata.
     */
    public OrcaWord[] getWordArray() {
        return wordArray;
    }
}
