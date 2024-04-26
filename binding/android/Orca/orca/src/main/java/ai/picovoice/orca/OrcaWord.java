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

public class OrcaWord {

    private final String word;
    private final float startSec;
    private final float endSec;
    private final OrcaPhoneme[] phonemeArray;

    /**
     * Constructor.
     *
     * @param word         Synthesized word.
     * @param startSec     Start time of the word in seconds.
     * @param endSec       End time of the word in seconds.
     * @param phonemeArray Synthesized phonemes and their associated metadata.
     */
    public OrcaWord(String word, float startSec, float endSec, OrcaPhoneme[] phonemeArray) {
        this.word = word;
        this.startSec = startSec;
        this.endSec = endSec;
        this.phonemeArray = phonemeArray;
    }

    /**
     * Getter for the synthesized word.
     *
     * @return Synthesized word.
     */
    public String getWord() {
        return word;
    }

    /**
     * Getter for the start time of the word in seconds.
     *
     * @return Start time of the word in seconds.
     */
    public float getStartSec() {
        return startSec;
    }

    /**
     * Getter for the end time of the word in seconds.
     *
     * @return End time of the word in seconds.
     */
    public float getEndSec() {
        return endSec;
    }

    /**
     * Getter for synthesized phonemes and their associated metadata.
     *
     * @return Synthesized phonemes and their associated metadata.
     */
    public OrcaPhoneme[] getPhonemeArray() {
        return phonemeArray;
    }
}
