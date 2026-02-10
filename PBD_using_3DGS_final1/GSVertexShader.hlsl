#include "Common.hlsli"

cbuffer BasicVertexConstantBuffer : register(b0) {
    matrix model;
    matrix invTranspose;
    matrix view;
    matrix projection;
};

GSGeometryShaderInput main(GSVertexShaderInput input) {
    GSGeometryShaderInput output;
    output.normalModel = input.normalModel;
    output.posModel = input.posModel;
    output.rotationModel = input.rotationModel;
    output.scaleModel = input.scaleModel;
    output.shModel = input.shModel;
    output.opacityModel = input.opacityModel;

    return output;
}