//===--- SYCL.cpp - SYCL Tool and ToolChain Implementations -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "SYCL.h"
#include "CommonArgs.h"
#include "llvm/Support/Path.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang::driver::tools;
using namespace clang;
using namespace llvm::opt;

// Struct that relates an AOT target value with
// Intel CPUs and Intel GPUs.
struct StringToOffloadArchSYCLMap {
  const char *ArchName;
  SYCLSupportedIntelArchs IntelArch;
};

// Mapping of supported SYCL offloading architectures.
static const StringToOffloadArchSYCLMap StringToArchNamesMap[] = {
    // Intel CPU mapping.
    {"skylake-avx512", SYCLSupportedIntelArchs::SKYLAKEAVX512},
    {"core-avx2", SYCLSupportedIntelArchs::COREAVX2},
    {"corei7-avx", SYCLSupportedIntelArchs::COREI7AVX},
    {"corei7", SYCLSupportedIntelArchs::COREI7},
    {"westmere", SYCLSupportedIntelArchs::WESTMERE},
    {"sandybridge", SYCLSupportedIntelArchs::SANDYBRIDGE},
    {"ivybridge", SYCLSupportedIntelArchs::IVYBRIDGE},
    {"broadwell", SYCLSupportedIntelArchs::BROADWELL},
    {"coffeelake", SYCLSupportedIntelArchs::COFFEELAKE},
    {"alderlake", SYCLSupportedIntelArchs::ALDERLAKE},
    {"skylake", SYCLSupportedIntelArchs::SKYLAKE},
    {"skx", SYCLSupportedIntelArchs::SKX},
    {"cascadelake", SYCLSupportedIntelArchs::CASCADELAKE},
    {"icelake-client", SYCLSupportedIntelArchs::ICELAKECLIENT},
    {"icelake-server", SYCLSupportedIntelArchs::ICELAKESERVER},
    {"sapphirerapids", SYCLSupportedIntelArchs::SAPPHIRERAPIDS},
    {"graniterapids", SYCLSupportedIntelArchs::GRANITERAPIDS},
    // Intel GPU mapping.
    {"bdw", SYCLSupportedIntelArchs::BDW},
    {"skl", SYCLSupportedIntelArchs::SKL},
    {"kbl", SYCLSupportedIntelArchs::KBL},
    {"cfl", SYCLSupportedIntelArchs::CFL},
    {"apl", SYCLSupportedIntelArchs::APL},
    {"bxt", SYCLSupportedIntelArchs::BXT},
    {"glk", SYCLSupportedIntelArchs::GLK},
    {"whl", SYCLSupportedIntelArchs::WHL},
    {"aml", SYCLSupportedIntelArchs::AML},
    {"cml", SYCLSupportedIntelArchs::CML},
    {"icllp", SYCLSupportedIntelArchs::ICLLP},
    {"icl", SYCLSupportedIntelArchs::ICL},
    {"ehl", SYCLSupportedIntelArchs::EHL},
    {"jsl", SYCLSupportedIntelArchs::JSL},
    {"tgllp", SYCLSupportedIntelArchs::TGLLP},
    {"tgl", SYCLSupportedIntelArchs::TGL},
    {"rkl", SYCLSupportedIntelArchs::RKL},
    {"adl_s", SYCLSupportedIntelArchs::ADL_S},
    {"rpl_s", SYCLSupportedIntelArchs::RPL_S},
    {"adl_p", SYCLSupportedIntelArchs::ADL_P},
    {"adl_n", SYCLSupportedIntelArchs::ADL_N},
    {"dg1", SYCLSupportedIntelArchs::DG1},
    {"acm_g10", SYCLSupportedIntelArchs::ACM_G10},
    {"dg2_g10", SYCLSupportedIntelArchs::DG2_G10},
    {"acm_g11", SYCLSupportedIntelArchs::ACM_G11},
    {"dg2_g10", SYCLSupportedIntelArchs::DG2_G10},
    {"dg2_g11", SYCLSupportedIntelArchs::DG2_G11},
    {"acm_g12", SYCLSupportedIntelArchs::ACM_G12},
    {"dg2_g12", SYCLSupportedIntelArchs::DG2_G12},
    {"pvc", SYCLSupportedIntelArchs::PVC},
    {"pvc_vg", SYCLSupportedIntelArchs::PVC_VG},
    {"mtl_u", SYCLSupportedIntelArchs::MTL_U},
    {"mtl_s", SYCLSupportedIntelArchs::MTL_S},
    {"arl_u", SYCLSupportedIntelArchs::ARL_U},
    {"arl_s", SYCLSupportedIntelArchs::ARL_S},
    {"mtl_h", SYCLSupportedIntelArchs::MTL_H},
    {"arl_h", SYCLSupportedIntelArchs::ARL_H},
    {"bmg_g21", SYCLSupportedIntelArchs::BMG_G21},
    {"lnl_m", SYCLSupportedIntelArchs::LNL_M}};

