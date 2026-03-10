#pragma once

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <vector>

#include "D3D11Utils.h"
#include "Ray.h"

#include <chrono> // CPU 타이머 
#include <iomanip>

namespace jhm {

using Microsoft::WRL::ComPtr;
using std::shared_ptr;
using std::vector;
using std::wstring;

// 모든 예제들이 공통적으로 사용할 기능들을 가지고 있는
// 부모 클래스
class AppBase {
  public:
    AppBase();
    virtual ~AppBase();

    float GetAspectRatio() const;

    // GPU 타이머 
    void CreateGpuTimerQueries();
    void ReadBackGpuTimer(int idx);

    int Run();

    virtual bool Initialize();
    virtual void UpdateGUI() = 0;
    virtual void Update(float dt) = 0;
    virtual void Render() = 0;

    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Convenience overrides for handling Mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y){};
    virtual void OnMouseUp(WPARAM btnState, int x, int y){};
    virtual void OnMouseMove(WPARAM btnState, int x, int y){};
    virtual Ray TransformScreenToWorld(int x, int y) = 0;

    // Convenience overrides for handling Keyboard input.
    virtual void ChangeView(int key) {};

  protected: // 상속 받은 클래스에서도 접근 가능
    bool InitMainWindow();
    bool InitDirect3D();
    bool InitGUI();

    void SetViewport();
    bool CreateRenderTargetView();

  public:
    // 변수 이름 붙이는 규칙은 VS DX11/12 기본 템플릿을 따릅니다.
    // 다만 변수 이름을 줄이기 위해 d3d는 생략했습니다.
    // 예: m_d3dDevice -> m_device
    int m_screenWidth; // 렌더링할 최종 화면의 해상도
    int m_screenHeight;
    int m_guiWidth = 0;
    HWND m_mainWindow;
    UINT numQualityLevels = 0;

    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<ID3D11RenderTargetView> m_renderTargetView;
    ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;
    ComPtr<IDXGISwapChain> m_swapChain;

    ComPtr<ID3D11RasterizerState> m_rasterizerSate;
    ComPtr<ID3D11RasterizerState> m_wireRasterizerSate;
    bool m_drawAsWire = false;

    // Depth buffer 관련
    ComPtr<ID3D11DepthStencilView> m_depthStencilView;
    ComPtr<ID3D11DepthStencilState> m_depthStencilState;
    ComPtr<ID3D11BlendState> m_blendState;

    D3D11_VIEWPORT m_screenViewport;

    // mouse control
    bool m_leftButtonDown = false;
    bool m_collision = false;

    // CPU 타이머 
    struct CpuScopeTimer {
        using clock = std::chrono::high_resolution_clock;
        clock::time_point t0;
        CpuScopeTimer() : t0(clock::now()) {}
        double Ms() const {
            auto t1 = clock::now();
            return std::chrono::duration<double, std::milli>(t1 - t0).count();
        }
    };

    // GPU 타이머
    static constexpr int GPU_QUERY_LATENCY = 4;

    struct GpuTimerQuery {
        ComPtr<ID3D11Query> disjoint;
        ComPtr<ID3D11Query> tsBegin;
        ComPtr<ID3D11Query> tsEnd;
    };

    GpuTimerQuery m_gpuTimer[GPU_QUERY_LATENCY];
    uint64_t m_frameId = 0;

    // 최근 측정값(표시용)
    double m_gpuDrawMs = 0.0;

    // Frame Time 출력 버튼 (imgui)
    bool m_startTimer = false;

    // Rotation 버튼 
    bool m_rotationButton = false;
};
} // namespace hlab