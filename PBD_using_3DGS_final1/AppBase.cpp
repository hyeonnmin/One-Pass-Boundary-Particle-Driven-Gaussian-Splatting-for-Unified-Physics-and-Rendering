#include "AppBase.h"

#include "D3D11Utils.h"

#include <chrono> // CPU 타이머 
#include <iomanip>
#include <wrl/client.h>

// imgui_impl_win32.cpp에 정의된 메시지 처리 함수에 대한 전방 선언
// VCPKG를 통해 IMGUI를 사용할 경우 빨간줄로 경고가 뜰 수 있음
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace jhm {

using namespace std;

// RegisterClassEx()에서 멤버 함수를 직접 등록할 수가 없기 때문에
// 클래스의 멤버 함수에서 간접적으로 메시지를 처리할 수 있도록 도와줍니다.
AppBase *g_appBase = nullptr;

// RegisterClassEx()에서 실제로 등록될 콜백 함수
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    // g_appBase를 이용해서 간접적으로 멤버 함수 호출
    return g_appBase->MsgProc(hWnd, msg, wParam, lParam);
}

// 생성자
AppBase::AppBase()
    : m_screenWidth(1280), m_screenHeight(960), m_mainWindow(0),
      m_screenViewport(D3D11_VIEWPORT()) {

    g_appBase = this;
}

AppBase::~AppBase() {
    g_appBase = nullptr;

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    DestroyWindow(m_mainWindow);
    // UnregisterClass(wc.lpszClassName, wc.hInstance);//생략

    // COMPtr에서 알아서 release
    // ComPtr automatically maintains a reference count for the underlying
    // interface pointer and releases the interface when the reference count
    // goes to zero.
    // https:learn.microsoft.com/en-us/cpp/cppcx/wrl/comptr-class?view=msvc-170
    // 예시: m_d3dDevice.Reset(); 생략
}

float AppBase::GetAspectRatio() const {
    return float(m_screenWidth - m_guiWidth) / m_screenHeight;
}

struct CpuScopeTimer {
    using clock = std::chrono::high_resolution_clock;
    clock::time_point t0;
    CpuScopeTimer() : t0(clock::now()) {}
    double Ms() const {
        auto t1 = clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
};


void AppBase::CreateGpuTimerQueries() {
    for (int i = 0; i < GPU_QUERY_LATENCY; ++i) {
        D3D11_QUERY_DESC dq = {};
        dq.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        dq.MiscFlags = 0;
        m_device->CreateQuery(&dq, m_gpuTimer[i].disjoint.GetAddressOf());

        D3D11_QUERY_DESC tq = {};
        tq.Query = D3D11_QUERY_TIMESTAMP;
        tq.MiscFlags = 0;
        m_device->CreateQuery(&tq, m_gpuTimer[i].tsBegin.GetAddressOf());
        m_device->CreateQuery(&tq, m_gpuTimer[i].tsEnd.GetAddressOf());
    }
}

void AppBase::ReadBackGpuTimer(int idx) {
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint = {};
    UINT64 tBegin = 0, tEnd = 0;

    // GetData는 결과가 준비 안 됐으면 S_FALSE 반환
    HRESULT hrD = m_context->GetData(m_gpuTimer[idx].disjoint.Get(), &disjoint,
                                     sizeof(disjoint), 0);
    HRESULT hrB = m_context->GetData(m_gpuTimer[idx].tsBegin.Get(), &tBegin,
                                     sizeof(tBegin), 0);
    HRESULT hrE =
        m_context->GetData(m_gpuTimer[idx].tsEnd.Get(), &tEnd, sizeof(tEnd), 0);

    if (hrD == S_OK && hrB == S_OK && hrE == S_OK) {
        if (!disjoint.Disjoint && disjoint.Frequency > 0) {
            const double delta = double(tEnd - tBegin);
            m_gpuDrawMs = (delta / double(disjoint.Frequency)) * 1000.0;
        }
    }
}

int AppBase::Run() {

    // Main message loop
    MSG msg = {0};

    // 통계(60프레임 평균)
    static int s_frame = 0;
    static double s_frameAcc = 0.0;

    // 화면 표시용(최근 평균)
    static float s_frameAvgMs = 0.0f;

    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            CpuScopeTimer frameTimer;

            ImGui_ImplDX11_NewFrame(); // GUI 프레임 시작
            ImGui_ImplWin32_NewFrame();

            ImGui::NewFrame(); // 어떤 것들을 렌더링 할지 기록 시작
            ImGui::Begin("Scene Control");

            // ImGui가 측정해주는 Framerate 출력
            ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                        1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);

            UpdateGUI(); // 추가적으로 사용할 GUI

            m_guiWidth = 0;
            // 화면을 크게 쓰기 위해 기능 정지
            // ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
            // m_guiWidth = int(ImGui::GetWindowWidth());

