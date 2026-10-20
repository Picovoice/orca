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
    chromeWebSecurity: false, // TODO: look into this
    setupNodeEvents(on, config) {
      on('before:browser:launch', (browser, launchOptions) => {
        if (browser.name === 'chrome') {
          launchOptions.args.push('--enable-features=SharedArrayBuffer');
          launchOptions.args.push('--log-net-log=/tmp/netlog.json');

          // Cypress already starts Chrome with a debugging port; find which one
          const portArg = launchOptions.args.find((a: string) =>
            a.startsWith('--remote-debugging-port=')
          );
          if (portArg) {
            forwardBrowserConsole(Number(portArg.split('=')[1])); // not awaited on purpose
          } else {
            console.log('[cdp] no --remote-debugging-port found in Chrome args');
          }
        }

        return launchOptions;
      });

      on('task', {
        log(message) {
          console.log(message);
          return null;
        },
      });

      return config;
    },
  },
});

// Turn CDP's description of console arguments into readable text
function formatArgs(args: any[]): string {
  return args
    .map((a) => {
      if (a.value !== undefined) {
        return typeof a.value === 'string' ? a.value : JSON.stringify(a.value);
      }
      return a.description || a.unserializableValue || a.type;
    })
    .join(' ');
}

async function forwardBrowserConsole(port: number) {
  const WS = (globalThis as any).WebSocket; // built into Node 22+
  if (!WS) {
    console.log(`[cdp] no built-in WebSocket in Node ${process.version} (need 22+)`);
    return;
  }

  // Chrome needs a moment to start, so retry for up to ~10 seconds
  for (let attempt = 0; attempt < 50; attempt++) {
    try {
      const res = await fetch(`http://127.0.0.1:${port}/json/list`);
      const targets: any[] = await res.json();
      const page = targets.find((t) => t.type === 'page');
      if (!page) throw new Error('no page yet');

      const ws = new WS(page.webSocketDebuggerUrl);
      await new Promise((resolve, reject) => {
        ws.onopen = resolve;
        ws.onerror = reject;
      });

      let nextId = 1;
      const send = (method: string, params: object = {}, sessionId?: string) =>
        ws.send(JSON.stringify({ id: nextId++, method, params, sessionId }));

      const enableAll = (sessionId?: string) => {
        send('Runtime.enable', {}, sessionId);
        send('Log.enable', {}, sessionId);
        send('Network.enable', {}, sessionId);
      };

      const requests = new Map<string, string>();
      const reqKey = (m: any) => (m.sessionId || '') + ':' + m.params.requestId;

      ws.onmessage = (event: any) => {
        const msg = JSON.parse(String(event.data));

        if (msg.method === 'Runtime.consoleAPICalled') {
          const where = msg.sessionId ? 'worker/frame' : 'page';
          console.log(`[${where} console.${msg.params.type}] ${formatArgs(msg.params.args)}`);
        } else if (msg.method === 'Runtime.exceptionThrown') {
          const d = msg.params.exceptionDetails;
          console.log(`[uncaught] ${(d.exception && d.exception.description) || d.text}`);
        } else if (msg.method === 'Log.entryAdded') {
          const e = msg.params.entry;
          console.log(`[browser ${e.source} ${e.level}] ${e.text}${e.url ? ' (' + e.url + ')' : ''}`);
        } else if (msg.method === 'Network.requestWillBeSent') {
          const r = msg.params.request;
          requests.set(reqKey(msg), `${r.method} ${r.url}`);
          console.log(`\tmore info ${String(event.data)}`)
        } else if (msg.method === 'Network.loadingFinished') {
          requests.delete(reqKey(msg));
        } else if (msg.method === 'Network.loadingFailed') {
          const p = msg.params;
          const details = [
            p.errorText,
            p.canceled ? 'canceled' : '',
            p.blockedReason ? `blocked: ${p.blockedReason}` : '',
            p.corsErrorStatus
              ? `CORS: ${p.corsErrorStatus.corsError} ${p.corsErrorStatus.failedParameter}`
              : '',
          ]
            .filter(Boolean)
            .join(', ');
          console.log(`[network failed] ${requests.get(reqKey(msg)) || p.requestId} -> ${details}`);
          console.log(`\tmore info2 ${String(event.data)}`)
          requests.delete(reqKey(msg));
        } else if (msg.method === 'Target.attachedToTarget') {
          enableAll(msg.params.sessionId);
        }
      };

      enableAll();
      send('Target.setAutoAttach', {
        autoAttach: true,
        waitForDebuggerOnStart: false,
        flatten: true,
      });

      console.log(`[cdp] forwarding browser console (port ${port})`);
      return;
    } catch {
      await new Promise((r) => setTimeout(r, 200));
    }
  }
  console.log('[cdp] could not connect to Chrome');
}
