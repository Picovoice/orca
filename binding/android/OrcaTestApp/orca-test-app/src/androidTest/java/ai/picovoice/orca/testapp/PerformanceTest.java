/*
    Copyright 2024 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is
    located in the "LICENSE" file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the
    License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
    express or implied. See the License for the specific language governing permissions and
    limitations under the License.
*/

package ai.picovoice.orca.testapp;

import static org.junit.Assert.assertTrue;

import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import ai.picovoice.orca.Orca;
import ai.picovoice.orca.OrcaSynthesizeParams;

@RunWith(Parameterized.class)
public class PerformanceTest extends BaseTest {
    @Parameterized.Parameter(value = 0)
    public String modelFile;

    @Parameterized.Parameters(name = "{0}")
    public static Collection<Object[]> initParameters() {
        List<Object[]> parameters = new ArrayList<>();
        for (String modelFile : getModelFiles()) {
            parameters.add(new Object[]{modelFile});
        }
        return parameters;
    }

    int numTestIterations = 30;

    @Before
    public void Setup() throws Exception {
        super.Setup();
        String iterationString = appContext.getString(R.string.numTestIterations);

        try {
            numTestIterations = Integer.parseInt(iterationString);
        } catch (NumberFormatException ignored) {
        }
    }

    @Test
    public void testProcPerformance() throws Exception {
        final String procThresholdString = appContext.getString(R.string.procPerformanceThresholdSec);
        Assume.assumeNotNull(procThresholdString);
        Assume.assumeFalse(procThresholdString.equals(""));

        final double procPerformanceThresholdSec = Double.parseDouble(procThresholdString);
        final String procSentence = testJson
                .getAsJsonObject("test_sentences")
                .get("text")
                .getAsString();
        final Orca orca = new Orca.Builder()
                .setAccessKey(accessKey)
                .setModelPath(modelFile)
                .build(appContext);

        long totalNSec = 0;
        for (int i = 0; i < numTestIterations + 1; i++) {
            long before = System.nanoTime();
            orca.synthesize(
                    procSentence,
                    new OrcaSynthesizeParams.Builder().build());
            long after = System.nanoTime();

            // throw away first run to account for cold start
            if (i > 0) {
                totalNSec += (after - before);
            }
        }
        orca.delete();

        double avgNSec = totalNSec / (double) numTestIterations;
        double avgSec = ((double) Math.round(avgNSec * 1e-6)) / 1000.0;
        assertTrue(
                String.format(
                        "Expected threshold (%.3fs), process took (%.3fs)",
                        procPerformanceThresholdSec,
                        avgSec),
                avgSec <= procPerformanceThresholdSec
        );
    }
}