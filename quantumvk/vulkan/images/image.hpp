#pragma once

#include "format.hpp"

#include "quantumvk/vulkan/misc/cookie.hpp"

#include "quantumvk/vulkan/vulkan_common.hpp"
#include "quantumvk/vulkan/vulkan_headers.hpp"

#include "quantumvk/vulkan/memory/memory_allocator.hpp"

#include "quantumvk/utils/small_vector.hpp"

namespace Vulkan
{
	//Forward declare device
	class Device;

	////////////////////////////////
	//Helper Image Functions////////
	////////////////////////////////

	//Convert Image Usage to stages the image might be used in
	static inline VkPipelineStageFlags ImageUsageToPossibleStages(VkImageUsageFlags usage)
	{
		VkPipelineStageFlags flags = 0;

		if (usage & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT))
			flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
			flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
			flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
			flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		if (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
		{
			VkPipelineStageFlags possible = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
				VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

			if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
				possible |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			flags &= possible;
		}

		return flags;
	}

	//Convert Layout to possible memory access
	static inline VkAccessFlags ImageLayoutToPossibleAccess(VkImageLayout layout)
	{
		switch (layout)
		{
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			return VK_ACCESS_TRANSFER_READ_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			return VK_ACCESS_TRANSFER_WRITE_BIT;
		default:
			return ~0u;
		}
	}

	// Convert Usage to stages the image may be used in
	static inline VkAccessFlags ImageUsageToPossibleAccess(VkImageUsageFlags usage)
	{
		VkAccessFlags flags = 0;

		if (usage & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT))
			flags |= VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
			flags |= VK_ACCESS_SHADER_READ_BIT;
		if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
			flags |= VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
			flags |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

		// Transient attachments can only be attachments, and never other resources.
		if (usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
		{
			flags &= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
				VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		}

		return flags;
	}

	//Get mip levels from extent
	static inline uint32_t ImageNumMipLevels(const VkExtent3D& extent)
	{
		uint32_t size = std::max(std::max(extent.width, extent.height), extent.depth);
		uint32_t levels = 0;
		while (size)
		{
			levels++;
			size >>= 1;
		}
		return levels;
	}

	//Get format features from usage
	static inline VkFormatFeatureFlags ImageUsageToFeatures(VkImageUsageFlags usage)
	{
		VkFormatFeatureFlags flags = 0;
		if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
			flags |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
			flags |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			flags |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			flags |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

		return flags;
	}

	////////////////////////////////
	////////////////////////////////
	////////////////////////////////

	////////////////////////////////
	//Image Data structs////////////
	////////////////////////////////

	// Specifies what data to load into a layer of an image
	struct InitialImageLayerData
	{
		// Data to load into layer (nullptr zero initializes that layer)
		void* data = nullptr;
	};

	// Specifies what data to load into all the layers of one mip level of an image
	struct InitialImageLevelData
	{
		// Array of InitialImageLayerData to load into each layer.
		InitialImageLayerData* layers;
	};

	struct InitialImageData
	{
		// Array of InitialImageLevelData to load into each layer.
		InitialImageLevelData* levels;
	};

	struct ImageStagingCopyInfo
	{
		// Position within buffer (void*) to copy memory from
		uint32_t buffer_offset = 0;
		// Width of larger image stored in buffer (leave at zero to make the region in the buffer correspond exactly to the data in the image)
		uint32_t buffer_row_length = 0;
		// Height of larger image stored in buffer (leave at zero to make the region in the buffer correspond exactly to the data in the image)
		uint32_t buffer_image_height = 0;

		uint32_t mip_level = 0;
		uint32_t base_array_layer = 0;
		uint32_t num_layers = 1;

		VkOffset3D image_offset = { 0, 0, 0 };
		VkExtent3D image_extent;
	};

	////////////////////////////////
	//Image/////////////////////////
	////////////////////////////////

	// Misc Image Create Info Flags
	// Normally image sharing mode is Exclusive and owned by the graphics queue family
	// Pipeline barriers can be used to transfer this ownership. Alternatively the concurrent
	// flags can be set to indicate what queues can own the image.
	enum ImageMiscFlagBits
	{
		// Causes lower mip levels to be automatically filled using linear blitting
		IMAGE_MISC_GENERATE_MIPS_BIT = 1 << 0,
		// Allows image views of type cube and cube array to be created if image is of type VK_IMAGE_2D.
		IMAGE_MISC_CUBE_COMPATIBLE_BIT = 1 << 1,
		// Allows image views of type 2D array to be created if image is of type VK_IMAGE_3D.
		IMAGE_MISC_2D_ARRAY_COMPATIBLE_BIT = 1 << 2,
		// This flags make the CreateImage call check that linear filtering is supported. If not, a null image is returned.
		IMAGE_MISC_VERIFY_FORMAT_FEATURE_SAMPLED_LINEAR_FILTER_BIT = 1 << 7,
		IMAGE_MISC_LINEAR_IMAGE_IGNORE_DEVICE_LOCAL_BIT = 1 << 8
	};
	using ImageMiscFlags = uint32_t;