// Check if the user provided value for --offload-arch is a valid
// SYCL supported Intel AOT target.
SYCLSupportedIntelArchs
clang::driver::StringToOffloadArchSYCL(llvm::StringRef ArchNameAsString) {
  auto result = std::find_if(
      std::begin(StringToArchNamesMap), std::end(StringToArchNamesMap),
      [ArchNameAsString](const StringToOffloadArchSYCLMap &map) {
        return ArchNameAsString == map.ArchName;
      });
  if (result == std::end(StringToArchNamesMap))
    return SYCLSupportedIntelArchs::UNKNOWN;
  return result->IntelArch;
}

// This is a mapping between the user provided --offload-arch value for Intel
// GPU targets and the spir64_gen device name accepted by OCLOC (the Intel GPU
// AOT compiler).
StringRef clang::driver::mapIntelGPUArchName(StringRef ArchName) {
  StringRef Arch;
  Arch = llvm::StringSwitch<StringRef>(ArchName)
             .Case("bdw", "bdw")
             .Case("skl", "skl")
             .Case("kbl", "kbl")
             .Case("cfl", "cfl")
             .Cases("apl", "bxt", "apl")
             .Case("glk", "glk")
             .Case("whl", "whl")
             .Case("aml", "aml")
             .Case("cml", "cml")
             .Cases("icllp", "icl", "icllp")
             .Cases("ehl", "jsl", "ehl")
             .Cases("tgllp", "tgl", "tgllp")
             .Case("rkl", "rkl")
             .Cases("adl_s", "rpl_s", "adl_s")
             .Case("adl_p", "adl_p")
             .Case("adl_n", "adl_n")
             .Case("dg1", "dg1")
             .Cases("acm_g10", "dg2_g10", "acm_g10")
             .Cases("acm_g11", "dg2_g11", "acm_g11")
             .Cases("acm_g12", "dg2_g12", "acm_g12")
             .Case("pvc", "pvc")
             .Case("pvc_vg", "pvc_vg")
             .Cases("mtl_u", "mtl_s", "arl_u", "arl_s", "mtl_u")
             .Case("mtl_h", "mtl_h")
             .Case("arl_h", "arl_h")
             .Case("bmg_g21", "bmg_g21")
             .Case("lnl_m", "lnl_m")
             .Default("");
  return Arch;
}

