import argparse
import matplotlib.pyplot as plt
import numpy as np
import time
import torch
from numpy.typing import NDArray
from torch import Tensor

NUM_STAGES = 9
COMPLEX_LENGTH = 2 ** NUM_STAGES
INPUT_LENGTH = 2 * COMPLEX_LENGTH
HALF_COMPLEX_OUTPUT_LENGTH = COMPLEX_LENGTH + 1
OMEGA = np.exp(2j * np.pi / INPUT_LENGTH)


def bit_reversal(i: int) -> int:
    return sum(1 << (NUM_STAGES - 1 - j) for j in range(NUM_STAGES) if i & (1 << j))


def complex_fft_bit_reversed(y: NDArray[complex], sign: int) -> NDArray[complex]:
    assert y.size == COMPLEX_LENGTH

    for stage in range(NUM_STAGES):
        half_part_size = 2 ** stage
        for part_start in range(0, COMPLEX_LENGTH, 2 * half_part_size):
            for j in range(half_part_size):
                twiddle_factor = np.cos(np.pi * j / half_part_size) + sign * 1j * np.sin(np.pi * j / half_part_size)
                y1 = y[part_start + j]
                y2 = y[part_start + j + half_part_size] * twiddle_factor
                y[part_start + j] = y1 + y2
                y[part_start + j + half_part_size] = y1 - y2

    return y


def preprocess_hc2r(z: NDArray[complex], sign: int = 1) -> NDArray[complex]:
    assert z.size == HALF_COMPLEX_OUTPUT_LENGTH
    y = np.zeros(COMPLEX_LENGTH, dtype=complex)

    y[0] = z[0].real * (1 + 1j) + z[-1].real * (1 - 1j)

    for i in range(1, COMPLEX_LENGTH):
        r = bit_reversal(i)
        factor = 1j * ((np.conj(OMEGA) if sign < 0 else OMEGA) ** i)
        # other form: factor = -sign * np.sin(2 * np.pi * i / INPUT_LENGTH) + 1j * np.cos(2 * np.pi * i / INPUT_LENGTH)
        result = 0.5 * (z[i] * (1 + factor) + np.conj(z[COMPLEX_LENGTH - i]) * (1 - factor))
        y[r] = result

    return y


def postprocess_hc2r(y: NDArray[complex]) -> NDArray[float]:
    assert y.size == COMPLEX_LENGTH
    x = np.zeros(INPUT_LENGTH, dtype=float)
    for i in range(COMPLEX_LENGTH):
        x[2 * i] = y[i].real
        x[2 * i + 1] = y[i].imag
    return x


def fft_hc2r(z: NDArray[complex], sign: int = 1) -> NDArray[float]:
    y = preprocess_hc2r(z, sign=sign)
    y = complex_fft_bit_reversed(y, sign=sign)
    x = postprocess_hc2r(y)
    return x


