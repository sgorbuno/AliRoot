//**************************************************************************\
//* This file is property of and copyright by the ALICE Project            *\
//* ALICE Experiment at CERN, All rights reserved.                         *\
//*                                                                        *\
//* Primary Authors: Matthias Richter <Matthias.Richter@ift.uib.no>        *\
//*                  for The ALICE HLT Project.                            *\
//*                                                                        *\
//* Permission to use, copy, modify and distribute this software and its   *\
//* documentation strictly for non-commercial purposes is hereby granted   *\
//* without fee, provided that the above copyright notice appears in all   *\
//* copies and that both the copyright notice and this permission notice   *\
//* appear in the supporting documentation. The authors make no claims     *\
//* about the suitability of this software for any purpose. It is          *\
//* provided "as is" without express or implied warranty.                  *\
//**************************************************************************

/// \file GPUDataTypes.h
/// \author David Rohr

#ifndef GPUDATATYPES_H
#define GPUDATATYPES_H

#include "GPUCommonDef.h"

// These are basic and non-comprex data types, which will also be visible on the GPU.
// Please add complex data types required on the host but not GPU to GPUHostDataTypes.h and forward-declare!
#ifndef __OPENCL__
#include <cstddef>
#endif
#ifdef GPUCA_NOCOMPAT_ALLOPENCL
#include <type_traits>
#endif
#ifdef GPUCA_NOCOMPAT
#include "GPUTRDDef.h"

class AliHLTTPCClusterMCLabel;
struct AliHLTTPCRawCluster;
namespace o2
{
namespace tpc
{
struct ClusterNativeAccess;
template <class T>
struct CompressedClustersPtrs_helper;
struct CompressedClustersCounters;
using CompressedClusters = CompressedClustersPtrs_helper<CompressedClustersCounters>;
} // namespace tpc
} // namespace o2
#endif

namespace o2
{
struct InteractionRecord;
class MCCompLabel;
namespace base
{
class MatLayerCylSet;
} // namespace base
namespace trd
{
class TRDGeometryFlat;
} // namespace trd
namespace dataformats
{
template <class T>
class MCTruthContainer;
} // namespace dataformats
} // namespace o2

namespace GPUCA_NAMESPACE
{
namespace gpu
{
namespace deprecated
{
struct PackedDigit;
}
} // namespace gpu
} // namespace GPUCA_NAMESPACE

namespace GPUCA_NAMESPACE
{
namespace gpu
{
class TPCFastTransform;
} // namespace gpu
} // namespace GPUCA_NAMESPACE