SmallString<64> clang::driver::getGenDeviceMacro(StringRef DeviceName) {
  SmallString<64> Macro;
  StringRef Ext = llvm::StringSwitch<StringRef>(DeviceName)
                      .Case("bdw", "INTEL_GPU_BDW")
                      .Case("skl", "INTEL_GPU_SKL")
                      .Case("kbl", "INTEL_GPU_KBL")
                      .Case("cfl", "INTEL_GPU_CFL")
                      .Case("apl", "INTEL_GPU_APL")
                      .Case("glk", "INTEL_GPU_GLK")
                      .Case("whl", "INTEL_GPU_WHL")
                      .Case("aml", "INTEL_GPU_AML")
                      .Case("cml", "INTEL_GPU_CML")
                      .Case("icllp", "INTEL_GPU_ICLLP")
                      .Case("ehl", "INTEL_GPU_EHL")
                      .Case("tgllp", "INTEL_GPU_TGLLP")
                      .Case("rkl", "INTEL_GPU_RKL")
                      .Case("adl_s", "INTEL_GPU_ADL_S")
                      .Case("adl_p", "INTEL_GPU_ADL_P")
                      .Case("adl_n", "INTEL_GPU_ADL_N")
                      .Case("dg1", "INTEL_GPU_DG1")
                      .Case("acm_g10", "INTEL_GPU_ACM_G10")
                      .Case("acm_g11", "INTEL_GPU_ACM_G11")
                      .Case("acm_g12", "INTEL_GPU_ACM_G12")
                      .Case("pvc", "INTEL_GPU_PVC")
                      .Case("pvc_vg", "INTEL_GPU_PVC_VG")
                      .Case("mtl_u", "INTEL_GPU_MTL_U")
                      .Case("mtl_h", "INTEL_GPU_MTL_H")
                      .Case("arl_h", "INTEL_GPU_ARL_H")
                      .Case("bmg_g21", "INTEL_GPU_BMG_G21")
                      .Case("lnl_m", "INTEL_GPU_LNL_M")
                      .Case("ptl_h", "INTEL_GPU_PTL_H")
                      .Case("ptl_u", "INTEL_GPU_PTL_U")
                      .Case("sm_50", "NVIDIA_GPU_SM_50")
                      .Case("sm_52", "NVIDIA_GPU_SM_52")
                      .Case("sm_53", "NVIDIA_GPU_SM_53")
                      .Case("sm_60", "NVIDIA_GPU_SM_60")
                      .Case("sm_61", "NVIDIA_GPU_SM_61")
                      .Case("sm_62", "NVIDIA_GPU_SM_62")
                      .Case("sm_70", "NVIDIA_GPU_SM_70")
                      .Case("sm_72", "NVIDIA_GPU_SM_72")
                      .Case("sm_75", "NVIDIA_GPU_SM_75")
                      .Case("sm_80", "NVIDIA_GPU_SM_80")
                      .Case("sm_86", "NVIDIA_GPU_SM_86")
                      .Case("sm_87", "NVIDIA_GPU_SM_87")
                      .Case("sm_89", "NVIDIA_GPU_SM_89")
                      .Case("sm_90", "NVIDIA_GPU_SM_90")
                      .Case("sm_90a", "NVIDIA_GPU_SM_90A")
                      .Case("gfx700", "AMD_GPU_GFX700")
                      .Case("gfx701", "AMD_GPU_GFX701")
                      .Case("gfx702", "AMD_GPU_GFX702")
                      .Case("gfx703", "AMD_GPU_GFX703")
                      .Case("gfx704", "AMD_GPU_GFX704")
                      .Case("gfx705", "AMD_GPU_GFX705")
                      .Case("gfx801", "AMD_GPU_GFX801")
                      .Case("gfx802", "AMD_GPU_GFX802")
                      .Case("gfx803", "AMD_GPU_GFX803")
                      .Case("gfx805", "AMD_GPU_GFX805")
                      .Case("gfx810", "AMD_GPU_GFX810")
                      .Case("gfx900", "AMD_GPU_GFX900")
                      .Case("gfx902", "AMD_GPU_GFX902")
                      .Case("gfx904", "AMD_GPU_GFX904")
                      .Case("gfx906", "AMD_GPU_GFX906")
                      .Case("gfx908", "AMD_GPU_GFX908")
                      .Case("gfx909", "AMD_GPU_GFX909")
                      .Case("gfx90a", "AMD_GPU_GFX90A")
                      .Case("gfx90c", "AMD_GPU_GFX90C")
                      .Case("gfx940", "AMD_GPU_GFX940")
                      .Case("gfx941", "AMD_GPU_GFX941")
                      .Case("gfx942", "AMD_GPU_GFX942")
                      .Case("gfx1010", "AMD_GPU_GFX1010")
                      .Case("gfx1011", "AMD_GPU_GFX1011")
                      .Case("gfx1012", "AMD_GPU_GFX1012")
                      .Case("gfx1013", "AMD_GPU_GFX1013")
                      .Case("gfx1030", "AMD_GPU_GFX1030")
                      .Case("gfx1031", "AMD_GPU_GFX1031")
                      .Case("gfx1032", "AMD_GPU_GFX1032")
                      .Case("gfx1033", "AMD_GPU_GFX1033")
                      .Case("gfx1034", "AMD_GPU_GFX1034")
                      .Case("gfx1035", "AMD_GPU_GFX1035")
                      .Case("gfx1036", "AMD_GPU_GFX1036")
                      .Case("gfx1100", "AMD_GPU_GFX1100")
                      .Case("gfx1101", "AMD_GPU_GFX1101")
                      .Case("gfx1102", "AMD_GPU_GFX1102")
                      .Case("gfx1103", "AMD_GPU_GFX1103")
                      .Case("gfx1150", "AMD_GPU_GFX1150")
                      .Case("gfx1151", "AMD_GPU_GFX1151")
                      .Case("gfx1200", "AMD_GPU_GFX1200")
                      .Case("gfx1201", "AMD_GPU_GFX1201")
                      .Default("");
  if (!Ext.empty()) {
    Macro = "__SYCL_TARGET_";
    Macro += Ext;
    Macro += "__";
  }
  return Macro;
}

