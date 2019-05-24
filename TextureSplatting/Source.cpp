#include <Windows.h>
#include <DirectXMath.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wincodec.h>

using namespace DirectX;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "windowscodecs.lib")

// 一つの頂点情報を格納する構造体
struct VERTEX {
	XMFLOAT3 V;
	XMFLOAT2 T;
};

// GPU(シェーダ側)へ送る数値をまとめた構造体
struct CONSTANT_BUFFER {
	XMMATRIX mWVP;
	int texNum;
};

#define WIN_STYLE WS_OVERLAPPEDWINDOW
int CWIDTH;     // クライアント領域の幅
int CHEIGHT;    // クライアント領域の高さ

HWND WHandle;
const char *ClassName = "Temp_Window";

IDXGISwapChain *pSwapChain;
ID3D11Device *pDevice;
ID3D11DeviceContext *pDeviceContext;

ID3D11RenderTargetView *pBackBuffer_RTV;

ID3D11RasterizerState *pRasterizerState;
ID3D11VertexShader *pVertexShader;
ID3D11InputLayout *pVertexLayout;
ID3D11PixelShader *pPixelShader;
ID3D11PixelShader *pPixelShader2;
ID3D11Buffer *pConstantBuffer;
ID3D11Buffer *pVertexBuffer;

ID3D11Texture2D *pTexture[4];
ID3D11ShaderResourceView *pTextureSRV[4];


static float x = 0;

LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInstancePrev, LPSTR pCmdLine, int nCmdShow) {

	// ウィンドウクラスを登録する
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WinProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = ClassName;
	RegisterClass(&wc);

	// ウィンドウの作成
	WHandle = CreateWindow(ClassName, "ポリゴン テクスチャ", WIN_STYLE, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 800, NULL, NULL, hInstance, NULL);
	if (WHandle == NULL) return 0;
	ShowWindow(WHandle, nCmdShow);

	// メッセージループの実行
	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			// ----- DXの処理 -----
			float clearColor[4] = { 0.1, 0.1, 0.1, 1 };
			pDeviceContext->ClearRenderTargetView(pBackBuffer_RTV, clearColor);

			// パラメータの計算
			XMVECTOR eye_pos = XMVectorSet(0.0f, 0.0f, -2.0f, 1.0f);
			XMVECTOR eye_lookat = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			XMVECTOR eye_up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
			XMMATRIX World = XMMatrixScaling(0.5, 0.5, 0.5);
			XMMATRIX View = XMMatrixLookAtLH(eye_pos, eye_lookat, eye_up);
			XMMATRIX Proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, (FLOAT)CWIDTH / (FLOAT)CHEIGHT, 0.1f, 100.0f);

			// パラメータの受け渡し
			D3D11_MAPPED_SUBRESOURCE cdata;
			CONSTANT_BUFFER cb;
			{
				World *= XMMatrixTranslation(-0.5f, 0, 0);
				cb.mWVP = XMMatrixTranspose(World * View * Proj);
				cb.texNum = 0;
				pDeviceContext->Map(pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cdata);
				memcpy_s(cdata.pData, cdata.RowPitch, (void*)(&cb), sizeof(cb));
				pDeviceContext->Unmap(pConstantBuffer, 0);

				pDeviceContext->PSSetShader(pPixelShader, NULL, 0);
				pDeviceContext->Draw(4, 0);
			}
			for (int i = 0; i < 3; i++) {
				World = XMMatrixScaling(0.5, 0.5, 0.5) * XMMatrixTranslation(0, -0.5f + 0.5f*i, 0);
				cb.mWVP = XMMatrixTranspose(World * View * Proj);
				cb.texNum = i + 1;
				pDeviceContext->Map(pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cdata);
				memcpy_s(cdata.pData, cdata.RowPitch, (void*)(&cb), sizeof(cb));
				pDeviceContext->Unmap(pConstantBuffer, 0);

				pDeviceContext->Draw(4, 0);
			}
			World = XMMatrixScaling(0.5, 0.5, 0.5) * XMMatrixTranslation(0.5f, 0, 0);
			cb.mWVP = XMMatrixTranspose(World * View * Proj);
			cb.texNum = -1;
			pDeviceContext->Map(pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cdata);
			memcpy_s(cdata.pData, cdata.RowPitch, (void*)(&cb), sizeof(cb));
			pDeviceContext->Unmap(pConstantBuffer, 0);

			pDeviceContext->PSSetShader(pPixelShader2, NULL, 0);
			pDeviceContext->Draw(4, 0);

			pSwapChain->Present(0, 0);
		}
	}

	return 0;
}


LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch (uMsg) {
	case WM_CREATE:
	{

		// ----- パイプラインの準備 -----
		RECT csize;
		GetClientRect(hwnd, &csize);
		CWIDTH = csize.right;
		CHEIGHT = csize.bottom;

		DXGI_SWAP_CHAIN_DESC scd = { 0 };
		scd.BufferCount = 1;
		scd.BufferDesc.Width = CWIDTH;
		scd.BufferDesc.Height = CHEIGHT;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.RefreshRate.Numerator = 60;
		scd.BufferDesc.RefreshRate.Denominator = 1;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.OutputWindow = hwnd;
		scd.SampleDesc.Count = 1;
		scd.SampleDesc.Quality = 0;
		scd.Windowed = TRUE;
		D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
		D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &fl, 1, D3D11_SDK_VERSION, &scd, &pSwapChain, &pDevice, NULL, &pDeviceContext);

		ID3D11Texture2D *pbbTex;
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pbbTex);
		pDevice->CreateRenderTargetView(pbbTex, NULL, &pBackBuffer_RTV);
		pbbTex->Release();

		// ビューポートの設定
		D3D11_VIEWPORT vp;
		vp.Width = CWIDTH;
		vp.Height = CHEIGHT;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;

		// シェーダの設定
		ID3DBlob *pCompileVS = NULL;
		ID3DBlob *pCompilePS = NULL;
		ID3DBlob *pCompilePS2 = NULL;
		D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "VS", "vs_5_0", NULL, 0, &pCompileVS, NULL);
		pDevice->CreateVertexShader(pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(), NULL, &pVertexShader);
		D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "PS_ORIGIN", "ps_5_0", NULL, 0, &pCompilePS, NULL);
		pDevice->CreatePixelShader(pCompilePS->GetBufferPointer(), pCompilePS->GetBufferSize(), NULL, &pPixelShader);
		D3DCompileFromFile(L"shader.hlsl", NULL, NULL, "PS_SPLAT", "ps_5_0", NULL, 0, &pCompilePS2, NULL);
		pDevice->CreatePixelShader(pCompilePS2->GetBufferPointer(), pCompilePS2->GetBufferSize(), NULL, &pPixelShader2);

		// 頂点レイアウト
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		pDevice->CreateInputLayout(layout, 2, pCompileVS->GetBufferPointer(), pCompileVS->GetBufferSize(), &pVertexLayout);
		pCompileVS->Release();
		pCompilePS->Release();
		pCompilePS2->Release();

		// 定数バッファの設定
		D3D11_BUFFER_DESC cb;
		cb.ByteWidth = sizeof(CONSTANT_BUFFER);
		cb.Usage = D3D11_USAGE_DYNAMIC;
		cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cb.MiscFlags = 0;
		cb.StructureByteStride = 0;
		pDevice->CreateBuffer(&cb, NULL, &pConstantBuffer);

		// 頂点データの作成
		VERTEX vertices[] = {
			XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT2(0, 1),
			XMFLOAT3(-0.5f, 0.5f, 0.0f), XMFLOAT2(0, 0),
			XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT2(1, 1),
			XMFLOAT3(0.5f, 0.5f, 0.0f), XMFLOAT2(1, 0),
		};

		// 頂点データ用バッファの設定
		D3D11_BUFFER_DESC bd;
		bd.ByteWidth = sizeof(VERTEX) * 4;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = vertices;
		pDevice->CreateBuffer(&bd, &InitData, &pVertexBuffer);

		// ラスタライザの設定
		D3D11_RASTERIZER_DESC rdc = {};
		rdc.CullMode = D3D11_CULL_NONE;
		rdc.FillMode = D3D11_FILL_SOLID;
		pDevice->CreateRasterizerState(&rdc, &pRasterizerState);

		const wchar_t name[4][30] = { L"rgb_map.bmp", L"grass.bmp", L"tile.bmp", L"water.bmp"};

		for (int i = 0; i < 4; ++i) {
			// テクスチャの読み込み
			CoInitialize(NULL);
			IWICImagingFactory *pFactory = NULL;
			IWICBitmapDecoder *pDecoder = NULL;
			IWICBitmapFrameDecode* pFrame = NULL;
			IWICFormatConverter* pFormatConverter = NULL;
			CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)(&pFactory));
			pFactory->CreateDecoderFromFilename(name[i], NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);
			pDecoder->GetFrame(0, &pFrame);
			pFactory->CreateFormatConverter(&pFormatConverter);
			pFormatConverter->Initialize(pFrame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, NULL, 1.0f, WICBitmapPaletteTypeMedianCut);
			UINT imgWidth;
			UINT imgHeight;
			pFormatConverter->GetSize(&imgWidth, &imgHeight);

			// テクスチャの設定
			D3D11_TEXTURE2D_DESC texdec;
			texdec.Width = imgWidth;
			texdec.Height = imgHeight;
			texdec.MipLevels = 1;
			texdec.ArraySize = 1;
			texdec.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texdec.SampleDesc.Count = 1;
			texdec.SampleDesc.Quality = 0;
			texdec.Usage = D3D11_USAGE_DYNAMIC;
			texdec.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texdec.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			texdec.MiscFlags = 0;
			pDevice->CreateTexture2D(&texdec, NULL, &pTexture[i]);

			// テクスチャを送る
			D3D11_MAPPED_SUBRESOURCE hMappedres;
			pDeviceContext->Map(pTexture[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &hMappedres);
			pFormatConverter->CopyPixels(NULL, imgWidth * 4, imgWidth * imgHeight * 4, (BYTE*)hMappedres.pData);
			pDeviceContext->Unmap(pTexture[i], 0);

			// シェーダリソースビュー(テクスチャ用)の設定
			D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
			srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srv.Texture2D.MipLevels = 1;
			pDevice->CreateShaderResourceView(pTexture[i], &srv, &pTextureSRV[i]);

			pFormatConverter->Release();
			pFrame->Release();
			pDecoder->Release();
			pFactory->Release();
		}

		// オブジェクトの反映
		UINT stride = sizeof(VERTEX);
		UINT offset = 0;
		pDeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
		pDeviceContext->IASetInputLayout(pVertexLayout);
		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		pDeviceContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);
		pDeviceContext->PSSetConstantBuffers(0, 1, &pConstantBuffer);
		pDeviceContext->PSSetShaderResources(0, 4, &pTextureSRV[0]);
		pDeviceContext->VSSetShader(pVertexShader, NULL, 0);
		pDeviceContext->PSSetShader(pPixelShader, NULL, 0);
		pDeviceContext->RSSetState(pRasterizerState);
		pDeviceContext->OMSetRenderTargets(1, &pBackBuffer_RTV, NULL);
		pDeviceContext->RSSetViewports(1, &vp);

		return 0;
	}
	case WM_DESTROY:

		pSwapChain->Release();
		pDeviceContext->Release();
		pDevice->Release();

		pBackBuffer_RTV->Release();
		for (int i = 0; i < 4; ++i) {
			pTextureSRV[i]->Release();
			pTexture[i]->Release();
		}

		pVertexShader->Release();
		pVertexLayout->Release();
		pPixelShader->Release();
		pPixelShader2->Release();
		pConstantBuffer->Release();
		pVertexBuffer->Release();
		pRasterizerState->Release();

		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}