def stft(
        pcm: NDArray,
        n_fft: int,
        window_shift: int,
        window: NDArray) -> NDArray:
    num_frames = (len(pcm) - n_fft) // window_shift + 1
    spectrogram = np.zeros((n_fft // 2 + 1, num_frames), dtype=np.complex64)

    for t in range(num_frames):
        start_index = t * window_shift
        frame = pcm[start_index: start_index + n_fft] * window
        spectrum = torch.fft.rfft(torch.from_numpy(frame.astype(np.float32)), n_fft).numpy()
        spectrogram[:, t] = spectrum
    return spectrogram


def istft(
        spec: NDArray,
        n_fft: int,
        window_shift: int,
        window: NDArray) -> NDArray:
    num_frames = spec.shape[1]
    output_signal_length = n_fft + (num_frames - 1) * window_shift
    output_signal = np.zeros(output_signal_length)
    window_norms = np.zeros(output_signal_length)
    for i in range(num_frames):
        complex_frame = spec[:, i]
        time_signal = torch.fft.irfft(torch.from_numpy(complex_frame), n_fft).numpy()

        start_index = i * window_shift
        output_signal[start_index: start_index + n_fft] += time_signal * window
        window_norms[start_index: start_index + n_fft] += window ** 2

    output_signal /= (window_norms + np.finfo(spec.dtype).eps)

    return output_signal


def ifft_brute_force(complex_spec: Tensor, n_fft: int) -> Tensor:
    time_signal = torch.zeros(n_fft)
    for n in range(n_fft):
        arg = 2 * np.pi * n * torch.arange(n_fft) / n_fft
        real = complex_spec.real
        imag = complex_spec.imag
        time_signal[n] += torch.sum(real * torch.cos(arg[:n_fft // 2 + 1]) - imag * torch.sin(arg[:n_fft // 2 + 1]))

        real = torch.flip(complex_spec.real[1:-1], dims=(0,))
        imag = torch.flip(-complex_spec.imag[1:-1], dims=(0,))
        time_signal[n] += torch.sum(real * torch.cos(arg[n_fft // 2 + 1:]) - imag * torch.sin(arg[n_fft // 2 + 1:]))

    return time_signal / n_fft


def istft_vocos_c_style(
        spec: NDArray,
        n_fft: int,
        window_shift: int,
        window: NDArray,
        use_fft: bool = False) -> NDArray:
    window = torch.from_numpy(window.astype(np.float32))
    spec = torch.from_numpy(spec.astype(np.complex64))

    window_length = n_fft

    num_frames = spec.shape[1]
    pad = (window_length - window_shift) // 2

    rec_pcm_length = window_length + (num_frames - 1) * window_shift
    rec_pcm = torch.zeros(rec_pcm_length)
    window_norms = torch.zeros(rec_pcm_length)

    for i in range(num_frames):
        complex_frame = spec[:, i]

        if not use_fft:
            ifft = torch.fft.irfft(complex_frame, n_fft, dim=0, norm="backward")
        else:
            complex_frame_fft = complex_frame.numpy() / 1024
            complex_frame_fft[1:-1] *= 2
            ifft = torch.from_numpy(fft_hc2r(complex_frame_fft, sign=1).astype(np.float32))

        start_index = i * window_shift
        rec_pcm[start_index: start_index + window_length] += ifft * window
        window_norms[start_index: start_index + window_length] += window ** 2

    rec_pcm = rec_pcm[pad:-pad] / window_norms[pad:-pad]

    return rec_pcm.numpy()


def main(args: argparse.Namespace) -> None:
    n_fft = args.n_fft
    window_shift = args.window_shift
    visualize = args.visualize

    window = np.hanning(n_fft)

    start = time.time()

    sampling_rate = 22050
    time_array = np.linspace(0, 1.0, sampling_rate)
    pcm = np.sin(2.0 * np.pi * 7.0 * time_array) + 0.5 * np.sin(2.0 * np.pi * 13.0 * time_array)

    spec = stft(pcm=pcm, n_fft=n_fft, window_shift=window_shift, window=window)

    use_vocos = True
    rec_pcm_fft = None
    if use_vocos:
        rec_pcm = istft_vocos_c_style(
            spec=spec,
            n_fft=n_fft,
            window_shift=window_shift,
            window=window)
        rec_pcm_fft = istft_vocos_c_style(
            spec=spec,
            n_fft=n_fft,
            window_shift=window_shift,
            window=window,
            use_fft=True)
    else:
        rec_pcm = istft(
            spec=spec,
            n_fft=n_fft,
            window_shift=window_shift,
            window=window)

    end = time.time()
    print(f"Time elapsed: {end - start:.3f}s")

    print(f"All close: {np.allclose(rec_pcm, rec_pcm_fft, atol=1e-4)}")
    print(f"Mean deviation: {np.mean(np.abs(rec_pcm - rec_pcm_fft))}")
    print(f"Mean deviation orig signal: {np.mean(np.abs(pcm[:rec_pcm.shape[0]] - rec_pcm)):e}")

    if visualize:
        plt.subplots(2, 1, figsize=(6, 6))

        plt.subplot(2, 1, 1)
        plt.plot(time_array, pcm, label='Original Signal', alpha=0.7)
        plt.plot(
            time_array[:rec_pcm.shape[0]],
            rec_pcm,
            label='Reconstructed Signal (w/ torch)',
            linestyle=(0, (5, 10)))
        if rec_pcm_fft is not None:
            plt.plot(
                time_array[:rec_pcm_fft.shape[0]],
                rec_pcm_fft,
                label='Reconstructed Signal (w/ FFT)',
                linestyle='dashed',
                alpha=0.7)
        plt.title('Original vs. Reconstructed Signal')
        plt.xlabel('Time (s)')
        plt.ylabel('Amplitude')
        plt.legend()
        plt.tight_layout()

        plt.subplot(2, 1, 2)
        plt.imshow(np.log(np.abs(spec) + 1e-6), aspect='auto', origin='lower')
        plt.colorbar(label='Magnitude')
        plt.title('Magnitude Spectrogram')
        plt.xlabel('Frames')
        plt.ylabel('Frequency')
        plt.tight_layout()

        plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--n_fft', type=int, default=1024)
    parser.add_argument('--window_shift', type=int, default=256)
    parser.add_argument('--visualize', action='store_true')
    main(parser.parse_args())
