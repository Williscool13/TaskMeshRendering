#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <iostream>

#include "task_mesh_rendering.h"

int main()
{
    TaskMeshRendering tmr;

    tmr.Initialize();
    tmr.Run();
    tmr.Cleanup();

    return 0;
}
