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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import ai.picovoice.orca.Orca;
import ai.picovoice.orca.OrcaException;

import androidx.test.ext.junit.runners.AndroidJUnit4;

@RunWith(AndroidJUnit4.class)
public class StandardTests extends BaseTest {

    String[] modelFiles;

    @Before
    public void Setup() throws Exception {
        super.Setup();
        modelFiles = getModelFiles();
    }

    @Test
    public void testErrorStack() {
        String[] error = {};
        try {
            new Orca.Builder()
                    .setAccessKey("invalid")
                    .setModelPath(modelFiles[0])
                    .build(appContext);
        } catch (OrcaException e) {
            error = e.getMessageStack();
        }

        assertTrue(0 < error.length);
        assertTrue(error.length <= 8);

        try {
            new Orca.Builder()
                    .setAccessKey("invalid")
                    .setModelPath(modelFiles[0])
                    .build(appContext);
        } catch (OrcaException e) {
            for (int i = 0; i < error.length; i++) {
                assertEquals(e.getMessageStack()[i], error[i]);
            }
        }
    }
}
