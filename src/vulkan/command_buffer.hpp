#pragma once

#include "buffer.hpp"
#include "buffer_pool.hpp"
#include "vulkan_headers.hpp"
#include "image.hpp"
#include "pipeline_event.hpp"
#include "query_pool.hpp"
#include "render_pass.hpp"
#include "sampler.hpp"
#include "shader.hpp"
#include "vulkan_common.hpp"
#include <string.h>

namespace Vulkan
{
	class DebugChannelInterface;

	enum CommandBufferDirtyBits
	{
		COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT = 1 << 0,
		COMMAND_BUFFER_DIRTY_PIPELINE_BIT = 1 << 1,

		COMMAND_BUFFER_DIRTY_VIEWPORT_BIT = 1 << 2,
		COMMAND_BUFFER_DIRTY_SCISSOR_BIT = 1 << 3,
		COMMAND_BUFFER_DIRTY_DEPTH_BIAS_BIT = 1 << 4,
		COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT = 1 << 5,

		COMMAND_BUFFER_DIRTY_STATIC_VERTEX_BIT = 1 << 6,

		COMMAND_BUFFER_DIRTY_PUSH_CONSTANTS_BIT = 1 << 7,

		COMMAND_BUFFER_DYNAMIC_BITS = COMMAND_BUFFER_DIRTY_VIEWPORT_BIT | COMMAND_BUFFER_DIRTY_SCISSOR_BIT |
		COMMAND_BUFFER_DIRTY_DEPTH_BIAS_BIT |
		COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT
	};
	using CommandBufferDirtyFlags = uint32_t;

#define COMPARE_OP_BITS 3
#define STENCIL_OP_BITS 3
#define BLEND_FACTOR_BITS 5
#define BLEND_OP_BITS 3
#define CULL_MODE_BITS 2
#define FRONT_FACE_BITS 1

	union PipelineState {
		struct
		{
			// Depth state.
			unsigned depth_write : 1;
			unsigned depth_test : 1;
			unsigned blend_enable : 1;

			unsigned cull_mode : CULL_MODE_BITS;
			unsigned front_face : FRONT_FACE_BITS;
			unsigned depth_bias_enable : 1;

			unsigned depth_compare : COMPARE_OP_BITS;

			unsigned stencil_test : 1;
			unsigned stencil_front_fail : STENCIL_OP_BITS;
			unsigned stencil_front_pass : STENCIL_OP_BITS;
			unsigned stencil_front_depth_fail : STENCIL_OP_BITS;
			unsigned stencil_front_compare_op : COMPARE_OP_BITS;
			unsigned stencil_back_fail : STENCIL_OP_BITS;
			unsigned stencil_back_pass : STENCIL_OP_BITS;
			unsigned stencil_back_depth_fail : STENCIL_OP_BITS;
			unsigned stencil_back_compare_op : COMPARE_OP_BITS;

			unsigned alpha_to_coverage : 1;
			unsigned alpha_to_one : 1;
			unsigned sample_shading : 1;

			unsigned src_color_blend : BLEND_FACTOR_BITS;
			unsigned dst_color_blend : BLEND_FACTOR_BITS;
			unsigned color_blend_op : BLEND_OP_BITS;
			unsigned src_alpha_blend : BLEND_FACTOR_BITS;
			unsigned dst_alpha_blend : BLEND_FACTOR_BITS;
			unsigned alpha_blend_op : BLEND_OP_BITS;
			unsigned primitive_restart : 1;
			unsigned topology : 4;

			unsigned wireframe : 1;
			unsigned subgroup_control_size : 1;
			unsigned subgroup_full_group : 1;
			unsigned subgroup_minimum_size_log2 : 3;
			unsigned subgroup_maximum_size_log2 : 3;
			unsigned conservative_raster : 1;

			uint32_t write_mask;
		} state;
		uint32_t words[4];
	};

	struct PotentialState
	{
		float blend_constants[4];
		uint32_t spec_constants[VULKAN_NUM_SPEC_CONSTANTS];
		uint8_t spec_constant_mask;
	};

