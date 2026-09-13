# APC-Injector

A kernel-mode DLL injector for Windows x64, using **APC (Asynchronous Procedure Call)** injection via a custom kernel driver.

## How it works

- A kernel driver (`driver.sys`) maps and injects a DLL into a target process entirely from kernel mode
- Uses `KeInitializeApc` / `KeInsertQueueApc` to queue a user-mode APC on a target thread, executing the DLL entry point
- The loader (`loader.exe`) loads and communicates with the driver to trigger injection

## Structure

| File / Folder | Description |
|---|---|
| `modern_injector.cpp` | Core kernel-mode injection logic (APC, PE mapping, relocs) |
| `loader.cpp` | User-mode loader that talks to the driver |
| `test_client.cpp` | Simple test client |
| `test_dll.cpp` | Test DLL to inject |
| `driver/` | Kernel driver project |
| `kernelmode/` | Kernel-mode support code |
| `kernelmode.sln` | Visual Studio solution |

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 with WDK (Windows Driver Kit)

## Build

Open `kernelmode.sln` in Visual Studio and build in **Release x64**.

> For educational purposes only.