	enum ImageCommandQueueFlagBits
	{
		IMAGE_COMMAND_QUEUE_GENERIC = 1 << 0,
		IMAGE_COMMAND_QUEUE_ASYNC_GRAPHICS = 1 << 1,
		IMAGE_COMMAND_QUEUE_ASYNC_COMPUTE = 1 << 2,
		IMAGE_COMMAND_QUEUE_ASYNC_TRANSFER = 1 << 3,
	};

	using ImageCommandQueueFlags = uint32_t;

	//Forward declare image
	class Image;

	//Memory type of image
	enum class ImageDomain
	{
		Physical, // Device local
		Transient, // Not backed by real memory, used for transient attachments
		LinearHostCached, // Visible on host as linear stream of pixels (preferes to be cached)
		LinearHost // Visible on host as linear stream of pixels (preferes to be coherent)
	};

	// Specifies what formats image views of this image can have
	enum class ImageViewFormats
	{
		// Views of this image must have the same format as this image.
		Same = 0,
		// Views of this image must have a compatible format to the format of this image
		Compatible, 
		// Views of this image must have one of the formats specified in the custom_view_formats array.
		Custom,
	};

	enum class ImageSharingMode
	{
		Concurrent = 0,
		Exclusive
	};

	//Specifies image view creation
	struct ImageCreateInfo
	{
		ImageDomain domain = ImageDomain::Physical;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 1;
		uint32_t levels = 1;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageType type = VK_IMAGE_TYPE_2D;
		uint32_t layers = 1;
		VkImageUsageFlags usage = 0;
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		ImageMiscFlags misc = 0;
		VkImageLayout initial_layout = VK_IMAGE_LAYOUT_GENERAL;

		ImageViewFormats view_formats = ImageViewFormats::Same;
		uint32_t num_custom_view_formats = 0;
		VkFormat* custom_view_formats = nullptr;

		ImageSharingMode sharing_mode = ImageSharingMode::Concurrent;
		ImageCommandQueueFlagBits exclusive_owner = IMAGE_COMMAND_QUEUE_GENERIC;
		ImageCommandQueueFlags concurrent_owners = IMAGE_COMMAND_QUEUE_GENERIC | IMAGE_COMMAND_QUEUE_ASYNC_GRAPHICS | IMAGE_COMMAND_QUEUE_ASYNC_COMPUTE | IMAGE_COMMAND_QUEUE_ASYNC_TRANSFER;

		static ImageCreateInfo Immutable2dImage(uint32_t width, uint32_t height, VkFormat format, bool mipmapped = false)
		{
			ImageCreateInfo info;
			info.width = width;
			info.height = height;
			info.depth = 1;
			info.levels = mipmapped ? 0u : 1u;
			info.format = format;
			info.type = VK_IMAGE_TYPE_2D;
			info.layers = 1;
			info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			info.samples = VK_SAMPLE_COUNT_1_BIT;
			info.misc = mipmapped ? unsigned(IMAGE_MISC_GENERATE_MIPS_BIT) : 0u;
			info.initial_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			return info;
		}

		static ImageCreateInfo
			Immutable3dImage(uint32_t width, uint32_t height, uint32_t depth, VkFormat format, bool mipmapped = false)
		{
			ImageCreateInfo info = Immutable2dImage(width, height, format, mipmapped);
			info.depth = depth;
			info.type = VK_IMAGE_TYPE_3D;
			return info;
		}

