/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "plugin.h"

#include <directx/d3dx12.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

static Microsoft::WRL::ComPtr< ID3D12Device > d12Device;
static Microsoft::WRL::ComPtr< ID3D12CommandQueue > d12CommandQueue;
static Microsoft::WRL::ComPtr< ID3D12SwapChainAssistant > d12SwapChain;

static Microsoft::WRL::ComPtr< ID3D12Resource > d12BackBuffers[ 4 ];

static void Driver_D3D12_AbortOnFail_( HRESULT hr ) {
	if ( !FAILED( hr ) )
		return;

	abort();
}

/**
 * Determine the best adapter to utilise.
 */
static Microsoft::WRL::ComPtr< IDXGIAdapter > Driver_D3D12_GetAdapter_() {
	Microsoft::WRL::ComPtr< IDXGIFactory > factory;
	Driver_D3D12_AbortOnFail_( CreateDXGIFactory( IID_PPV_ARGS( &factory ) ) );

	size_t maxVRAM = 0;
	unsigned int i = 0;
	Microsoft::WRL::ComPtr< IDXGIAdapter > adapter, prAdapter;
	while ( factory->EnumAdapters( i, &adapter ) != DXGI_ERROR_NOT_FOUND ) {
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc( &desc );

		if ( SUCCEEDED( D3D12CreateDevice( adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof( ID3D12Device ), nullptr ) ) &&
		     desc.DedicatedVideoMemory > maxVRAM ) {
			maxVRAM = desc.DedicatedVideoMemory;
			Driver_D3D12_AbortOnFail_( adapter.As( &prAdapter ) );
		}

		++i;
	}

	return prAdapter;
}

static Microsoft::WRL::ComPtr< ID3D12Device > Driver_D3D12_CreateDevice_( Microsoft::WRL::ComPtr< IDXGIAdapter > adapter ) {
	Microsoft::WRL::ComPtr< ID3D12Device > device;
	Driver_D3D12_AbortOnFail_( D3D12CreateDevice( adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &device ) ) );

	return device;
}

void Driver_D3D12_Initialize() {
	Microsoft::WRL::ComPtr< ID3D12Debug > debug;
	Driver_D3D12_AbortOnFail_( D3D12GetDebugInterface( IID_PPV_ARGS( &debug ) ) );

	d12Device = Driver_D3D12_CreateDevice_( Driver_D3D12_GetAdapter_() );
	//d12CommandQueue = Driver_D3D12_CreateCommandQueue_( d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT );
}
