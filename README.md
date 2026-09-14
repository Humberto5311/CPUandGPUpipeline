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