namespace GPUCA_NAMESPACE
{
namespace gpu
{
#ifdef GPUCA_NOCOMPAT_ALLOPENCL
#include "utils/bitfield.h"
#define ENUM_CLASS class
#define ENUM_UINT : unsigned int
#define GPUCA_RECO_STEP GPUDataTypes::RecoStep
#else
#define ENUM_CLASS
#define ENUM_UINT
#define GPUCA_RECO_STEP GPUDataTypes
#endif

#ifdef __OPENCL__
MEM_CLASS_PRE() // Macro with some template magic for OpenCL 1.2
#endif
class GPUTPCTrack;
class GPUTPCHitId;
class GPUTPCGMMergedTrack;
struct GPUTPCGMMergedTrackHit;
class GPUTRDTrackletWord;
class GPUTPCMCInfo;
struct GPUTPCClusterData;
struct GPUTRDTrackletLabels;
struct GPUTPCDigitsMCInput;

class GPUDataTypes
{
 public:
  enum ENUM_CLASS GeometryType ENUM_UINT{RESERVED_GEOMETRY = 0, ALIROOT = 1, O2 = 2};
  enum DeviceType ENUM_UINT { INVALID_DEVICE = 0,
                              CPU = 1,
                              CUDA = 2,
                              HIP = 3,
                              OCL = 4,
                              OCL2 = 5 };
  enum ENUM_CLASS RecoStep { TPCConversion = 1,
                             TPCSliceTracking = 2,
                             TPCMerging = 4,
                             TPCCompression = 8,
                             TRDTracking = 16,
                             ITSTracking = 32,
                             TPCdEdx = 64,
                             TPCClusterFinding = 128,
                             AllRecoSteps = 0x7FFFFFFF,
                             NoRecoStep = 0 };
  enum ENUM_CLASS InOutType { TPCClusters = 1,
                              TPCSectorTracks = 2,
                              TPCMergedTracks = 4,
                              TPCCompressedClusters = 8,
                              TRDTracklets = 16,
                              TRDTracks = 32,
                              TPCRaw = 64 };

#ifdef GPUCA_NOCOMPAT_ALLOPENCL
  static constexpr const char* const RECO_STEP_NAMES[] = {"TPC Transformation", "TPC Sector Tracking", "TPC Track Merging and Fit", "TPC Compression", "TRD Tracking", "ITS Tracking", "TPC dEdx Computation", "TPC Cluster Finding"};
  typedef bitfield<RecoStep, unsigned int> RecoStepField;
  typedef bitfield<InOutType, unsigned int> InOutTypeField;
#endif
#ifdef GPUCA_NOCOMPAT
  static constexpr unsigned int NSLICES = 36;
#endif
  static DeviceType GetDeviceType(const char* type);
};

#ifdef GPUCA_NOCOMPAT_ALLOPENCL
struct GPURecoStepConfiguration {
  GPUDataTypes::RecoStepField steps = 0;
  GPUDataTypes::RecoStepField stepsGPUMask = GPUDataTypes::RecoStep::AllRecoSteps;
  GPUDataTypes::InOutTypeField inputs = 0;
  GPUDataTypes::InOutTypeField outputs = 0;
};
#endif

#ifdef GPUCA_NOCOMPAT

template <class T>
struct DefaultPtr {
  typedef T type;
};
template <class T>
struct ConstPtr {
  typedef const T type;
};

template <template <typename T> class S>
struct GPUCalibObjectsTemplate {
  typename S<TPCFastTransform>::type* fastTransform = nullptr;
  typename S<o2::base::MatLayerCylSet>::type* matLUT = nullptr;
  typename S<o2::trd::TRDGeometryFlat>::type* trdGeometry = nullptr;
};
typedef GPUCalibObjectsTemplate<DefaultPtr> GPUCalibObjects;
typedef GPUCalibObjectsTemplate<ConstPtr> GPUCalibObjectsConst;

struct GPUTrackingInOutZS {
  static constexpr unsigned int NSLICES = GPUDataTypes::NSLICES;
  static constexpr unsigned int NENDPOINTS = 20;
  struct GPUTrackingInOutZSSlice {
    const void* const* zsPtr[NENDPOINTS];
    const unsigned int* nZSPtr[NENDPOINTS];
    unsigned int count[NENDPOINTS];
  };
  struct GPUTrackingInOutZSCounts {
    unsigned int count[NSLICES][NENDPOINTS] = {};
  };
  struct GPUTrackingInOutZSMeta {
    void* ptr[NSLICES][NENDPOINTS];
    unsigned int n[NSLICES][NENDPOINTS];
  };
  GPUTrackingInOutZSSlice slice[NSLICES];
  o2::InteractionRecord* ir;
};

struct GPUTrackingInOutDigits {
  static constexpr unsigned int NSLICES = GPUDataTypes::NSLICES;
  const deprecated::PackedDigit* tpcDigits[NSLICES] = {nullptr};
  size_t nTPCDigits[NSLICES] = {0};
  GPUTPCDigitsMCInput* tpcDigitsMC;
};

struct GPUTrackingInOutPointers {
  GPUTrackingInOutPointers() = default;
  GPUTrackingInOutPointers(const GPUTrackingInOutPointers&) = default;
  static constexpr unsigned int NSLICES = GPUDataTypes::NSLICES;

  const GPUTrackingInOutZS* tpcZS = nullptr;
  const GPUTrackingInOutDigits* tpcPackedDigits = nullptr;
  const GPUTPCClusterData* clusterData[NSLICES] = {nullptr};
  unsigned int nClusterData[NSLICES] = {0};
  const AliHLTTPCRawCluster* rawClusters[NSLICES] = {nullptr};
  unsigned int nRawClusters[NSLICES] = {0};
  const o2::tpc::ClusterNativeAccess* clustersNative = nullptr;
  const GPUTPCTrack* sliceOutTracks[NSLICES] = {nullptr};
  unsigned int nSliceOutTracks[NSLICES] = {0};
  const GPUTPCHitId* sliceOutClusters[NSLICES] = {nullptr};
  unsigned int nSliceOutClusters[NSLICES] = {0};
  const AliHLTTPCClusterMCLabel* mcLabelsTPC = nullptr;
  unsigned int nMCLabelsTPC = 0;
  const GPUTPCMCInfo* mcInfosTPC = nullptr;
  unsigned int nMCInfosTPC = 0;
  const GPUTPCGMMergedTrack* mergedTracks = nullptr;
  unsigned int nMergedTracks = 0;
  const GPUTPCGMMergedTrackHit* mergedTrackHits = nullptr;
  unsigned int nMergedTrackHits = 0;
  const o2::tpc::CompressedClusters* tpcCompressedClusters = nullptr;
  const GPUTRDTrackletWord* trdTracklets = nullptr;
  unsigned int nTRDTracklets = 0;
  const GPUTRDTrackletLabels* trdTrackletsMC = nullptr;
  unsigned int nTRDTrackletsMC = 0;
  const GPUTRDTrackGPU* trdTracks = nullptr;
  unsigned int nTRDTracks = 0;
};
#else
struct GPUTrackingInOutPointers {
};
struct GPUCalibObjectsConst {
};
#endif

#undef ENUM_CLASS
#undef ENUM_UINT
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif
