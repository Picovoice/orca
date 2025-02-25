import { defineConfig } from 'cypress';

export default defineConfig({
  env: {
    'NUM_TEST_ITERATIONS': 10,
    'PROC_PERFORMANCE_THRESHOLD_SEC': 10,
  },
  e2e: {
    supportFile: 'cypress/support/index.ts',
    defaultCommandTimeout: 60000,
    specPattern: 'test/*.test.{js,jsx,ts,tsx}',
    video: false,
    screenshotOnRunFailure: false,
  },
});
