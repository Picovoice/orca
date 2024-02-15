import { defineConfig } from 'cypress';

export default defineConfig({
  env: {
    'NUM_TEST_ITERATIONS': 10,
    'PROC_PERFORMANCE_THRESHOLD_SEC': 10,
  },
  e2e: {
    defaultCommandTimeout: 30000,
    supportFile: false,
    specPattern: 'test/*.test.{js,jsx,ts,tsx}',
    video: false,
    screenshotOnRunFailure: false,
  },
});