SYCLInstallationDetector::SYCLInstallationDetector(
    const Driver &D, const llvm::Triple &HostTriple,
    const llvm::opt::ArgList &Args) {}

void SYCLInstallationDetector::addSYCLIncludeArgs(
    const ArgList &DriverArgs, ArgStringList &CC1Args) const {
  if (DriverArgs.hasArg(clang::driver::options::OPT_nobuiltininc))
    return;

  // Add the SYCL header search locations in the specified order.
  // FIXME: Add the header file locations once the SYCL library and headers
  //        are properly established within the build.
}

// Unsupported options for SYCL device compilation.
static ArrayRef<options::ID> getUnsupportedOpts() {
  static constexpr options::ID UnsupportedOpts[] = {
      options::OPT_fsanitize_EQ,      // -fsanitize
      options::OPT_fcf_protection_EQ, // -fcf-protection
      options::OPT_fprofile_generate,
      options::OPT_fprofile_generate_EQ,
      options::OPT_fno_profile_generate, // -f[no-]profile-generate
      options::OPT_ftest_coverage,
      options::OPT_fno_test_coverage, // -f[no-]test-coverage
      options::OPT_fcoverage_mapping,
      options::OPT_fno_coverage_mapping, // -f[no-]coverage-mapping
      options::OPT_coverage,             // --coverage
      options::OPT_fprofile_instr_generate,
      options::OPT_fprofile_instr_generate_EQ,
      options::OPT_fno_profile_instr_generate, // -f[no-]profile-instr-generate
      options::OPT_fprofile_arcs,
      options::OPT_fno_profile_arcs, // -f[no-]profile-arcs
      options::OPT_fcreate_profile,  // -fcreate-profile
      options::OPT_fprofile_instr_use,
      options::OPT_fprofile_instr_use_EQ,       // -fprofile-instr-use
      options::OPT_forder_file_instrumentation, // -forder-file-instrumentation
      options::OPT_fcs_profile_generate,        // -fcs-profile-generate
      options::OPT_fcs_profile_generate_EQ,
  };
  return UnsupportedOpts;
}

