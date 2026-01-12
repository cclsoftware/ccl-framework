# Core C Runtime
## Introduction
This provides standard c runtime functions commonly missing from embedded OSes.  Functions are implemented in the usual files with core prepended to the filename and conditionally declared in the public/coreclibrary.h

## Use
Each function implementation can be enabled by adding the relevant CORE_<FUNCTIONNAME>_NOT_PROVIDED definition.