#pragma once

#include "vulkan_headers.hpp"
#include "vulkan_common.hpp"

#include "quantumvk/utils/intrusive.hpp"

#include <memory>
#include <functional>

namespace Vulkan 
{
	struct DeviceExtensions
	{
		bool supports_physical_device_properties2 = false;
		bool supports_external = false;
		bool supports_dedicated = false;
		bool supports_image_format_list = false;
		bool supports_debug_marker = false;
		bool supports_debug_utils = false;
		bool supports_mirror_clamp_to_edge = false;
		bool supports_google_display_timing = false;
		bool supports_nv_device_diagnostic_checkpoints = false;
		bool supports_vulkan_11_instance = false;
		bool supports_vulkan_11_device = false;
		bool supports_vulkan_12_instance = false;
		bool supports_vulkan_12_device = false;
		bool supports_external_memory_host = false;
		bool supports_surface_capabilities2 = false;
		bool supports_full_screen_exclusive = false;
		bool supports_update_template = false;
		bool supports_maintenance_1 = false;
		bool supports_maintenance_2 = false;
		bool supports_maintenance_3 = false;
		bool supports_descriptor_indexing = false;
		bool supports_conservative_rasterization = false;
		bool supports_bind_memory2 = false;
		bool supports_get_memory_requirements2 = false;
		bool supports_draw_indirect_count = false;
		bool supports_draw_parameters = false;
		bool supports_driver_properties = false;
		bool supports_calibrated_timestamps = false;
		VkPhysicalDeviceSubgroupProperties subgroup_properties = {};
		VkPhysicalDevice8BitStorageFeaturesKHR storage_8bit_features = {};
		VkPhysicalDevice16BitStorageFeaturesKHR storage_16bit_features = {};
		VkPhysicalDeviceFloat16Int8FeaturesKHR float16_int8_features = {};
		VkPhysicalDeviceExternalMemoryHostPropertiesEXT host_memory_properties = {};
		VkPhysicalDeviceMultiviewFeaturesKHR multiview_features = {};
		VkPhysicalDeviceSubgroupSizeControlFeaturesEXT subgroup_size_control_features = {};
		VkPhysicalDeviceSubgroupSizeControlPropertiesEXT subgroup_size_control_properties = {};
		VkPhysicalDeviceComputeShaderDerivativesFeaturesNV compute_shader_derivative_features = {};
		VkPhysicalDeviceHostQueryResetFeaturesEXT host_query_reset_features = {};
		VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT demote_to_helper_invocation_features = {};
		VkPhysicalDeviceScalarBlockLayoutFeaturesEXT scalar_block_features = {};
		VkPhysicalDeviceUniformBufferStandardLayoutFeaturesKHR ubo_std430_features = {};
		VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timeline_semaphore_features = {};
		VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing_features = {};
		VkPhysicalDeviceDescriptorIndexingPropertiesEXT descriptor_indexing_properties = {};
		VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservative_rasterization_properties = {};
		VkPhysicalDevicePerformanceQueryFeaturesKHR performance_query_features = {};
		VkPhysicalDeviceSamplerYcbcrConversionFeaturesKHR sampler_ycbcr_conversion_features = {};
		VkPhysicalDeviceDriverPropertiesKHR driver_properties = {};
	};

	enum VendorID
	{
		VENDOR_ID_AMD = 0x1002,
		VENDOR_ID_NVIDIA = 0x10de,
		VENDOR_ID_INTEL = 0x8086,
		VENDOR_ID_ARM = 0x13b5,
		VENDOR_ID_QCOM = 0x5143
	};

	class Context;

	struct ContextDeleter
	{
		void operator()(Context* context);
	};

	// The context is responsible for:
	// - Creating VkInstance
	// - Creating VkDevice
	// - Setting up VkQueues for graphics, compute and transfer.
	// - Setting up validation layers.
	// - Creating debug callbacks.
	class Context
	{
	public:

		// This is here to load libvulkan.so/libvulkan-1.dll/etc.
		// We do this once since we can have multiple devices around.
		// It is possible to pass in a custom pointer to vkGetInstanceProcAddr.
		// This is useful if the user loads the Vulkan loader in a custom way
		// and we can bootstrap ourselves straight from vkGetInstanceProcAddr rather
		// than loading Vulkan dynamically. This is common for GLFW for example.
		static bool InitLoader(PFN_vkGetInstanceProcAddr addr);
		// Creates the VkInstance and device. Simply pass in a number of instance 
		// extensions and device extensions and QuantumVk will d the rest.
		bool InitInstanceAndDevice(const char** instance_ext, uint32_t instance_ext_count, const char** device_ext, uint32_t device_ext_count);
		// This inits QuantumVk from an externaly created instance and device.
		bool InitFromInstanceAndDevice(VkInstance instance, VkPhysicalDevice gpu, VkDevice device, VkQueue queue, uint32_t queue_family);
		// Inits QuantumVk from and externally created instance.
		bool InitDeviceFromInstance(VkInstance instance, VkPhysicalDevice gpu, VkSurfaceKHR surface, const char** required_device_extensions,
			unsigned num_required_device_extensions, const char** required_device_layers,
			unsigned num_required_device_layers, const VkPhysicalDeviceFeatures* required_features);