	struct DynamicState
	{
		float depth_bias_constant = 0.0f;
		float depth_bias_slope = 0.0f;
		uint8_t front_compare_mask = 0;
		uint8_t front_write_mask = 0;
		uint8_t front_reference = 0;
		uint8_t back_compare_mask = 0;
		uint8_t back_write_mask = 0;
		uint8_t back_reference = 0;
	};

	struct VertexAttribState
	{
		uint32_t binding;
		VkFormat format;
		uint32_t offset;
	};

	struct IndexState
	{
		VkBuffer buffer;
		VkDeviceSize offset;
		VkIndexType index_type;
	};

	struct VertexBindingState
	{
		VkBuffer buffers[VULKAN_NUM_VERTEX_BUFFERS];
		VkDeviceSize offsets[VULKAN_NUM_VERTEX_BUFFERS];
	};

	enum CommandBufferSavedStateBits
	{
		COMMAND_BUFFER_SAVED_BINDINGS_0_BIT = 1u << 0,
		COMMAND_BUFFER_SAVED_BINDINGS_1_BIT = 1u << 1,
		COMMAND_BUFFER_SAVED_BINDINGS_2_BIT = 1u << 2,
		COMMAND_BUFFER_SAVED_BINDINGS_3_BIT = 1u << 3,
		COMMAND_BUFFER_SAVED_BINDINGS_4_BIT = 1u << 4,
		COMMAND_BUFFER_SAVED_BINDINGS_5_BIT = 1u << 5,
		COMMAND_BUFFER_SAVED_BINDINGS_6_BIT = 1u << 6,
		COMMAND_BUFFER_SAVED_BINDINGS_7_BIT = 1u << 7,
		COMMAND_BUFFER_SAVED_VIEWPORT_BIT = 1u << 8,
		COMMAND_BUFFER_SAVED_SCISSOR_BIT = 1u << 9,
		COMMAND_BUFFER_SAVED_RENDER_STATE_BIT = 1u << 10,
		COMMAND_BUFFER_SAVED_PUSH_CONSTANT_BIT = 1u << 11
	};
	static_assert(VULKAN_NUM_DESCRIPTOR_SETS == 8, "Number of descriptor sets != 8.");
	using CommandBufferSaveStateFlags = uint32_t;

	struct CommandBufferSavedState
	{
		CommandBufferSaveStateFlags flags = 0;
		ResourceBindings bindings;
		VkViewport viewport;
		VkRect2D scissor;

		PipelineState static_state;
		PotentialState potential_static_state;
		DynamicState dynamic_state;
	};

	struct DeferredPipelineCompile
	{
		Program* program;
		const RenderPass* compatible_render_pass;
		PipelineState static_state;
		PotentialState potential_static_state;
		VertexAttribState attribs[VULKAN_NUM_VERTEX_ATTRIBS];
		VkDeviceSize strides[VULKAN_NUM_VERTEX_BUFFERS];
		VkVertexInputRate input_rates[VULKAN_NUM_VERTEX_BUFFERS];

		unsigned subpass_index;
		Util::Hash hash;
		VkPipelineCache cache;
	};

	class CommandBuffer;
	//Functor to delete command buffer
	struct CommandBufferDeleter
	{
		void operator()(CommandBuffer* cmd);
	};

	class Device;
	class CommandBuffer : public Util::IntrusivePtrEnabled<CommandBuffer, CommandBufferDeleter, HandleCounter>
	{
	public:
		friend struct CommandBufferDeleter;
		//Type of Command buffer
		enum class Type
		{
			Generic,
			AsyncGraphics,
			AsyncCompute,
			AsyncTransfer,
			Count
		};

		~CommandBuffer();
		VkCommandBuffer GetCommandBuffer() const
		{
			return cmd;
		}

		void BeginRegion(const char* name, const float* color = nullptr);
		void EndRegion();

		Device& GetDevice()
		{
			return *device;
		}

		bool SwapchainTouched() const
		{
			return uses_swapchain;
		}

		void SetThreadIndex(unsigned index_)
		{
			thread_index = index_;
		}

