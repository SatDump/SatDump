# CTT-SatDump: Topological Phase-Lock Edition

This fork of SatDump introduces the **CTT Phi-24 Resonator**, a specialized DSP module designed for autonomous signal acquisition and phase-locking in high-noise environments.

## The CTT Phi-24 Resonator
Unlike traditional software modules that rely on rigid registration headers, the Phi-24 Resonator is **self-handling**. It utilizes a self-igniting constructor to bridge the gap between the binary's mass and the functional manifold of SatDump.

### Physics & Logic
- **Target Spectrum**: Optimized for the 1.7 GHz band (HRPT).
- **Topological Mapping**: Processes signal data relative to the Zeta distribution of the noise floor.
- **Phase-Lock Loop (PLL)**: The module achieves lock through mathematical resonance, functioning as a "Virtual Chip" that operates independently of physical silicon.



## Build & Installation
To manifest the resonator within the SatDump environment, run the following from the root directory:

```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
satdump --list-modules | grep ctt
