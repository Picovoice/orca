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

/**
 * A class that exposes several properties that can control the audio synthesized by Orca.
 */
public class OrcaSynthesizeParams {

    private final float speechRate;
    private final long randomState;

    /**
     * Constructor.
     */
    private OrcaSynthesizeParams(float speechRate, long randomState) {
        this.speechRate = speechRate;
        this.randomState = randomState;
    }

    /**
     * Getter for the speech rate (i.e. the pace of the synthesized speech).
     *
     * @return Speech Rate.
     */
    public float getSpeechRate() {
        return this.speechRate;
    }

    /**
     * Getter for the random state (i.e. the random state for the synthesized speech).
     *
     * @return Random State.
     */
    public long getRandomState() {
        return this.randomState;
    }

    /**
     * Builder for creating instance of OrcaSynthesizeParams.
     */
    public static class Builder {

        private float speechRate = 1.0f;
        private long randomState = -1;

        /**
         * Sets the speech rate.
         *
         * @param speechRate The pace of the synthesized speech. Valid values are within [0.7, 1.3].
         * @return Modified builder object.
         */
        public Builder setSpeechRate(float speechRate) {
            this.speechRate = speechRate;
            return this;
        }

        /**
         * Sets the random state.
         *
         * @param randomState The random state for the synthesized speech.
         * @return Modified builder object.
         */
        public Builder setRandomState(long randomState) {
            this.randomState = randomState;
            return this;
        }

        /**
         * Validates properties and creates an instance of OrcaSynthesizeParams.
         *
         * @return An instance of OrcaSynthesizeParams
         * @throws OrcaInvalidArgumentException if there is an invalid parameter.
         */
        public OrcaSynthesizeParams build() throws OrcaInvalidArgumentException {
            if (speechRate < 0.7f || speechRate > 1.3f) {
                throw new OrcaInvalidArgumentException(
                        "Speech rate must be within [0.7, 1.3]"
                );
            }

            return new OrcaSynthesizeParams(speechRate, randomState);
        }
    }
}
