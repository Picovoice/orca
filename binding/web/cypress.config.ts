import { defineConfig } from 'cypress';

export default defineConfig({
  env: {
    'NUM_TEST_ITERATIONS': 20,
    'INIT_PERFORMANCE_THRESHOLD_SEC': 3,
    'PROC_PERFORMANCE_THRESHOLD_SEC': 5,
  },
  e2e: {
    defaultCommandTimeout: 30000,
    supportFile: false,
    specPattern: 'test/*.test.{js,jsx,ts,tsx}',
    video: false,
    screenshotOnRunFailure: false,
  },
});
