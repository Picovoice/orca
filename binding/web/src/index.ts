import { Orca, OrcaStream } from './orca';
import { OrcaWorker, OrcaStreamWorker } from './orca_worker';

import {
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

import orcaWasm from '../lib/pv_orca.wasm';
import orcaWasmSimd from '../lib/pv_orca_simd.wasm';

Orca.setWasm(orcaWasm);
Orca.setWasmSimd(orcaWasmSimd);
OrcaWorker.setWasm(orcaWasm);
OrcaWorker.setWasmSimd(orcaWasmSimd);

export {
  Orca,
  OrcaStream,
  OrcaErrors,
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
