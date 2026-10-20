import './commands';

declare global {
  namespace Cypress {
    interface Chainable {
      getFramesFromFile(path: string): Chainable<Int16Array>;
    }
  }

  var appLogs: string[];
}

if (!globalThis.appLogs) {
  globalThis.appLogs = [];
}
export const appLogs: string[] = globalThis.appLogs;

function toText(arg: unknown): string {
  if (typeof arg === 'string') return arg
  try {
    return JSON.stringify(arg)
  } catch {
    return String(arg)
  }
}

// Runs every time the app's page loads, before your app code runs
Cypress.on('window:before:load', (win) => {
  const originalLog = win.console.log
  win.console.log = (...args: unknown[]) => {
    appLogs.push(args.map(toText).join(' '))
    originalLog.apply(win.console, args) // still log to the browser console as normal
  }
})

afterEach(() => {
  if (appLogs.length) {
    cy.task('log', appLogs.join('\n'))
  }
  appLogs.length = 0;
})

export {};