		unsigned GetThreadIndex() const
		{
			return thread_index;
		}

		void SetIsSecondary()
		{
			is_secondary = true;
		}

		bool GetIsSecondary() const
		{
			return is_secondary;
		}

		void ClearImage(const Image& image, const VkClearValue& value);
		void ClearImage(const Image& image, const VkClearValue& value, VkImageAspectFlags aspect);
		void ClearQuad(unsigned attachment, const VkClearRect& rect, const VkClearValue& value,
			VkImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
		void ClearQuad(const VkClearRect& rect, const VkClearAttachment* attachments, unsigned num_attachments);

		void FillBuffer(const Buffer& dst, uint32_t value);
		void FillBuffer(const Buffer& dst, uint32_t value, VkDeviceSize offset, VkDeviceSize size);
		void CopyBuffer(const Buffer& dst, VkDeviceSize dst_offset, const Buffer& src, VkDeviceSize src_offset,
			VkDeviceSize size);
		void CopyBuffer(const Buffer& dst, const Buffer& src);
		void CopyBuffer(const Buffer& dst, const Buffer& src, const VkBufferCopy* copies, size_t count);
		void CopyImage(const Image& dst, const Image& src);
		void CopyImage(const Image& dst, const Image& src,
			const VkOffset3D& dst_offset, const VkOffset3D& src_offset,
			const VkExtent3D& extent,
			const VkImageSubresourceLayers& dst_subresource,
			const VkImageSubresourceLayers& src_subresource);

		void CopyBufferToImage(const Image& image, const Buffer& buffer, VkDeviceSize buffer_offset,
			const VkOffset3D& offset, const VkExtent3D& extent, unsigned row_length,
			unsigned slice_height, const VkImageSubresourceLayers& subresrouce);
		void CopyBufferToImage(const Image& image, const Buffer& buffer, unsigned num_blits, const VkBufferImageCopy* blits);

		void CopyBufferToImage(const Buffer& buffer, const Image& image, unsigned num_blits, const VkBufferImageCopy* blits);
		void CopyBufferToImage(const Buffer& dst, const Image& src, VkDeviceSize buffer_offset, const VkOffset3D& offset,
			const VkExtent3D& extent, unsigned row_length, unsigned slice_height,
			const VkImageSubresourceLayers& subresrouce);

		void FullBarrier();
		void PixelBarrier();
		void Barrier(VkPipelineStageFlags src_stage, VkAccessFlags src_access, VkPipelineStageFlags dst_stage,
			VkAccessFlags dst_access);

		PipelineEvent SignalEvent(VkPipelineStageFlags stages);
		void WaitEvents(unsigned num_events, const VkEvent* events,
			VkPipelineStageFlags src_stages, VkPipelineStageFlags dst_stages,
			unsigned barriers, const VkMemoryBarrier* globals,
			unsigned buffer_barriers, const VkBufferMemoryBarrier* buffers,
			unsigned image_barriers, const VkImageMemoryBarrier* images);

		void Barrier(VkPipelineStageFlags src_stages, VkPipelineStageFlags dst_stages,
			unsigned barriers, const VkMemoryBarrier* globals,
			unsigned buffer_barriers, const VkBufferMemoryBarrier* buffers,
			unsigned image_barriers, const VkImageMemoryBarrier* images);

		void BufferBarrier(const Buffer& buffer, VkPipelineStageFlags src_stage, VkAccessFlags src_access,
			VkPipelineStageFlags dst_stage, VkAccessFlags dst_access);

		void ImageBarrier(const Image& image, VkImageLayout old_layout, VkImageLayout new_layout,
			VkPipelineStageFlags src_stage, VkAccessFlags src_access, VkPipelineStageFlags dst_stage,
			VkAccessFlags dst_access);

		void BlitImage(const Image& dst,
			const Image& src,
			const VkOffset3D& dst_offset0, const VkOffset3D& dst_extent,
			const VkOffset3D& src_offset0, const VkOffset3D& src_extent, unsigned dst_level, unsigned src_level,
			unsigned dst_base_layer = 0, uint32_t src_base_layer = 0, unsigned num_layers = 1,
			VkFilter filter = VK_FILTER_LINEAR);

		// Prepares an image to have its mipmap generated.
		// Puts the top-level into TRANSFER_SRC_OPTIMAL, and all other levels are invalidated with an UNDEFINED -> TRANSFER_DST_OPTIMAL.
		void BarrierPrepareGenerateMipmap(const Image& image, VkImageLayout base_level_layout, VkPipelineStageFlags src_stage, VkAccessFlags src_access,
			bool need_top_level_barrier = true);

		// The image must have been transitioned with barrier_prepare_generate_mipmap before calling this function.
		// After calling this function, the image will be entirely in TRANSFER_SRC_OPTIMAL layout.
		// Wait for TRANSFER stage to drain before transitioning away from TRANSFER_SRC_OPTIMAL.
		void GenerateMipmap(const Image& image);

		void BeginRenderPass(const RenderPassInfo& info, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
		void NextSubpass(VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
		void EndRenderPass();
		void SubmitSecondary(Util::IntrusivePtr<CommandBuffer> secondary);
		inline unsigned GetCurrentSubpass() const
		{
			return pipeline_state.subpass_index;
		}
		Util::IntrusivePtr<CommandBuffer> RequestSecondaryCommandBuffer(unsigned thread_index, unsigned subpass);
		static Util::IntrusivePtr<CommandBuffer> RequestSecondaryCommandBuffer(Device& device,
			const RenderPassInfo& rp, unsigned thread_index, unsigned subpass);

		void SetProgram(Program* program);

#ifdef QM_VULKAN_FILESYSTEM
		// Convenience functions for one-off shader binds.
		void set_program(const std::string& vertex, const std::string& fragment, const std::vector<std::pair<std::string, int>>& defines = {});
		void set_program(const std::string& compute, const std::vector<std::pair<std::string, int>>& defines = {});
#endif

		void SetBufferView(unsigned set, unsigned binding, const BufferView& view);
		void SetInputAttachments(unsigned set, unsigned start_binding);
		void SetTexture(unsigned set, unsigned binding, const ImageView& view);
		void SetUnormTexture(unsigned set, unsigned binding, const ImageView& view);
		void SetSrgbTexture(unsigned set, unsigned binding, const ImageView& view);
		void SetTexture(unsigned set, unsigned binding, const ImageView& view, const Sampler& sampler);
		void SetTexture(unsigned set, unsigned binding, const ImageView& view, StockSampler sampler);
		void SetStorageTexture(unsigned set, unsigned binding, const ImageView& view);
		void SetSampler(unsigned set, unsigned binding, const Sampler& sampler);
		void SetSampler(unsigned set, unsigned binding, StockSampler sampler);
		void SetUniformBuffer(unsigned set, unsigned binding, const Buffer& buffer);
		void SetUniformBuffer(unsigned set, unsigned binding, const Buffer& buffer, VkDeviceSize offset,
			VkDeviceSize range);
		void SetStorageBuffer(unsigned set, unsigned binding, const Buffer& buffer);
		void SetStorageBuffer(unsigned set, unsigned binding, const Buffer& buffer, VkDeviceSize offset,
			VkDeviceSize range);

		void SetBindless(unsigned set, VkDescriptorSet desc_set);

		void PushConstants(const void* data, VkDeviceSize offset, VkDeviceSize range);

		void* AllocateConstantData(unsigned set, unsigned binding, VkDeviceSize size);

		template <typename T>
		T* AllocateTypedConstantData(unsigned set, unsigned binding, unsigned count)
		{
			return static_cast<T*>(AllocateConstantData(set, binding, count * sizeof(T)));
		}

		void* AllocateVertexData(unsigned binding, VkDeviceSize size, VkDeviceSize stride,
			VkVertexInputRate step_rate = VK_VERTEX_INPUT_RATE_VERTEX);
		void* AllocateIndexData(VkDeviceSize size, VkIndexType index_type);

		void* UpdateBuffer(const Buffer& buffer, VkDeviceSize offset, VkDeviceSize size);
		void* UpdateImage(const Image& image, const VkOffset3D& offset, const VkExtent3D& extent, uint32_t row_length,
			uint32_t image_height, const VkImageSubresourceLayers& subresource);
		void* UpdateImage(const Image& image, uint32_t row_length = 0, uint32_t image_height = 0);

		void SetViewport(const VkViewport& viewport);
		const VkViewport& get_viewport() const;
		void SetScissor(const VkRect2D& rect);

		void SetVertexAttrib(uint32_t attrib, uint32_t binding, VkFormat format, VkDeviceSize offset);
		void SetVertexBinding(uint32_t binding, const Buffer& buffer, VkDeviceSize offset, VkDeviceSize stride, VkVertexInputRate step_rate = VK_VERTEX_INPUT_RATE_VERTEX);
		void SetIndexBuffer(const Buffer& buffer, VkDeviceSize offset, VkIndexType index_type);

		void Draw(uint32_t vertex_count, uint32_t instance_count = 1, uint32_t first_vertex = 0, uint32_t first_instance = 0);
		void DrawIndexed(uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0, int32_t vertex_offset = 0, uint32_t first_instance = 0);

		void Dispatch(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z);

		void DrawIndirect(const Buffer& buffer, uint32_t offset, uint32_t draw_count, uint32_t stride);
		void DrawIndexedIndirect(const Buffer& buffer, uint32_t offset, uint32_t draw_count, uint32_t stride);
		void DrawMultiIndirect(const Buffer& buffer, uint32_t offset, uint32_t draw_count, uint32_t stride, const Buffer& count, uint32_t count_offset);
		void DrawIndexedMultiIndirect(const Buffer& buffer, uint32_t offset, uint32_t draw_count, uint32_t stride, const Buffer& count, uint32_t count_offset);
		void dispatch_indirect(const Buffer& buffer, uint32_t offset);

		void SetOpaqueState();
		void SetQuadState();
		void SetOpaqueSpriteState();
		void SetTransparentSpriteState();

		void SaveState(CommandBufferSaveStateFlags flags, CommandBufferSavedState& state);
		void RestoreState(const CommandBufferSavedState& state);

#define SET_STATIC_STATE(value)                               \
	do                                                        \
	{                                                         \
		if (pipeline_state.static_state.state.value != value) \
		{                                                     \
			pipeline_state.static_state.state.value = value;  \
			set_dirty(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT); \
		}                                                     \
	} while (0)

#define SET_POTENTIALLY_STATIC_STATE(value)                       \
	do                                                            \
	{                                                             \
		if (pipeline_state.potential_static_state.value != value) \
		{                                                         \
			pipeline_state.potential_static_state.value = value;  \
			set_dirty(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT);     \
		}                                                         \
	} while (0)

		inline void SetDepthTest(bool depth_test, bool depth_write)
		{
			SET_STATIC_STATE(depth_test);
			SET_STATIC_STATE(depth_write);
		}

		inline void SetWireframe(bool wireframe)
		{
			SET_STATIC_STATE(wireframe);
		}

		inline void SetDepthCompare(VkCompareOp depth_compare)
		{
			SET_STATIC_STATE(depth_compare);
		}

		inline void SetBlendEnable(bool blend_enable)
		{
			SET_STATIC_STATE(blend_enable);
		}

		inline void SetBlendFactors(VkBlendFactor src_color_blend, VkBlendFactor src_alpha_blend, VkBlendFactor dst_color_blend, VkBlendFactor dst_alpha_blend)
		{
			SET_STATIC_STATE(src_color_blend);
			SET_STATIC_STATE(dst_color_blend);
			SET_STATIC_STATE(src_alpha_blend);
			SET_STATIC_STATE(dst_alpha_blend);
		}

		inline void SetBlendFactors(VkBlendFactor src_blend, VkBlendFactor dst_blend)
		{
			SetBlendFactors(src_blend, src_blend, dst_blend, dst_blend);
		}

		inline void SetBlendOp(VkBlendOp color_blend_op, VkBlendOp alpha_blend_op)
		{
			SET_STATIC_STATE(color_blend_op);
			SET_STATIC_STATE(alpha_blend_op);
		}

		inline void SetBlendOp(VkBlendOp blend_op)
		{
			SetBlendOp(blend_op, blend_op);
		}

		inline void SetDepthBias(bool depth_bias_enable)
		{
			SET_STATIC_STATE(depth_bias_enable);
		}

		inline void SetColorWriteMask(uint32_t write_mask)
		{
			SET_STATIC_STATE(write_mask);
		}

		inline void SetStencilTest(bool stencil_test)
		{
			SET_STATIC_STATE(stencil_test);
		}

		inline void SetStencilFrontOps(VkCompareOp stencil_front_compare_op, VkStencilOp stencil_front_pass, VkStencilOp stencil_front_fail, VkStencilOp stencil_front_depth_fail)
		{
			SET_STATIC_STATE(stencil_front_compare_op);
			SET_STATIC_STATE(stencil_front_pass);
			SET_STATIC_STATE(stencil_front_fail);
			SET_STATIC_STATE(stencil_front_depth_fail);
		}

		inline void SetStencilBackOps(VkCompareOp stencil_back_compare_op, VkStencilOp stencil_back_pass, VkStencilOp stencil_back_fail, VkStencilOp stencil_back_depth_fail)
		{
			SET_STATIC_STATE(stencil_back_compare_op);
			SET_STATIC_STATE(stencil_back_pass);
			SET_STATIC_STATE(stencil_back_fail);
			SET_STATIC_STATE(stencil_back_depth_fail);
		}

		inline void SetStencilOps(VkCompareOp stencil_compare_op, VkStencilOp stencil_pass, VkStencilOp stencil_fail,
			VkStencilOp stencil_depth_fail)
		{
			SetStencilFrontOps(stencil_compare_op, stencil_pass, stencil_fail, stencil_depth_fail);
			SetStencilBackOps(stencil_compare_op, stencil_pass, stencil_fail, stencil_depth_fail);
		}

		inline void SetPrimitiveTopology(VkPrimitiveTopology topology)
		{
			SET_STATIC_STATE(topology);
		}

		inline void SetPrimitiveRestart(bool primitive_restart)
		{
			SET_STATIC_STATE(primitive_restart);
		}

		inline void SetMultisampleState(bool alpha_to_coverage, bool alpha_to_one = false, bool sample_shading = false)
		{
			SET_STATIC_STATE(alpha_to_coverage);
			SET_STATIC_STATE(alpha_to_one);
			SET_STATIC_STATE(sample_shading);
		}

		inline void SetFrontFace(VkFrontFace front_face)
		{
			SET_STATIC_STATE(front_face);
		}

		inline void SetCullMode(VkCullModeFlags cull_mode)
		{
			SET_STATIC_STATE(cull_mode);
		}

		inline void SetBlendConstants(const float blend_constants[4])
		{
			SET_POTENTIALLY_STATIC_STATE(blend_constants[0]);
			SET_POTENTIALLY_STATIC_STATE(blend_constants[1]);
			SET_POTENTIALLY_STATIC_STATE(blend_constants[2]);
			SET_POTENTIALLY_STATIC_STATE(blend_constants[3]);
		}

		inline void SetSpecializationConstantMask(uint32_t spec_constant_mask)
		{
			VK_ASSERT((spec_constant_mask & ~((1u << VULKAN_NUM_SPEC_CONSTANTS) - 1u)) == 0u);
			SET_POTENTIALLY_STATIC_STATE(spec_constant_mask);
		}

		template <typename T>
		inline void SetSpecializationConstant(unsigned index, const T& value)
		{
			VK_ASSERT(index < VULKAN_NUM_SPEC_CONSTANTS);
			static_assert(sizeof(value) == sizeof(uint32_t), "Spec constant data must be 32-bit.");
			if (memcmp(&pipeline_state.potential_static_state.spec_constants[index], &value, sizeof(value)))
			{
				memcpy(&pipeline_state.potential_static_state.spec_constants[index], &value, sizeof(value));
				if (pipeline_state.potential_static_state.spec_constant_mask & (1u << index))
					set_dirty(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT);
			}
		}

		inline void EnableSubgroupSizeControl(bool subgroup_control_size)
		{
			SET_STATIC_STATE(subgroup_control_size);
		}

		inline void SetSubgroupSizeLog2(bool subgroup_full_group, uint8_t subgroup_minimum_size_log2, uint8_t subgroup_maximum_size_log2)
		{
			VK_ASSERT(subgroup_minimum_size_log2 < 8);
			VK_ASSERT(subgroup_maximum_size_log2 < 8);
			SET_STATIC_STATE(subgroup_full_group);
			SET_STATIC_STATE(subgroup_minimum_size_log2);
			SET_STATIC_STATE(subgroup_maximum_size_log2);
		}

		inline void SetConservativeRasterization(bool conservative_raster)
		{
			SET_STATIC_STATE(conservative_raster);
		}

#define SET_DYNAMIC_STATE(state, flags)   \
	do                                    \
	{                                     \
		if (dynamic_state.state != state) \
		{                                 \
			dynamic_state.state = state;  \
			set_dirty(flags);             \
		}                                 \
	} while (0)

		inline void SetDepthBias(float depth_bias_constant, float depth_bias_slope)
		{
			SET_DYNAMIC_STATE(depth_bias_constant, COMMAND_BUFFER_DIRTY_DEPTH_BIAS_BIT);
			SET_DYNAMIC_STATE(depth_bias_slope, COMMAND_BUFFER_DIRTY_DEPTH_BIAS_BIT);
		}

		inline void SetStencilFrontReference(uint8_t front_compare_mask, uint8_t front_write_mask,
			uint8_t front_reference)
		{
			SET_DYNAMIC_STATE(front_compare_mask, COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT);
			SET_DYNAMIC_STATE(front_write_mask, COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT);
			SET_DYNAMIC_STATE(front_reference, COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT);
		}

		inline void SetStencilBackReference(uint8_t back_compare_mask, uint8_t back_write_mask, uint8_t back_reference)
		{
			SET_DYNAMIC_STATE(back_compare_mask, COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT);
			SET_DYNAMIC_STATE(back_write_mask, COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT);
			SET_DYNAMIC_STATE(back_reference, COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT);
		}

		inline void SetStencilReference(uint8_t compare_mask, uint8_t write_mask, uint8_t reference)
		{
			SetStencilFrontReference(compare_mask, write_mask, reference);
			SetStencilReference(compare_mask, write_mask, reference);
		}

		inline Type GetCommandBufferType() const
		{
			return type;
		}

		QueryPoolHandle WriteTimestamp(VkPipelineStageFlagBits stage);
		void AddCheckpoint(const char* tag);
		void SetBacktraceCheckpoint();

		void End();
		void EnableProfiling();
		bool HasProfiling() const;

		void BeginDebugChannel(DebugChannelInterface* iface, const char* tag, VkDeviceSize size);
		void EndDebugChannel();

		void ExtractPipelineState(DeferredPipelineCompile& compile) const;
		static VkPipeline BuildGraphicsPipeline(Device* device, const DeferredPipelineCompile& compile);
		static VkPipeline BuildComputePipeline(Device* device, const DeferredPipelineCompile& compile);

		bool FlushPipelineStateWithoutBlocking();

	private:
		friend class Util::ObjectPool<CommandBuffer>;
		CommandBuffer(Device* device, VkCommandBuffer cmd, VkPipelineCache cache, Type type);

		Device* device;
		const VolkDeviceTable& table;
		VkCommandBuffer cmd;
		Type type;

		const Framebuffer* framebuffer = nullptr;
		const RenderPass* actual_render_pass = nullptr;
		const Vulkan::ImageView* framebuffer_attachments[VULKAN_NUM_ATTACHMENTS + 1] = {};

		IndexState index_state = {};
		VertexBindingState vbo = {};
		ResourceBindings bindings;
		VkDescriptorSet bindless_sets[VULKAN_NUM_DESCRIPTOR_SETS] = {};
		VkDescriptorSet allocated_sets[VULKAN_NUM_DESCRIPTOR_SETS] = {};

		VkPipeline current_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout current_pipeline_layout = VK_NULL_HANDLE;
		PipelineLayout* current_layout = nullptr;
		VkSubpassContents current_contents = VK_SUBPASS_CONTENTS_INLINE;
		unsigned thread_index = 0;

		VkViewport viewport = {};
		VkRect2D scissor = {};

		CommandBufferDirtyFlags dirty = ~0u;
		uint32_t dirty_sets = 0;
		uint32_t dirty_sets_dynamic = 0;
		uint32_t dirty_vbos = 0;
		uint32_t active_vbos = 0;
		bool uses_swapchain = false;
		bool is_compute = true;
		bool is_secondary = false;

		void set_dirty(CommandBufferDirtyFlags flags)
		{
			dirty |= flags;
		}

		CommandBufferDirtyFlags get_and_clear(CommandBufferDirtyFlags flags)
		{
			auto mask = dirty & flags;
			dirty &= ~flags;
			return mask;
		}

		DeferredPipelineCompile pipeline_state = {};
		DynamicState dynamic_state = {};
#ifndef _MSC_VER
		static_assert(sizeof(pipeline_state.static_state.words) >= sizeof(pipeline_state.static_state.state),
			"Hashable pipeline state is not large enough!");
#endif

		bool flush_render_state(bool synchronous);
		bool flush_compute_state(bool synchronous);
		void clear_render_state();

		bool flush_graphics_pipeline(bool synchronous);
		bool flush_compute_pipeline(bool synchronous);
		void flush_descriptor_sets();
		void begin_graphics();
		void flush_descriptor_set(uint32_t set);
		void rebind_descriptor_set(uint32_t set);
		void begin_compute();
		void begin_context();

		BufferBlock vbo_block;
		BufferBlock ibo_block;
		BufferBlock ubo_block;
		BufferBlock staging_block;

		void set_texture(unsigned set, unsigned binding, VkImageView float_view, VkImageView integer_view,
			VkImageLayout layout,
			uint64_t cookie);

		void init_viewport_scissor(const RenderPassInfo& info, const Framebuffer* framebuffer);

		bool profiling = false;
		std::string debug_channel_tag;
		Vulkan::BufferHandle debug_channel_buffer;
		DebugChannelInterface* debug_channel_interface = nullptr;

		static void update_hash_graphics_pipeline(DeferredPipelineCompile& compile, uint32_t& active_vbos);
		static void update_hash_compute_pipeline(DeferredPipelineCompile& compile);
	};

#ifdef QM_VULKAN_FILESYSTEM
	struct CommandBufferUtil
	{
		static void draw_fullscreen_quad(CommandBuffer& cmd, const std::string& vertex, const std::string& fragment,
			const std::vector<std::pair<std::string, int>>& defines = {});
		static void draw_fullscreen_quad_depth(CommandBuffer& cmd, const std::string& vertex, const std::string& fragment,
			bool depth_test, bool depth_write, VkCompareOp depth_compare,
			const std::vector<std::pair<std::string, int>>& defines = {});
		static void set_fullscreen_quad_vertex_state(CommandBuffer& cmd);
		static void set_quad_vertex_state(CommandBuffer& cmd);

		static void setup_fullscreen_quad(CommandBuffer& cmd, const std::string& vertex, const std::string& fragment,
			const std::vector<std::pair<std::string, int>>& defines = {},
			bool depth_test = false, bool depth_write = false,
			VkCompareOp depth_compare = VK_COMPARE_OP_ALWAYS);

		static void draw_fullscreen_quad(CommandBuffer& cmd, unsigned instances = 1);
		static void draw_quad(CommandBuffer& cmd, unsigned instances = 1);
	};
#endif

	using CommandBufferHandle = Util::IntrusivePtr<CommandBuffer>;

	constexpr int size = sizeof(CommandBuffer);
}
