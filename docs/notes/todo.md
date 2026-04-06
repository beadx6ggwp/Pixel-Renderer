## Maybe
有朝一日把底層實作

AMD Xilinx Artix-7 FPGA Vivado
- [A tiled FPGA GPU for real-time rasterization](https://github.com/raster-gpu/raster-i)
- [FPGA Implementation of a Simple 3D Graphics
Pipeline, DOI: 10.15598/aeee.v13i1.1125](https://pdfs.semanticscholar.org/515d/9dcafa9d78a39d867fce5747a7831e668451.pdf)
- [gplgpu](https://github.com/asicguy/gplgpu) 
- [从零开始制作一个属于你自己的GPU | 基于FPGA的图形加速器实现原理](https://zhuanlan.zhihu.com/p/714400366)
- [The Linux graphics stack in a nutshell 翻譯 & 筆記](https://mes0903.github.io/Linux/linux-graphic-stack)

- 成大 VLSI System Design (VSD), NCTU/NTHU ICLAB
- Project1: RISC-V CPU
- Project2: +AXI(BUS) w/ syn and cadence tool for verification 
- Project3: +Cache w/ syn, apr, tool verification
- Project4: +I/O interrupt DMA w/ syn, apr
- Final: +ASIC（Proposed by yourself)

- 上层C语言层面的软渲染器实现。
- 上层渲染器的运算瓶颈问题，为什么使用FPGA可以为CPU实现图形加速。
- Zynq7020 Soc的基础架构，我们的GPU架构模式
- 实现AXI总线，完成CPU-GPU之间的数据交互
- FIFO，GPU Info，Memcpy三个模块的工作原理
- 调用VDMA IP核，实现双缓存Framebuffer，编写驱动实现HDMI输出到屏幕
- 实现Render模块的第一部分，完成颜色纹理的AlphaBlend
- 实现Render模块的第二部分，完成三角图元的光栅化渲染
- 编写GPU驱动，软硬结合渲染图元、模型、实现一个粒子系统。