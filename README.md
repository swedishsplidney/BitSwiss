# **BitSwiss**

[![CMake](https://shields.io)](https://cmake.org)
[![Dear ImGui](https://shields.io)](https://github.com/ocornut/imgui)
[![curl](https://shields.io)](https://curl.se)

a lightweight, cross-platform, open source C++/Dear ImGui utility to curate, download, and provision massive offline archives.

this tool allows users to select from a massive ledger of distinct historical, scientific and technical archives,
and have them automatically be downloaded onto any removable storage device.

---

## system architecture

the application decouples data definitions from user configuration management to guarantee mathematically sound allocation and zero-copy memory overhead

* **master data list:** explicitly registers a large library of global assets once. features live asynchronous metadata queries over http to determine exact byte counts on startup without locking the render frame loop.
* **preconfigured tiers:** employs preconfigured data arrays mapping directly to the master data list to provide instant presets tailored for different storage sizes, areas of focus, and more.
* **hardware mapping engine:** intercepts system volume layers to evaluate target file blocks against actual target parameters, automatically enforcing overload guards before writing data.

---

## contents and datasets

the program currently coordinates a comprehensive knowledge and information ledger across multiple domains:

the data includes but is not limited to:
* **the Wikipedia spectrum:** features three distinct fully searchable file sizes (full with pictures, text only, headers and introductions only) containing every Wikipedia entry.
* **the stackexchange matrix:** integrates the entirety of Stack Overflow alongside niche engineering, technology, science, math, literature, art, and more sister forums.
* **massive public domain literature packages:** houses the entirety of Project Gutenberg which includes many texts from the Library of Congress, and thousands of other public domain ebooks. users can choose from the entire collection  or from specific collections.
* **practical hardware and repair:** features the entire comprehensive iFixit repair database that can be used to repair and understand thousands of different devices.

there are also multiple preconfigured datasets including:
* **small storage tailored set:** includes only the most necessary information including wikipedia headers, can fit on a 16gb drive.
* **medium storage tailored set:** includes all of wikipedia (without pictures), all of iFixit, and more. fits on a 64gb drive.
* **large storage tailored set:** includes all of wikipedia (without pictures), iFixit, Gutenberg medical, technological and science collections, and a handful of stackexchange archives.
* **CHUNGUS data set:** includes every single distinct package. very large.

---

## technical stack & dependencies

the project runs natively on cross-platform core engines:

* **gui frontend:** uses Dear ImGui (docking branch) bound to an OpenGL3/GLFW3 backend graphics pipeline
* **network transport:** native 'libcurl' utilizing multi-threaded worker pools ('std::thread') to issue concurrent 'HTTP HEAD' metadata requests
* **cross-platform compilation:** fully compatible with Linux, modern Windows 10/11, and macOS environments.

### system requirements:

to build cleanly, ensure the native developer headers are present on your machine:

* cmake
* gcc
* glfw-devel
* mesa-libgl-devel
* libcurl-devel

---

## building and running

1. create and enter an isolated build folder:
```bash
mkdir build && cd build
```
2. generate build cache using CMake
```bash
cmake ..
```
3. compile the target binary executable
```bash
cmake --build .
```
4. launch the program
```bash
./bitswiss
```

---

## ai usage disclosure

no generative ai or LLMs were used to write or debug any code, headers, or documentation.

---

coded  in C++ using JetBrains CLion

compiled using CMake

GNU general public license

created for Hack Club Stardance 2026 by SwedishSplidney
