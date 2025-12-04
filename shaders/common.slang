module common;

public struct Vertex
{
    public float3 position;
}

public struct Meshlet
{
    public float4 meshletBoundingSphere;
    public float3 coneApex;
    public float coneCutoff;
    public float3 coneAxis;
    public uint32_t vertexOffset;
    public uint32_t meshletVerticesOffset;
    public uint32_t meshletTriangleOffset;
    public uint32_t meshletVerticesCount;
    public uint32_t meshletTriangleCount;
};

public struct MeshletPrimitive
{
    public uint32_t meshletOffset;
    public uint32_t meshletCount;
    public uint32_t materialIndex;
    public uint32_t bHasTransparent;
    public uint32_t bHasSkinning;
    uint32_t padding1;
    uint32_t padding2;
    uint32_t padding3;
    public float4 boundingSphere;
};

public struct MeshletVertexIndex {
    public uint32_t vertexIndex;
}

public struct MeshletTriangleIndex {
    public uint8_t triangleIndex;
}

public struct VertexData {
    public Vertex[] vertices;
};

public struct MeshletPrimitiveData {
    public MeshletPrimitive[] primitives;
}

public struct MeshletData {
    public Meshlet[] meshlets;
};

public struct MeshletVerticesData {
    public MeshletVertexIndex[] meshletVertices;
};

public struct MeshletTrianglesData {
    public MeshletTriangleIndex[] meshletTriangles;
};


public struct Frustum
{
    public float4 planes[6];
};

public struct SceneData
{
    public float4x4 view;
    public float4x4 proj;
    public float4x4 viewProj;

    public float4x4 invView;
    public float4x4 invProj;
    public float4x4 invViewProj;

    float4x4 viewProjCameraLookDirection;

    public float4x4 prevView;
    public float4x4 prevProj;
    public float4x4 prevViewProj;

    public float4x4 prevInvView;
    public float4x4 prevInvProj;
    public float4x4 prevInvViewProj;

    float4x4 prevViewProjCameraLookDirection;

    public float4 cameraWorldPos;
    public float4 prevCameraWorldPos;

    public Frustum frustum;

    public float2 renderTargetSize;
    float2 texelSize;

    float2 cameraPlanes;

    public float deltaTime;
};