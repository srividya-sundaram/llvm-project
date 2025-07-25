; RUN:  llc -amdgpu-scalarize-global-loads=false  -mtriple=amdgcn -amdgpu-load-store-vectorizer=0 < %s | FileCheck -check-prefix=GCN %s

; GatherAllAliases gives up on trying to analyze cases where the
; pointer may have been loaded from an aliased store, so make sure
; that this works and allows moving the stores to a better chain to
; allow them to be merged merged when it's clear the pointer is loaded
; from constant/invariant memory.

; GCN-LABEL: {{^}}test_merge_store_constant_i16_invariant_global_pointer_load:
; GCN-DAG: buffer_load_dwordx2 [[PTR:v\[[0-9]+:[0-9]+\]]],
; GCN-DAG: v_mov_b32_e32 [[K:v[0-9]+]], 0x1c8007b
; GCN: buffer_store_dword [[K]], [[PTR]]
define amdgpu_kernel void @test_merge_store_constant_i16_invariant_global_pointer_load(ptr addrspace(1) dereferenceable(4096) nonnull %in) #0 {
  %ptr = load ptr addrspace(1), ptr addrspace(1) %in, !invariant.load !0
  %ptr.1 = getelementptr i16, ptr addrspace(1) %ptr, i64 1
  store i16 123, ptr addrspace(1) %ptr, align 4
  store i16 456, ptr addrspace(1) %ptr.1
  ret void
}

; GCN-LABEL: {{^}}test_merge_store_constant_i16_invariant_constant_pointer_load:
; GCN: s_load_dwordx2 s[[[SPTR_LO:[0-9]+]]:[[SPTR_HI:[0-9]+]]]
; GCN: v_mov_b32_e32 [[K:v[0-9]+]], 0x1c8007b
; GCN: buffer_store_dword [[K]], off, s[[[SPTR_LO]]:
define amdgpu_kernel void @test_merge_store_constant_i16_invariant_constant_pointer_load(ptr addrspace(4) dereferenceable(4096) nonnull %in) #0 {
  %ptr = load ptr addrspace(1), ptr addrspace(4) %in, !invariant.load !0
  %ptr.1 = getelementptr i16, ptr addrspace(1) %ptr, i64 1
  store i16 123, ptr addrspace(1) %ptr, align 4
  store i16 456, ptr addrspace(1) %ptr.1
  ret void
}

!0 = !{}

attributes #0 = { nounwind }
