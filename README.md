# Decent Enclave Application Framework

## NOTE

- For the purpose of anonymity of paper submission, we created this *temporary* repo to show the source code. (That's why you can't see full commit history in here)
- Some submodules in `libs` directory may be pushed with source files directly, to avoid potential violation of anonymity caused by the link of forked repos.
- So far the code has only been fully tested under *Windows 10* environment.

## Requirements

- Intel SGX hardware support
- Intel SGX driver (PSW)
- Intel SGX SDK (v2.8.100.2 or higher)
	- [Intel SGX SDK(https://software.intel.com/en-us/sgx/sdk)](https://software.intel.com/en-us/sgx/sdk)
- CMake
	- [CMake(https://cmake.org/download/)](https://cmake.org/download/)
- Python 3.7 or higher
- Microsoft Visual Studio 2019 (community version or higher) (*not* Visual Studio Code)
- Make sure the system environmental variable `SGXSDKInstallPath` (on Windows) is set, otherwise, CMake will not be able to find the path to the SGX SDK

## Build & Run

- This repo contains only the Decent API SDK, which does not contain any executable. Please refer to the specific repos for their build & run instructions.