            ImGui::End();
            ImGui::Render(); // 렌더링할 것들 기록 끝


            Update(ImGui::GetIO().DeltaTime); // 애니메이션 같은 변화


            Render(); // 우리가 구현한 렌더링

            

            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // GUI 렌더링

            // Switch the back buffer and the front buffer
            // 주의: ImGui RenderDrawData() 다음에 Present() 호출
            m_swapChain->Present(1, 0);

  
           double frameMs = frameTimer.Ms();

           // 누적
           s_frameAcc += frameMs;

           // GPU도 누적(가장 최근 값이라도 누적해서 평균내기)
           static double s_gpuAcc = 0.0;
           s_gpuAcc += m_gpuDrawMs;

           if (++s_frame % 60 == 0) {
               s_frameAvgMs = (float)(s_frameAcc / 60.0);
               float gpuAvgMs = (float)(s_gpuAcc / 60.0);

               std::cout << std::fixed << std::setprecision(3)
                         << "[AVG/60f] CPU Frame: " << s_frameAvgMs << " ms | "
                         << "[AVG/60f] GPU Scene: " << gpuAvgMs << " ms\n";

               s_frameAcc = 0.0;
               s_gpuAcc = 0.0;
           }

        }
    }

    return 0;
}

bool AppBase::Initialize() {

    if (!InitMainWindow())
        return false;

    if (!InitDirect3D())
        return false;

    if (!InitGUI())
        return false;

    return true;
}

LRESULT AppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        // Reset and resize swapchain
        // std::cout << (UINT)LOWORD(lParam) << " " << (UINT)HIWORD(lParam)
        //          << std::endl;

        if (m_swapChain) { // 처음 실행이 아닌지 확인

            m_screenWidth = int(LOWORD(lParam));
            m_screenHeight = int(HIWORD(lParam));
            m_guiWidth = 0;


            //참조 제거
            m_shaderResourceView.Reset();
            m_depthStencilView.Reset();
            m_renderTargetView.Reset();
            m_context->ClearState(); 
            m_context->Flush();

            m_swapChain->ResizeBuffers(0, // 현재 개수 유지
                                       (UINT)LOWORD(lParam), // 해상도 변경
                                       (UINT)HIWORD(lParam),
                                       DXGI_FORMAT_UNKNOWN, // 현재 포맷 유지
                                       0);
            CreateRenderTargetView();
            D3D11Utils::CreateDepthBuffer(m_device, m_screenWidth,
                                          m_screenHeight, numQualityLevels,
                                          m_depthStencilView);
            SetViewport();
        }

        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_MOUSEMOVE:
        // cout << "Mouse " << LOWORD(lParam) << " " << HIWORD(lParam) << endl;
        break;
    case WM_LBUTTONDOWN:
        // cout << "WM_LBUTTONUP Left mouse button down" << endl;
        POINT start;
        GetCursorPos(&start);
        ScreenToClient(m_mainWindow, &start);

        OnMouseDown(WM_LBUTTONDOWN, start.x, start.y);
        break;
    case WM_LBUTTONUP:
        // cout << "WM_LBUTTONUP Left mouse button" << endl;
        POINT end;
        GetCursorPos(&end);
        ScreenToClient(m_mainWindow, &end);

        OnMouseUp(WM_LBUTTONUP, end.x, end.y);
        break;
    case WM_RBUTTONUP:
        // cout << "WM_RBUTTONUP Right mouse button" << endl;
        break;
    case WM_KEYDOWN:
        // cout << "WM_KEYDOWN " << (int)wParam << endl;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }

    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

bool AppBase::InitMainWindow() {

    WNDCLASSEX wc = {sizeof(WNDCLASSEX),
                     CS_CLASSDC,
                     WndProc,
                     0L,
                     0L,
                     GetModuleHandle(NULL),
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     L"Graphics", // lpszClassName, L-string
                     NULL};

    // The RegisterClass function has been superseded by the RegisterClassEx
    // function.
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerclassa?redirectedfrom=MSDN
    if (!RegisterClassEx(&wc)) {
        cout << "RegisterClassEx() failed." << endl;
        return false;
    }

    // 툴바까지 포함한 윈도우 전체 해상도가 아니라
    // 우리가 실제로 그리는 해상도가 width x height가 되도록
    // 윈도우를 만들 해상도를 다시 계산해서 CreateWindow()에서 사용

    // 우리가 원하는 그림이 그려질 부분의 해상도
    RECT wr = {0, 0, m_screenWidth, m_screenHeight};

    // 필요한 윈도우 크기(해상도) 계산
    // wr의 값이 바뀜
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);

    // 윈도우를 만들때 위에서 계산한 wr 사용
    m_mainWindow = CreateWindow(wc.lpszClassName, L"Graphics Example",
                                WS_OVERLAPPEDWINDOW,
                                100, // 윈도우 좌측 상단의 x 좌표
                                100, // 윈도우 좌측 상단의 y 좌표
                                wr.right - wr.left, // 윈도우 가로 방향 해상도
                                wr.bottom - wr.top, // 윈도우 세로 방향 해상도
                                NULL, NULL, wc.hInstance, NULL);

    if (!m_mainWindow) {
        cout << "CreateWindow() failed." << endl;
        return false;
    }

    ShowWindow(m_mainWindow, SW_SHOWDEFAULT);
    UpdateWindow(m_mainWindow);

    return true;
}