		Context(const Context&) = delete;
		void operator=(const Context&) = delete;

		Context();
		~Context();

		// Returns the context's VkInstance.
		VkInstance GetInstance() const { return instance; }
		// Returns the context's VkPhysicalDevice.
		VkPhysicalDevice GetGPU() const { return gpu; }
		// Returns the context's VkDevice.
		VkDevice GetDevice() const { return device; }
		// Returns the volk table used to load vkFunctions.
		VolkDeviceTable& GetDeviceTable() { return *device_table; }
		// Returns the volk table used to load vkFunctions.
		const VolkDeviceTable& GetDeviceTable() const { return *device_table; }

		// Returns the general properties of the GPU.
		const VkPhysicalDeviceProperties& GetGPUProps() const { return gpu_props; }
		// Returns the gpu's memory properties.
		const VkPhysicalDeviceMemoryProperties& GetMemProps() const { return mem_props; }

		// Returns the graphics queue that most work is done on.
		VkQueue GetGraphicsQueue() const { return graphics_queue; }
		// Returns the compute queue that asyn compute comands are subitted on.
		VkQueue GetComputeQueue() const { return compute_queue; }
		// Returns the transfer queue used for dma operations.
		VkQueue GetTransferQueue() const { return transfer_queue; }

		uint32_t GetGraphicsQueueFamily() const { return graphics_queue_family; }
		uint32_t GetComputeQueueFamily() const { return compute_queue_family; }
		uint32_t GetTransferQueueFamily() const { return transfer_queue_family; }

		uint32_t GetTimestampValidBits() const { return timestamp_valid_bits; }

		void ReleaseInstance() { owned_instance = false; }
		void ReleaseDevice() { owned_device = false; }

		const DeviceExtensions& GetEnabledDeviceExtensions() const { return *ext; }
		const VkPhysicalDeviceFeatures& GetSupportedDeviceFeatures() const { return feat; }

		static const VkApplicationInfo& GetApplicationInfo(bool supports_vulkan_11_instance, bool supports_vulkan_12_instance);

		void NotifyValidationError(const char* msg);

		// Options

		void SetNumThreadIndices(unsigned indices) { num_thread_indices = indices; }
		void SetNotificationCallback(std::function<void(const char*)> func);
		void SetChooseGPUFunc(std::function<VkPhysicalDevice(std::vector<VkPhysicalDevice>&)> func);

		// ---------
		// Option Getters

		unsigned GetNumThreadIndices() const { return num_thread_indices; }

		// --------------

	private:

		VkDevice device = VK_NULL_HANDLE;
		VkInstance instance = VK_NULL_HANDLE;
		VkPhysicalDevice gpu = VK_NULL_HANDLE;

		VkPhysicalDeviceProperties gpu_props = {};
		VkPhysicalDeviceMemoryProperties mem_props = {};

		VkQueue graphics_queue = VK_NULL_HANDLE;
		VkQueue compute_queue = VK_NULL_HANDLE;
		VkQueue transfer_queue = VK_NULL_HANDLE;
		uint32_t graphics_queue_family = VK_QUEUE_FAMILY_IGNORED;
		uint32_t compute_queue_family = VK_QUEUE_FAMILY_IGNORED;
		uint32_t transfer_queue_family = VK_QUEUE_FAMILY_IGNORED;
		uint32_t timestamp_valid_bits = 0;
		unsigned num_thread_indices = 1;

		//These classes are simply too big to contain within context
		VolkDeviceTable* device_table;
		DeviceExtensions* ext;
		VkPhysicalDeviceFeatures feat;

		bool CreateInstance(const char** instance_ext, uint32_t instance_ext_count);
		bool CreateDevice(VkPhysicalDevice gpu_, VkSurfaceKHR surface, const char** required_device_extensions,
			unsigned num_required_device_extensions, const char** required_device_layers,
			unsigned num_required_device_layers, const VkPhysicalDeviceFeatures* required_features);

		bool owned_instance = false;
		bool owned_device = false;

#ifdef VULKAN_DEBUG
		VkDebugReportCallbackEXT debug_callback = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
#endif
		std::function<void(const char*)> message_callback;
		std::function<VkPhysicalDevice(std::vector<VkPhysicalDevice>&)> choose_gpu_func;

		bool force_no_validation = false;

		void Destroy();
		void CheckDescriptorIndexFeatures();

	};
}