		static ImageCreateInfo RenderTarget(uint32_t width, uint32_t height, VkFormat format)
		{
			ImageCreateInfo info;
			info.width = width;
			info.height = height;
			info.depth = 1;
			info.levels = 1;
			info.format = format;
			info.type = VK_IMAGE_TYPE_2D;
			info.layers = 1;
			info.usage = (FormatHasDepthOrStencilAspect(format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) |
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

			info.samples = VK_SAMPLE_COUNT_1_BIT;
			info.misc = 0;
			info.initial_layout = VK_IMAGE_LAYOUT_GENERAL;
			return info;
		}

		static ImageCreateInfo TransientRenderTarget(uint32_t width, uint32_t height, VkFormat format)
		{
			ImageCreateInfo info;
			info.domain = ImageDomain::Transient;
			info.width = width;
			info.height = height;
			info.depth = 1;
			info.levels = 1;
			info.format = format;
			info.type = VK_IMAGE_TYPE_2D;
			info.layers = 1;
			info.usage = (FormatHasDepthOrStencilAspect(format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
			info.samples = VK_SAMPLE_COUNT_1_BIT;
			info.misc = 0;
			info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
			return info;
		}
	};

	//Specifies layout type
	enum class Layout
	{
		Optimal,
		General
	};

	class Image;

	struct ImageDeleter
	{
		void operator()(Image* image);
	};

	//Ref counted vkImage and vmaAllocation wrapper
	class Image : public Util::IntrusivePtrEnabled<Image, ImageDeleter, HandleCounter>, public Cookie, public InternalSyncEnabled
	{
	public:
		friend struct ImageDeleter;

		~Image();

		Image(Image&&) = delete;

		Image& operator=(Image&&) = delete;

		VkImage GetImage() const
		{
			return image;
		}

		VkFormat GetFormat() const
		{
			return create_info.format;
		}

		uint32_t GetWidth(uint32_t lod = 0) const
		{
			return std::max(1u, create_info.width >> lod);
		}

		uint32_t GetHeight(uint32_t lod = 0) const
		{
			return std::max(1u, create_info.height >> lod);
		}

		uint32_t GetDepth(uint32_t lod = 0) const
		{
			return std::max(1u, create_info.depth >> lod);
		}

		const ImageCreateInfo& GetCreateInfo() const
		{
			return create_info;
		}

		VkImageLayout GetLayout(VkImageLayout optimal) const
		{
			return layout_type == Layout::Optimal ? optimal : VK_IMAGE_LAYOUT_GENERAL;
		}

		Layout GetLayoutType() const
		{
			return layout_type;
		}

		void SetLayout(Layout layout)
		{
			layout_type = layout;
		}

		bool IsSwapchainImage() const
		{
			return swapchain_layout != VK_IMAGE_LAYOUT_UNDEFINED;
		}

		VkImageLayout GetSwapchainLayout() const
		{
			return swapchain_layout;
		}

		void SetSwapchainLayout(VkImageLayout layout)
		{
			swapchain_layout = layout;
		}

		const DeviceAllocation& GetAllocation() const
		{
			return alloc;
		}

		void DisownImage();

		bool ImageViewFormatSupported(VkFormat view_format) const;

	private:
		friend class Util::ObjectPool<Image>;

		Image(Device* device, VkImage image, const DeviceAllocation& alloc, const ImageCreateInfo& info);

		Device* device;
		VkImage image;
		DeviceAllocation alloc;
		ImageCreateInfo create_info;

		Layout layout_type = Layout::Optimal;
		VkImageLayout swapchain_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		bool owns_image = true;

		std::vector<VkFormat> custom_view_formats;
	};

	using ImageHandle = Util::IntrusivePtr<Image>;

	////////////////////////////////
	////////////////////////////////
	////////////////////////////////

	////////////////////////////////
	//Image View////////////////////
	////////////////////////////////

	//Specifies how to create an image view
	struct ImageViewCreateInfo
	{
		ImageHandle image;
		VkFormat format = VK_FORMAT_UNDEFINED;
		uint32_t base_level = 0;
		uint32_t levels = VK_REMAINING_MIP_LEVELS;
		uint32_t base_layer = 0;
		uint32_t layers = VK_REMAINING_ARRAY_LAYERS;
		VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		VkComponentMapping swizzle = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A, };
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
	};

	//Forward Declare image view
	class ImageView;

	//Functor to delete image view
	struct ImageViewDeleter
	{
		void operator()(ImageView* view);
	};

	//Ref-counted vkImageView wrapper
	class ImageView : public Util::IntrusivePtrEnabled<ImageView, ImageViewDeleter, HandleCounter>, public Cookie, public InternalSyncEnabled
	{
	public:
		friend struct ImageViewDeleter;

		ImageView(Device* device, VkImageView view, VkImageView depth, VkImageView stencil, const ImageViewCreateInfo& info);

		~ImageView();

		// By default, gets a combined view which includes all layers, levels, and aspects of the image
		VkImageView GetView() const
		{
			return view;
		}

		// Gets an image view which only includes floating point domains.
		// Takes effect when we want to sample from an image which is Depth/Stencil,
		// but we only want to sample depth.
		VkImageView GetFloatView() const
		{
			return depth_view != VK_NULL_HANDLE ? depth_view : view;
		}

		// Gets an image view which only includes integer domains.
		// Takes effect when we want to sample from an image which is Depth/Stencil,
		// but we only want to sample stencil.
		VkImageView GetIntegerView() const
		{
			return stencil_view != VK_NULL_HANDLE ? stencil_view : view;
		}

		const Image& GetImage() const
		{
			return *info.image;
		}

		Image& GetImage()
		{
			return *info.image;
		}

		VkImageAspectFlags GetAspect() const { return info.aspect; }
		VkFormat GetFormat() const { return info.format; }
		VkImageViewType GetType() const { return info.view_type; }
		const ImageViewCreateInfo& GetCreateInfo() const { return info; }

	private:
		Device* device;
		// Default view, contains all aspects
		VkImageView view = VK_NULL_HANDLE;
		// Depth view, null if the image isn't of a (VK_ASPECT_DEPTH_BIT | VK_ASPECT_STENCIL_BIT) format, identical to view except the aspect is VK_ASPECT_DEPTH_BIT
		VkImageView depth_view = VK_NULL_HANDLE;
		// Stencil view, null if the image isn't of a (VK_ASPECT_DEPTH_BIT | VK_ASPECT_STENCIL_BIT) format, identical to view except the aspect is VK_ASPECT_STENCIL_BIT
		VkImageView stencil_view = VK_NULL_HANDLE;
		// ImageView create info
		ImageViewCreateInfo info;
	};

	using ImageViewHandle = Util::IntrusivePtr<ImageView>;

	////////////////////////////////
	////////////////////////////////
	////////////////////////////////

	////////////////////////////////
	//Linear Host Image/////////////
	////////////////////////////////

	//Forward declare linear host image
	class LinearHostImage;
	//Functor to delete linear host image
	struct LinearHostImageDeleter
	{
		void operator()(LinearHostImage* image);
	};
	//Forward declare buffer
	class Buffer;
	//Linear Host image create info flags
	enum LinearHostImageCreateInfoFlagBits
	{
		LINEAR_HOST_IMAGE_HOST_CACHED_BIT = 1 << 0,
		LINEAR_HOST_IMAGE_REQUIRE_LINEAR_FILTER_BIT = 1 << 1,
		LINEAR_HOST_IMAGE_IGNORE_DEVICE_LOCAL_BIT = 1 << 2
	};
	using LinearHostImageCreateInfoFlags = uint32_t;

	//Linear host image create info
	struct LinearHostImageCreateInfo
	{
		unsigned width = 0;
		unsigned height = 0;
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageUsageFlags usage = 0;
		VkPipelineStageFlags stages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		LinearHostImageCreateInfoFlags flags = 0;
	};

	// Special image type which supports direct CPU mapping.
	// Useful optimization for UMA implementations of Vulkan where we don't necessarily need
	// to perform staging copies. It gracefully falls back to staging buffer as needed.
	// Only usage flag SAMPLED_BIT is currently supported.
	class LinearHostImage : public Util::IntrusivePtrEnabled<LinearHostImage, LinearHostImageDeleter, HandleCounter>
	{
	public:
		friend struct LinearHostImageDeleter;

		size_t GetRowPitchBytes() const;
		size_t GetOffset() const;
		//const ImageView& GetView() const;
		const Image& GetImage() const;
		const DeviceAllocation& GetHostVisibleAllocation() const;
		const Buffer& GetHostVisibleBuffer() const;
		bool NeedStagingCopy() const;
		VkPipelineStageFlags GetUsedPipelineStages() const;

	private:
		friend class Util::ObjectPool<LinearHostImage>;
		LinearHostImage(Device* device, ImageHandle gpu_image, Util::IntrusivePtr<Buffer> cpu_image, VkPipelineStageFlags stages);
		Device* device;
		ImageHandle gpu_image;
		Util::IntrusivePtr<Buffer> cpu_image;
		VkPipelineStageFlags stages;
		size_t row_pitch;
		size_t row_offset;
	};
	using LinearHostImageHandle = Util::IntrusivePtr<LinearHostImage>;

	////////////////////////////////
	////////////////////////////////
	////////////////////////////////

}