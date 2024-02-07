import {Orca} from './orca';
import {OrcaWorker} from './orca_worker';

import {
  OrcaModel,
  OrcaOptions,
  OrcaSpeech,
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

import orcaWasm from '../lib/pv_orca.wasm';
import orcaWasmSimd from '../lib/pv_orca_simd.wasm';

Orca.setWasm(orcaWasm);
Orca.setWasmSimd(orcaWasmSimd);
OrcaWorker.setWasm(orcaWasm);
OrcaWorker.setWasmSimd(orcaWasmSimd);

export {
  Orca,
  OrcaModel,
  OrcaOptions,
  OrcaSpeech,
  OrcaWorker,
  OrcaWorkerInitRequest,
  OrcaWorkerSynthesizeRequest,
  OrcaWorkerReleaseRequest,
  OrcaWorkerRequest,
  OrcaWorkerInitResponse,
  OrcaWorkerSynthesizeResponse,
  OrcaWorkerReleaseResponse,
  OrcaWorkerFailureResponse,
  OrcaWorkerResponse,
  OrcaErrors,
};
