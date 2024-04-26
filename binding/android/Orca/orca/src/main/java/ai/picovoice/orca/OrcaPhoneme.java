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

public class OrcaPhoneme {

    private final String phoneme;
    private final float startSec;
    private final float endSec;

    /**
     * Constructor.
     *
     * @param phoneme  Synthesized phoneme.
     * @param startSec Start time of the phoneme in seconds.
     * @param endSec   End time of the phoneme in seconds.
     */
    public OrcaPhoneme(String phoneme, float startSec, float endSec) {
        this.phoneme = phoneme;
        this.startSec = startSec;
        this.endSec = endSec;
    }

    /**
     * Getter for the synthesized phoneme.
     *
     * @return Synthesized phoneme.
     */
    public String getPhoneme() {
        return phoneme;
    }

    /**
     * Getter for the start time of the phoneme in seconds.
     *
     * @return Start time of the phoneme in seconds.
     */
    public float getStartSec() {
        return startSec;
    }

    /**
     * Getter for the end time of the phoneme in seconds.
     *
     * @return End time of the phoneme in seconds.
     */
    public float getEndSec() {
        return endSec;
    }
}
