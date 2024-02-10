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
 *
 */
public class OrcaSynthesizeParams {

    private final float speechRate;

    /**
     * Constructor.
     */
    private OrcaSynthesizeParams(float speechRate) {
        this.speechRate = speechRate;
    }

    public float getSpeechRate() {
        return this.speechRate;
    }

    /**
     * Builder for creating instance of OrcaSynthesizeParams.
     */
    public static class Builder {

        private float speechRate = 1.0f;

        public Builder setSpeechRate(float speechRate) {
            this.speechRate = speechRate;
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

            return new OrcaSynthesizeParams(speechRate);
        }
    }
}