bool AppBase::InitDirect3D() {

    // 이 예제는 Intel 내장 그래픽스 칩으로 실행을 확인하였습니다.
    // (LG 그램, 17Z90n, Intel Iris Plus Graphics)
    // 만약 그래픽스 카드 호환성 문제로 D3D11CreateDevice()가 실패하는 경우에는
    // D3D_DRIVER_TYPE_HARDWARE 대신 D3D_DRIVER_TYPE_WARP 사용해보세요
    // const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_WARP;
    const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;

    // 여기서 생성하는 것들
    // m_device, m_context, m_swapChain,
    // m_renderTargetView, m_screenViewport, m_rasterizerSate

    // m_device와 m_context 생성

    UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    const D3D_FEATURE_LEVEL featureLevels[2] = {
        D3D_FEATURE_LEVEL_11_0, // 더 높은 버전이 먼저 오도록 설정
        D3D_FEATURE_LEVEL_9_3};
    D3D_FEATURE_LEVEL featureLevel;

    if (FAILED(D3D11CreateDevice(
            nullptr,    // Specify nullptr to use the default adapter.
            driverType, // Create a device using the hardware graphics driver.
            0, // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
            createDeviceFlags, // Set debug and Direct2D compatibility flags.
            featureLevels,     // List of feature levels this app can support.
            ARRAYSIZE(featureLevels), // Size of the list above.
            D3D11_SDK_VERSION,     // Always set this to D3D11_SDK_VERSION for
                                   // Microsoft Store apps.
            device.GetAddressOf(), // Returns the Direct3D device created.
            &featureLevel,         // Returns feature level of device created.
            context.GetAddressOf() // Returns the device immediate context.
            ))) {
        cout << "D3D11CreateDevice() failed." << endl;
        return false;
    }

    /* 참고: 오류가 있을 경우 예외 발생 방법

    // MS 예제
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch Win32 API errors.
            throw Platform::Exception::CreateException(hr);
        }
    }

    // Luna DX12 교재
    #ifndef ThrowIfFailed
    #define ThrowIfFailed(x)                                              \
    {                                                                     \
        HRESULT hr__ = (x);                                               \
        std::wstring wfn = AnsiToWString(__FILE__);                       \
        if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
    }
    #endif
    */

    if (featureLevel != D3D_FEATURE_LEVEL_11_0) {
        cout << "D3D Feature Level 11 unsupported." << endl;
        return false;
    }

    // 참고: Immediate vs deferred context
    // A deferred context is primarily used for multithreading and is not
    // necessary for a single-threaded application.
    // https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-intro#deferred-context

    // 4X MSAA 지원하는지 확인
    // UINT numQualityLevels; // Resize 처리를 위해 멤버 변수로 변경
    device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4,
                                          &numQualityLevels);
    if (numQualityLevels <= 0) {
        cout << "MSAA not supported." << endl;
    }

    // numQualityLevels = 0; // MSAA 강제로 끄기

    if (FAILED(device.As(&m_device))) {
        cout << "device.AS() failed." << endl;
        return false;
    }

    if (FAILED(context.As(&m_context))) {
        cout << "context.As() failed." << endl;
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferDesc.Width = m_screenWidth;   // set the back buffer width
    sd.BufferDesc.Height = m_screenHeight; // set the back buffer height
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // use 32-bit color
    sd.BufferCount = 2;                                // Double-buffering
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;

    // DXGI_USAGE_SHADER_INPUT 쉐이더에 입력으로 넣어주기 위해 필요
    sd.BufferUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_mainWindow; // the window to be used
    sd.Windowed = TRUE;             // windowed/full-screen mode
    sd.Flags =
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // allow full-screen switching
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    if (numQualityLevels > 0) {
        sd.SampleDesc.Count = 4; // how many multisamples
        sd.SampleDesc.Quality = numQualityLevels - 1;
    } else {
        sd.SampleDesc.Count = 1; // how many multisamples
        sd.SampleDesc.Quality = 0;
    }

    if (FAILED(D3D11CreateDeviceAndSwapChain(
            0, // Default adapter
            driverType,
            0, // No software device
            createDeviceFlags, featureLevels, 1, D3D11_SDK_VERSION, &sd,
            m_swapChain.GetAddressOf(), m_device.GetAddressOf(), &featureLevel,
            m_context.GetAddressOf()))) {
        cout << "D3D11CreateDeviceAndSwapChain() failed." << endl;
        return false;
    }

    // 참고: IDXGIFactory를 이용한 CreateSwapChain()
    /*
    ComPtr<IDXGIDevice3> dxgiDevice;
    m_device.As(&dxgiDevice);

    ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiDevice->GetAdapter(&dxgiAdapter);

    ComPtr<IDXGIFactory> dxgiFactory;
    dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

    ComPtr<IDXGISwapChain> swapChain;
    dxgiFactory->CreateSwapChain(m_device.Get(), &sd, &swapChain);

    swapChain.As(&m_swapChain);
    */

    // 참고: IDXGIFactory4를 이용한 CreateSwapChainForHwnd()
    /*
    ComPtr<IDXGIFactory4> dxgiFactory;
    dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
    swapChainDesc.Width = lround(m_screenWidth); // Match the size of the
    window. swapChainDesc.Height = lround(m_screenHeight); swapChainDesc.Format
    = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
    swapChainDesc.SwapEffect =
        DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Microsoft Store apps must use
    this SwapEffect. swapChainDesc.Flags = 0; swapChainDesc.Scaling =
    DXGI_SCALING_NONE; swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    ComPtr<IDXGISwapChain1> swapChain;
    dxgiFactory->CreateSwapChainForHwnd(m_device.Get(), m_mainWindow,
    &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf());
    */

    CreateRenderTargetView();

    SetViewport();

    // Create a rasterizer state
    D3D11_RASTERIZER_DESC rastDesc;
    ZeroMemory(&rastDesc, sizeof(D3D11_RASTERIZER_DESC)); // Need this
    rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
    // rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
    rastDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_NONE;
    rastDesc.FrontCounterClockwise = false;
    rastDesc.DepthClipEnable = true; // <- zNear, zFar 확인에 필요

    m_device->CreateRasterizerState(&rastDesc,
                                    m_rasterizerSate.GetAddressOf());

    rastDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;

    m_device->CreateRasterizerState(&rastDesc,
                                    m_wireRasterizerSate.GetAddressOf());

    D3D11Utils::CreateDepthBuffer(m_device, m_screenWidth, m_screenHeight,
                                  numQualityLevels, m_depthStencilView);

    // Create depth stencil state
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
    depthStencilDesc.DepthEnable = true; // false
    depthStencilDesc.DepthWriteMask =
        D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc =
        D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;
    if (FAILED(m_device->CreateDepthStencilState(
            &depthStencilDesc, m_depthStencilState.GetAddressOf()))) {
        cout << "CreateDepthStencilState() failed." << endl;
    }

    CreateGpuTimerQueries();

    return true;
}

