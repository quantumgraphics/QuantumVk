#pragma once

// NOTE: The Vulkan symbols are function pointers and are provided by the "volk" project.

#include <volk/volk.h>
#include <cstdlib>
#include <stdexcept>

#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
// Workaround silly Xlib headers that define macros for these globally :(
#undef None
#undef Bool
#endif

#ifdef VULKAN_DEBUG
#define VK_ASSERT(x) ((void)0) //figure out asserts
#else
#define VK_ASSERT(x) ((void)0)
#endif

namespace Vulkan
{
	struct NoCopyNoMove
	{
		NoCopyNoMove() = default;
		NoCopyNoMove(const NoCopyNoMove&) = delete;
		void operator=(const NoCopyNoMove&) = delete;
	};
}