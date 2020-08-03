# Decent Enclave Application Framework

## NOTE

We've noticed a change in the latest version of the build tool and a staled 3rd party library may cause the build process to fail. We're working on updating the dependency, and the fix will be released shortly.

## Requirements
- Check compatibility of computer's hardware. 
  - Some computer doesn't have or support the SGX hardware. If the computer doesn't support, try to use the simulation mode to develop the program.
- Install [CMake(https://cmake.org/download/)](https://cmake.org/download/)
- Install [Intel SGX SDK(https://software.intel.com/en-us/sgx/sdk)](https://software.intel.com/en-us/sgx/sdk)

## NOTE
- This repo is only created for the purpose of anonymity in paper submission.
- Some submodules in `lib` directory are pushed with source files directly, to avoid potential violation of anonymity caused by forked repos.
- The code has only been fully tested under *Windows 10* environment.
