#pragma once

#include "utils/hash.hpp"
#include "utils/object_pool.hpp"
#include "utils/temporary_hashmap.hpp"
#include "vulkan_headers.hpp"
#include "sampler.hpp"
#include "limits.hpp"
#include <utility>
#include <vector>
#include "cookie.hpp"

namespace Vulkan
{
	//Forward declare Device
	class Device;
	//Descriptor set layout
	struct DescriptorSetLayout
	{
		//Location of all sampled images
		uint32_t sampled_image_mask = 0;
		//Location of all storage images
		uint32_t storage_image_mask = 0;
		//Location of all uniform buffers
		uint32_t uniform_buffer_mask = 0;
		//Location of all storage buffers
		uint32_t storage_buffer_mask = 0;
		//Location of all texel buffer views
		uint32_t sampled_buffer_mask = 0;
		//Location of input attachments
		uint32_t input_attachment_mask = 0;
		//Location of all samplers
		uint32_t sampler_mask = 0;
		//Location of non combined images
		uint32_t separate_image_mask = 0;
		uint32_t fp_mask = 0;
		//Location of immutable samplers
		uint32_t immutable_sampler_mask = 0;
		//Type of each immutable sampler
		uint64_t immutable_samplers = 0;
		//Size of array at each binding
		uint8_t array_size[VULKAN_NUM_BINDINGS] = {};
		enum { UNSIZED_ARRAY = 0xff };
	};

	// Avoid -Wclass-memaccess warnings since we hash DescriptorSetLayout.

	//Returns whether the set layout has an immutable sampler at binding
	static inline bool HasImmutableSampler(const DescriptorSetLayout& layout, unsigned binding)
	{
		return (layout.immutable_sampler_mask & (1u << binding)) != 0;
	}

	//Returns immutable sampler type at binding
	static inline StockSampler GetImmutableSampler(const DescriptorSetLayout& layout, unsigned binding)
	{
		VK_ASSERT(HasImmutableSampler(layout, binding));
		return static_cast<StockSampler>((layout.immutable_samplers >> (4 * binding)) & 0xf);
	}

	//Sets immutable sampler type at binding
	static inline void SetImmutableSampler(DescriptorSetLayout& layout, unsigned binding, StockSampler sampler)
	{
		layout.immutable_samplers |= uint64_t(sampler) << (4 * binding);
		layout.immutable_sampler_mask |= 1u << binding;
	}

	static const unsigned VULKAN_NUM_SETS_PER_POOL = 16;
	static const unsigned VULKAN_DESCRIPTOR_RING_SIZE = 8;

	class DescriptorSetAllocator;
	class BindlessDescriptorPool;
	class ImageView;

	struct BindlessDescriptorPoolDeleter
	{
		void operator()(BindlessDescriptorPool* pool);
	};

	class BindlessDescriptorPool : public Util::IntrusivePtrEnabled<BindlessDescriptorPool, BindlessDescriptorPoolDeleter, HandleCounter>, public InternalSyncEnabled
	{
	public:
		friend struct BindlessDescriptorPoolDeleter;

		explicit BindlessDescriptorPool(Device* device, DescriptorSetAllocator* allocator, VkDescriptorPool pool);
		~BindlessDescriptorPool();

		void operator=(const BindlessDescriptorPool&) = delete;
		BindlessDescriptorPool(const BindlessDescriptorPool&) = delete;

		bool allocate_descriptors(unsigned count);
		VkDescriptorSet get_descriptor_set() const;

		void set_texture(unsigned binding, const ImageView& view);
		void set_texture_unorm(unsigned binding, const ImageView& view);
		void set_texture_srgb(unsigned binding, const ImageView& view);

	private:
		Device* device;
		DescriptorSetAllocator* allocator;
		VkDescriptorPool desc_pool;
		VkDescriptorSet desc_set = VK_NULL_HANDLE;

		void set_texture(unsigned binding, VkImageView view, VkImageLayout layout);
	};
	using BindlessDescriptorPoolHandle = Util::IntrusivePtr<BindlessDescriptorPool>;

	enum class BindlessResourceType
	{
		ImageFP,
		ImageInt
	};

	class DescriptorSetAllocator : public HashedObject<DescriptorSetAllocator>
	{
	public:
		DescriptorSetAllocator(Util::Hash hash, Device* device, const DescriptorSetLayout& layout, const uint32_t* stages_for_bindings);
		~DescriptorSetAllocator();

		void operator=(const DescriptorSetAllocator&) = delete;
		DescriptorSetAllocator(const DescriptorSetAllocator&) = delete;

		void begin_frame();
		std::pair<VkDescriptorSet, bool> find(unsigned thread_index, Util::Hash hash);

		VkDescriptorSetLayout get_layout() const
		{
			return set_layout;
		}

		void clear();

		bool is_bindless() const
		{
			return bindless;
		}

		VkDescriptorPool allocate_bindless_pool(unsigned num_sets, unsigned num_descriptors);
		VkDescriptorSet allocate_bindless_set(VkDescriptorPool pool, unsigned num_descriptors);

	private:
		struct DescriptorSetNode : Util::TemporaryHashmapEnabled<DescriptorSetNode>, Util::IntrusiveListEnabled<DescriptorSetNode>
		{
			explicit DescriptorSetNode(VkDescriptorSet set_)
				: set(set_)
			{
			}

			VkDescriptorSet set;
		};

		Device* device;
		const VolkDeviceTable& table;
		VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;

		struct PerThread
		{
			Util::TemporaryHashmap<DescriptorSetNode, VULKAN_DESCRIPTOR_RING_SIZE, true> set_nodes;
			std::vector<VkDescriptorPool> pools;
			bool should_begin = true;
		};
		std::vector<std::unique_ptr<PerThread>> per_thread;
		std::vector<VkDescriptorPoolSize> pool_size;
		bool bindless = false;
	};
}