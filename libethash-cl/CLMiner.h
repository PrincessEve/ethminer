/// OpenCL miner implementation.
///
/// @file
/// @copyright GNU General Public License

#pragma once

#include <libdevcore/Worker.h>
#include <libethcore/EthashAux.h>
#include <libethcore/Miner.h>
#include <libhwmon/wrapnvml.h>
#include <libhwmon/wrapadl.h>
#if defined(__linux)
#include <libhwmon/wrapamdsysfs.h>
#endif
#include <iostream>
#include <fstream>
#include <queue>

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS true
#define CL_HPP_ENABLE_EXCEPTIONS true
#define CL_HPP_CL_1_2_DEFAULT_BUILD true
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include "CL/cl2.hpp"

// macOS OpenCL fix:
#ifndef CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV
#define CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV       0x4000
#endif

#ifndef CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV
#define CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV       0x4001
#endif

#define OPENCL_PLATFORM_UNKNOWN 0
#define OPENCL_PLATFORM_NVIDIA  1
#define OPENCL_PLATFORM_AMD     2
#define OPENCL_PLATFORM_CLOVER  3


namespace dev
{
namespace eth
{

struct KernelSetup
{
	unsigned int m_searchBufferArg;
	unsigned int m_headerArg;
	unsigned int m_dagArg;
	unsigned int m_startNonceArg;
	unsigned int m_targetArg;
	unsigned int m_isolateArg;
	int m_factor1Arg;
	int m_factor2Arg;
	int m_dagSize128Arg;

	KernelSetup() : m_searchBufferArg(0), m_headerArg(1), m_dagArg(2),
		m_startNonceArg(3), m_targetArg(4), m_isolateArg(5),
		m_factor1Arg(-1), m_factor2Arg(-1), m_dagSize128Arg(-1) {}
};

class CLMiner: public Miner
{
public:
	/* -- default values -- */
	/// Default value of the local work size. Also known as workgroup size.
	static const unsigned c_defaultLocalWorkSize = 128;
	/// Default value of the global work size as a multiplier of the local work size
	static const unsigned c_defaultGlobalWorkSizeMultiplier = 8192;
    /// Number of buffers to alternate between for the search kernels
	static const unsigned c_bufferCount = 2; 


	CLMiner(FarmFace& _farm, unsigned _index);
	~CLMiner();

	static unsigned instances() { return s_numInstances > 0 ? s_numInstances : 1; }
	static unsigned getNumDevices();
	static void listDevices();
	static bool configureGPU(
		unsigned _localWorkSize,
		unsigned _globalWorkSizeMultiplier,
		unsigned _platformId,
		uint64_t _currentBlock,
		unsigned _dagLoadMode,
		unsigned _dagCreateDevice
	);
	static void setNumInstances(unsigned _instances) { s_numInstances = std::min<unsigned>(_instances, getNumDevices()); }
	static void setThreadsPerHash(unsigned _threadsPerHash){s_threadsPerHash = _threadsPerHash; }
	static void setDevices(unsigned * _devices, unsigned _selectedDeviceCount)
	{
		for (unsigned i = 0; i < _selectedDeviceCount; i++)
		{
			s_devices[i] = _devices[i];
		}
	}
	HwMonitor hwmon() override;
protected:
	void kickOff() override;
	void pause() override;

private:
	void workLoop() override;
	void report(uint64_t _nonce, WorkPackage const& _w);

	bool init(const h256& seed);
	bool loadBinaryKernel(string platform, cl::Device device, uint32_t dagSize128, uint32_t lightSize64, int platformId, int computeCapability, char *options);

	cl::Context m_context;
	cl::CommandQueue m_queue;
	cl::Kernel m_searchKernel;
	cl::Kernel m_asmSearchKernel;
	cl::Kernel m_dagKernel;
	cl::Buffer m_dag;
	cl::Buffer m_light;
	cl::Buffer m_header;
	cl::Buffer m_searchBuffer[c_bufferCount];
	unsigned m_globalWorkSize = 0;
	unsigned m_workgroupSize = 0;

	KernelSetup m_kernelArgs;

	bool m_useAsmKernel = true;
	bool m_cpuValidateNonce = true;
	bool m_clReportMixHash = false;
	unsigned m_maxSolutions = 32;

	static unsigned s_platformId;
	static unsigned s_numInstances;
	static unsigned s_threadsPerHash;
	static int s_devices[16];

	/// The local work size for the search
	static unsigned s_workgroupSize;
	/// The initial global work size for the searches
	static unsigned s_initialGlobalWorkSize;

	Mutex x_all;

	wrap_nvml_handle *nvmlh = NULL;
	wrap_adl_handle *adlh = NULL;
#if defined(__linux)
	wrap_amdsysfs_handle *sysfsh = NULL;
#endif
};

}
}