bool AppBase::InitGUI() {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.DisplaySize = ImVec2(float(m_screenWidth), float(m_screenHeight));
    ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplDX11_Init(m_device.Get(), m_context.Get())) {
        return false;
    }

    if (!ImGui_ImplWin32_Init(m_mainWindow)) {
        return false;
    }

    return true;
}

void AppBase::SetViewport() {

    static int previousScreenWidth = -1;
    static int previousScreenHeigth = -1;
    static int previousGuiWidth = -1;
    
    if (previousGuiWidth != m_guiWidth ||
        previousScreenWidth != m_screenWidth ||
        previousScreenHeigth != m_screenHeight) {

        previousScreenWidth = m_screenWidth;
        previousScreenHeigth = m_screenHeight;
        previousGuiWidth = m_guiWidth;

        // Set the viewport
        ZeroMemory(&m_screenViewport, sizeof(D3D11_VIEWPORT));
        m_screenViewport.TopLeftX = float(m_guiWidth);
        m_screenViewport.TopLeftY = 0;
        m_screenViewport.Width = float(m_screenWidth - m_guiWidth);
        m_screenViewport.Height = float(m_screenHeight);
        // m_screenViewport.Width = static_cast<float>(m_screenHeight);
        m_screenViewport.MinDepth = 0.0f;
        m_screenViewport.MaxDepth = 1.0f; // Note: important for depth buffering

        m_context->RSSetViewports(1, &m_screenViewport);
    }
}

bool AppBase::CreateRenderTargetView() {

    ComPtr<ID3D11Texture2D> backBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    if (backBuffer) {
        m_device->CreateRenderTargetView(backBuffer.Get(), nullptr,
                                         m_renderTargetView.GetAddressOf());
        m_device->CreateShaderResourceView(backBuffer.Get(), nullptr,
                                           m_shaderResourceView.GetAddressOf());
    } else {
        std::cout << "CreateRenderTargetView() failed." << std::endl;
        return false;
    }

    return true;
}

} // namespace hlab