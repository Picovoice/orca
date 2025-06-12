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

import orcaWasm from './lib/pv_orca.wasm';
import orcaWasmLib from './lib/pv_orca.txt';
import orcaWasmSimd from './lib/pv_orca_simd.wasm';
import orcaWasmSimdLib from './lib/pv_orca_simd.txt';

Orca.setWasm(orcaWasm);
Orca.setWasmLib(orcaWasmLib);
Orca.setWasmSimd(orcaWasmSimd);
Orca.setWasmSimdLib(orcaWasmSimdLib);
OrcaWorker.setWasm(orcaWasm);
OrcaWorker.setWasmLib(orcaWasmLib);
OrcaWorker.setWasmSimd(orcaWasmSimd);
OrcaWorker.setWasmSimdLib(orcaWasmSimdLib);

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
