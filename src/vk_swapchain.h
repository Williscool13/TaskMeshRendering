//
// Created by William on 2025-10-10.
//

#ifndef WILLENGINETESTBED_SWAPCHAIN_H
#define WILLENGINETESTBED_SWAPCHAIN_H
#include <vector>
#include <volk/volk.h>

#include "VkBootstrap.h"

struct VulkanContext;

inline static constexpr bool ENABLE_HDR = false;
inline static constexpr VkFormat SWAPCHAIN_HDR_IMAGE_FORMAT = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
inline static constexpr VkColorSpaceKHR SWAPCHAIN_HDR_COLOR_SPACE = VK_COLOR_SPACE_HDR10_ST2084_EXT;
inline static constexpr VkFormat SWAPCHAIN_SDR_IMAGE_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
inline static constexpr VkColorSpaceKHR SWAPCHAIN_SDR_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

struct Swapchain
{
    Swapchain() = delete;

    explicit Swapchain(const VulkanContext* context, uint32_t width, uint32_t height);

    ~Swapchain();

    void Create(uint32_t width, uint32_t height);

    void Recreate(uint32_t width, uint32_t height);

    void Dump();

    [[nodiscard]] bool IsHDR() const { return format == SWAPCHAIN_HDR_IMAGE_FORMAT && colorSpace == SWAPCHAIN_HDR_COLOR_SPACE; }

    VkSwapchainKHR handle{};
    VkFormat format{};
    VkColorSpaceKHR colorSpace{};
    VkExtent2D extent{};
    uint32_t imageCount{};
    std::vector<VkImage> swapchainImages{};
    std::vector<VkImageView> swapchainImageViews{};

private:
    const VulkanContext* context;
};

#endif //WILLENGINETESTBED_SWAPCHAIN_H
