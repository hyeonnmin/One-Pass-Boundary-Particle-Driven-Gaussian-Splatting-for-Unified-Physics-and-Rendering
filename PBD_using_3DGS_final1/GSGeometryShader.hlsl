#include "Common.hlsli"

cbuffer BasicGeometryConstantBuffer : register(b0) {
    matrix model;
    matrix invTranspose;
    matrix view;
    matrix projection;
    matrix invModel;
    matrix invView;
    float scaling;
};

float3x3 QuaternionToMatrix(float4 q) {
    float w = q.x;
    float x = q.y;
    float y = q.z;
    float z = q.w;

    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float xy = x * y;
    float xz = x * z;
    float yz = y * z;
    float wx = w * x;
    float wy = w * y;
    float wz = w * z;

    // column-major 기준 float3x3
    return float3x3(1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy),
                    2 * (xy - wz), 1 - 2 * (xx + zz), 2 * (yz + wx),
                    2 * (xz + wy), 2 * (yz - wx), 1 - 2 * (xx + yy));
}

[maxvertexcount(4)] void
main(point GSGeometryShaderInput input[1],
     inout TriangleStream<GSPixelShaderInput> triStream) {
    float3x3 R_local = QuaternionToMatrix(input[0].rotationModel);
    float3x3 R_model = (float3x3)model;
    float3x3 R_view = (float3x3)view;
    float3x3 R_invView = transpose(R_view);

    float3x3 S_local =
        float3x3(input[0].scaleModel.x, 0, 0, 0, input[0].scaleModel.y, 0, 0, 0,
                 input[0].scaleModel.z);

    // scale 포함 최종 변환
    float3x3 M_final = mul(mul(mul(S_local, R_local), R_model), R_view);

    // row-vector 기준에서 ex*M_final = M_final의 row0 (축 벡터)
    float3 a0 = M_final[0].xyz; // local X axis (scaled) projected to view XY
    float3 a1 = M_final[1].xyz; // local Y axis
    float3 a2 = M_final[2].xyz; // local Z axis

    float c0 = dot(a0.xy, a0.xy);
    float c1 = dot(a1.xy, a1.xy);
    float c2 = dot(a2.xy, a2.xy);

    // screen-space에 가장 덜 기여하는 축(=XY가 가장 작은 축) 버리기
    int k = 0;
    float ck = c0;
    if (c1 < ck) {
        k = 1;
        ck = c1;
    }
    if (c2 < ck) {
        k = 2;
        ck = c2;
    }

    // 남길 두 축 선택 (u,v는 3D지만 실제로는 xy만 쓸거)
    float3 u, v, n;
    if (k == 0) {
        u = a1;
        v = a2;
    } else if (k == 1) {
        u = a0;
        v = a2;
    } else {
        u = a0;
        v = a1;
    }

    // --- 2D Gram-Schmidt ---
    float2 u2 = u.xy;
    float uLen = length(u2);
    float2 right2 = (uLen > 1e-8) ? (u2 / uLen) : float2(1, 0);
    float rightLen = uLen;

    float2 v2 = v.xy;
    float2 v_ortho2 = v2 - dot(v2, right2) * right2;
    float vLen = length(v_ortho2);

    // fallback: 직교 벡터
    float2 up2 =
        (vLen > 1e-8) ? (v_ortho2 / vLen) : float2(-right2.y, right2.x);
    float upLen = (vLen > 1e-8) ? vLen : length(v2);

    // 3D로 올리되 z=0 (뷰 평면 빌보드)
    float3 right = float3(right2, 0);
    float3 up = float3(up2, 0);

    // "길이 포함" 축 벡터(뷰 XY 평면 위)
    float3 axisRight = right * rightLen;
    float3 axisUp = up * upLen;

    // 1. 중심 위치(model → world)
    float3 center = mul(float4(input[0].posModel, 1.0f), model).xyz;

    // 2. 중심 위치(world -> view)
    center = mul(float4(center, 1.0f), view).xyz;

    float3 cornersVS[4] = {
        center + (-axisRight * scaling + axisUp * scaling),
        center + (axisRight * scaling + axisUp * scaling),
        center + (-axisRight * scaling - axisUp * scaling),
        center + (axisRight * scaling - axisUp * scaling),
    };

    float2 uvs[4] = {
        float2(-1.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(-1.0f, -1.0f),
        float2(1.0f, -1.0f),
    };

    GSPixelShaderInput output;

    [unroll] for (int i = 0; i < 4; ++i) {
        float3 posView = cornersVS[i];
        float4 posProj = mul(float4(posView, 1.0f), projection);

        // output.posModel = float4(input[0].posModel, 1.0f);
        // output.posWorld = mul(float4(input[0].posModel, 1.0f), model);

        float4 posModel = mul(mul(float4(posView, 1.0f), invView), invModel);
        output.posModel = posModel;
        output.posWorld = mul(posModel, model);

        float4 normal = float4(input[0].normalModel, 0.0f);
        output.normalWorld = mul(normal, invTranspose).xyz;
        output.normalWorld = normalize(output.normalWorld);

        output.posProj = posProj;
        output.texcoord = uvs[i];
        output.colorVert = (input[0].shModel * 0.2 + 0.5);
        output.alphaVert = 1.0f / (1.0f + exp(-input[0].opacityModel));

        triStream.Append(output);
    }
    triStream.RestartStrip();
}
