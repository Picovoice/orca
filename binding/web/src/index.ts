import { Orca, OrcaStream } from './orca';
import { OrcaWorker, OrcaStreamWorker } from './orca_worker';

import {
  OrcaOptions,
  OrcaModel,
  OrcaSynthesizeParams,
  OrcaPhoneme,
  OrcaAlignment,
  OrcaSynthesizeResult,
  OrcaWorkerInitRequest,
  OrcaWorkerSynthesizeRequest,
  OrcaWorkerReleaseRequest,
  OrcaWorkerRequest,
  OrcaWorkerInitResponse,
  OrcaWorkerSynthesizeResponse,
  OrcaWorkerReleaseResponse,
  OrcaWorkerFailureResponse,
  OrcaWorkerResponse,
} from './types';

import * as OrcaErrors from './orca_errors';

import orcaWasmSimd from './lib/pv_orca_simd.wasm';
import orcaWasmSimdLib from './lib/pv_orca_simd.txt';
import orcaWasmPThread from './lib/pv_orca_pthread.wasm';
import orcaWasmPThreadLib from './lib/pv_orca_pthread.txt';

Orca.setWasmSimd(orcaWasmSimd);
Orca.setWasmSimdLib(orcaWasmSimdLib);
Orca.setWasmPThread(orcaWasmPThread);
Orca.setWasmPThreadLib(orcaWasmPThreadLib);
OrcaWorker.setWasmSimd(orcaWasmSimd);
OrcaWorker.setWasmSimdLib(orcaWasmSimdLib);
OrcaWorker.setWasmPThread(orcaWasmPThread);
OrcaWorker.setWasmPThreadLib(orcaWasmPThreadLib);

export {
  Orca,
  OrcaStream,
  OrcaErrors,
  OrcaOptions,
  OrcaModel,
  OrcaSynthesizeParams,
  OrcaPhoneme,
  OrcaAlignment,
  OrcaSynthesizeResult,
  OrcaWorker,
  OrcaStreamWorker,
  OrcaWorkerInitRequest,
  OrcaWorkerSynthesizeRequest,
  OrcaWorkerReleaseRequest,
  OrcaWorkerRequest,
  OrcaWorkerInitResponse,
  OrcaWorkerSynthesizeResponse,
  OrcaWorkerReleaseResponse,
  OrcaWorkerFailureResponse,
  OrcaWorkerResponse,
};
