/// Tests the behaviors of using -fsycl --offload-arch=<intel-gpu-values>.

// SYCL AOT compilation to Intel GPUs using --offload-arch

// RUN: %clangxx -### -fsycl --offload-arch=bmg_g21_gpu %s 2>&1 | \
// RUN:   FileCheck %s --check-prefixes=TARGET-TRIPLE-GPU,CLANG-OFFLOAD-PACKAGER-GPU -DDEV_STR=bmg_g21_gpu -DMAC_STR=BMG_G21_GPU


// TARGET-TRIPLE-GPU: clang{{.*}} "-triple" "spirv64-unknown-unknown"
// CLANG-OFFLOAD-PACKAGER-GPU: clang-offload-packager{{.*}} "--image={{.*}}triple=spirv64-unknown-unknown,arch=[[DEV_STR]],kind=sycl"
// CLANG-OFFLOAD-PACKAGER-GPU-OPTS: clang-offload-packager{{.*}} "--image={{.*}}triple=spirv64-unknown-unknown,arch=[[DEV_STR]],kind=sycl{{.*}}"