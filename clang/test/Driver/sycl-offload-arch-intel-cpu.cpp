/// Tests the behaviors of using -fsycl --offload-arch=<intel-cpu-values>.

// SYCL AOT compilation to Intel CPUs using --offload-arch

// RUN: %clangxx -### -fsycl --offload-arch=graniterapids_cpu %s 2>&1 | \
// RUN:   FileCheck %s --check-prefixes=TARGET-TRIPLE-CPU,CLANG-OFFLOAD-PACKAGER-CPU -DDEV_STR=graniterapids_cpu

// TARGET-TRIPLE-CPU: clang{{.*}} "-triple" "spirv64-unknown-unknown"
// CLANG-OFFLOAD-PACKAGER-CPU: clang-offload-packager{{.*}} "--image={{.*}}triple=spirv64-unknown-unknown,arch=[[DEV_STR]],kind=sycl"

// Tests for handling a missing architecture.
//
// RUN: not %clangxx  -fsycl --offload-arch= %s -### 2>&1 \
// RUN:   | FileCheck -check-prefix=MISSING-OFFLOAD-ARCH-VALUE %s
// RUN: not %clang_cl  -fsycl --offload-arch= %s -### 2>&1 \
// RUN:   | FileCheck -check-prefix=MISSING-OFFLOAD-ARCH-VALUE %s

// MISSING-OFFLOAD-ARCH-VALUE: error: must pass in a valid cpu or gpu architecture string to '--offload-arch'

// Tests for handling a incorrect --offload-arch architecture vlue.
//
// RUN: not %clangxx  -fsycl --offload-arch=badArch %s -### 2>&1 \
// RUN:   | FileCheck -check-prefix=BAD-ARCH %s
// RUN: not %clang_cl  -fsycl --offload-arch=badArch %s -### 2>&1 \
// RUN:   | FileCheck -check-prefix=BAD-ARCH %s

// BAD-ARCH: error: SYCL target is invalid: 'badArch'

