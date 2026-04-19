module bindings;

import "./scene_data";
import "./materials";
import "./lights";

public enum BindingNums
{
    ResultText = 0,
    Scene = 1,
    LightScene = -1,
    Uniform = 2,
    Material = 3,
    Instances = 4,
    Vbuff = 5,
    IBuff = 6,
    PunctualLightBuffer = -1,
}

[[vk::push_constant]]
public ConstantBuffer<PushedCst> pushed;

[[vk::binding(BindingNums.ResultText, 0)]]
public RWTexture2D resultTexture;

[[vk::binding(BindingNums.Scene, 0)]]
public RaytracingAccelerationStructure scene;

// [[vk::binding(BindingNums.LightScene, 0)]]
// public RaytracingAccelerationStructure lightsScene;

[[vk::binding(BindingNums.Uniform, 0)]]
public ConstantBuffer<RendererUniforms> uni;

[[vk::binding(BindingNums.Material, 0)]]
public StructuredBuffer<Brdf> materialBuffer;

[[vk::binding(BindingNums.Instances, 0)]]
public StructuredBuffer<RendererInstance> instances;

[[vk::binding(BindingNums.Vbuff, 0)]]
public StructuredBuffer<Vertex> vertexBuffer;

[[vk::binding(BindingNums.IBuff, 0)]]
public StructuredBuffer<uint3> indexBuffer;

// [[vk::binding(BindingNums.PunctualLightBuffer, 0)]]
// public StructuredBuffer<Light> punctualLightBuffer;