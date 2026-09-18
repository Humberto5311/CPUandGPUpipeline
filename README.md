High-Performance Pipeline
A high-performance asynchronous pipeline designed for game engines, focused on enhancing parallel processing between the GPU and CPU.

 Technologies Used
Modern C++ (Advanced use of threads and lock-free atomic operations)
OpenCL (Hardware-accelerated computing for adaptive Ray Tracing)

 Features
Producer-Consumer Architecture: Dedicated CPU threads calculate geometry in parallel with rendering.
Smart Fallback: Automatic hardware detection (runs on Nvidia/AMD/Intel GPUs or activates high-performance CPU mode).
Safe Memory Management: Fast batch data transfer to VRAM/RAM without memory leaks.

 Licensed under the Mozilla Public License 2.0 by NEXCore (2026).

## Disclaimer

This software is provided "as is", without warranty of any kind, express or implied. In no event shall the author be liable for any claim, damages, data loss, or hardware failure arising from the use of this code. The use of this software is at your own risk.

