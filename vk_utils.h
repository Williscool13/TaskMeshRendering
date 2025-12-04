//
// Created by William on 2025-12-04.
//

#ifndef TASKMESHRENDERING_VK_UTILS_H
#define TASKMESHRENDERING_VK_UTILS_H

#include <fmt/format.h>
#include <volk/volk.h>
#include <vulkan/vk_enum_string_helper.h>


#define VK_CHECK(x)                                                            \
    do {                                                                       \
        VkResult err = x;                                                      \
        if (err) {                                                             \
            fmt::println("Detected Vulkan error: {}\n", string_VkResult(err)); \
            exit(1);                                                           \
        }                                                                      \
    } while (0)


#endif //TASKMESHRENDERING_VK_UTILS_H