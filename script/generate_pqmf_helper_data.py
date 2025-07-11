import argparse

import numpy as np
from scipy.signal import firwin


def float_to_q_int16(x: float, q: int = 10) -> int:
    scale = float(1 << q)
    return int(max(min(round(x * scale), 32767), -32768))


def main(args: argparse.Namespace) -> None:
    num_taps = args.num_taps
    num_subbands = args.num_subbands
    freq_cutoff_ratio = args.freq_cutoff_ratio
    beta_kaiser = args.beta_kaiser

    proto_filter = firwin(numtaps=num_taps + 1, cutoff=freq_cutoff_ratio, window=('kaiser', beta_kaiser))
    synthesis_filter = np.zeros((num_subbands, len(proto_filter)), dtype=np.float32)

    for k in range(num_subbands):
        constant_factor = \
            (2 * k + 1) * (np.pi / (2 * num_subbands)) * \
            (np.arange(num_taps + 1) - ((num_taps - 1) / 2))
        phase = (-1)**k * np.pi / 4
        synthesis_filter[k] = 2 * proto_filter * np.cos(constant_factor - phase)

    synthesis_filter = synthesis_filter.flatten()
    num_elements = len(synthesis_filter)
    print(f"static const float PQMF_SYNTHESIS_FILTER[{num_elements}] = ", end="")
    print("{")
    print("\t", end="")
    for i in range(num_elements):
        if i != 0 and i % 8 == 0:
            print("\n\t", end="")
        print(f"{synthesis_filter[i]}f{', ' if i + 1 < num_elements else '};'}", end="")
    print("")

    print(f"static const int16_t PQMF_SYNTHESIS_FILTER[{num_elements}] = ", end="")
    print("{")
    print("\t", end="")
    m = 0
    for i in range(num_elements):
        if i != 0 and i % 16 == 0:
            print("\n\t", end="")
        print(f"{float_to_q_int16(synthesis_filter[i], q=16)}{', ' if i + 1 < num_elements else '};'}", end="")
    print("")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--num-taps', type=int, default=62)
    parser.add_argument('--num-subbands', type=int, default=4)
    parser.add_argument('--freq-cutoff-ratio', type=float, default=0.142)
    parser.add_argument('--beta-kaiser', type=float, default=9.0)
    main(parser.parse_args())