SYCLToolChain::SYCLToolChain(const Driver &D, const llvm::Triple &Triple,
                             const ToolChain &HostTC, const ArgList &Args)
    : ToolChain(D, Triple, Args), HostTC(HostTC),
      SYCLInstallation(D, Triple, Args) {
  // Lookup binaries into the driver directory, this is used to discover any
  // dependent SYCL offload compilation tools.
  getProgramPaths().push_back(getDriver().Dir);

  // Diagnose unsupported options only once.
  for (OptSpecifier Opt : getUnsupportedOpts()) {
    if (const Arg *A = Args.getLastArg(Opt)) {
      D.Diag(clang::diag::warn_drv_unsupported_option_for_target)
          << A->getAsString(Args) << getTriple().str();
    }
  }
}

void SYCLToolChain::addClangTargetOptions(
    const llvm::opt::ArgList &DriverArgs, llvm::opt::ArgStringList &CC1Args,
    Action::OffloadKind DeviceOffloadingKind) const {
  HostTC.addClangTargetOptions(DriverArgs, CC1Args, DeviceOffloadingKind);
}

llvm::opt::DerivedArgList *
SYCLToolChain::TranslateArgs(const llvm::opt::DerivedArgList &Args,
                             StringRef BoundArch,
                             Action::OffloadKind DeviceOffloadKind) const {
  DerivedArgList *DAL =
      HostTC.TranslateArgs(Args, BoundArch, DeviceOffloadKind);

  bool IsNewDAL = false;
  if (!DAL) {
    DAL = new DerivedArgList(Args.getBaseArgs());
    IsNewDAL = true;
  }

  for (Arg *A : Args) {
    // Filter out any options we do not want to pass along to the device
    // compilation.
    auto Opt(A->getOption());
    bool Unsupported = false;
    for (OptSpecifier UnsupportedOpt : getUnsupportedOpts()) {
      if (Opt.matches(UnsupportedOpt)) {
        if (Opt.getID() == options::OPT_fsanitize_EQ &&
            A->getValues().size() == 1) {
          std::string SanitizeVal = A->getValue();
          if (SanitizeVal == "address") {
            if (IsNewDAL)
              DAL->append(A);
            continue;
          }
        }
        if (!IsNewDAL)
          DAL->eraseArg(Opt.getID());
        Unsupported = true;
      }
    }
    if (Unsupported)
      continue;
    if (IsNewDAL)
      DAL->append(A);
  }

  const OptTable &Opts = getDriver().getOpts();
  if (!BoundArch.empty()) {
    DAL->eraseArg(options::OPT_march_EQ);
    DAL->AddJoinedArg(nullptr, Opts.getOption(options::OPT_march_EQ),
                      BoundArch);
  }
  return DAL;
}

void SYCLToolChain::addClangWarningOptions(ArgStringList &CC1Args) const {
  HostTC.addClangWarningOptions(CC1Args);
}

ToolChain::CXXStdlibType
SYCLToolChain::GetCXXStdlibType(const ArgList &Args) const {
  return HostTC.GetCXXStdlibType(Args);
}

void SYCLToolChain::addSYCLIncludeArgs(const ArgList &DriverArgs,
                                       ArgStringList &CC1Args) const {
  SYCLInstallation.addSYCLIncludeArgs(DriverArgs, CC1Args);
}

void SYCLToolChain::AddClangSystemIncludeArgs(const ArgList &DriverArgs,
                                              ArgStringList &CC1Args) const {
  HostTC.AddClangSystemIncludeArgs(DriverArgs, CC1Args);
}

void SYCLToolChain::AddClangCXXStdlibIncludeArgs(const ArgList &Args,
                                                 ArgStringList &CC1Args) const {
  HostTC.AddClangCXXStdlibIncludeArgs(Args, CC1Args);
}
