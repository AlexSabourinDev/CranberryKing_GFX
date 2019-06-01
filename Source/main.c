#define _CRT_SECURE_NO_WARNINGS

#define CRANBERRY_GFX_IMPLEMENTATION
#include "cranberry_gfx.h"

#include <malloc.h>
#include <stdio.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int main()
{
	HINSTANCE instance = GetModuleHandle(NULL);

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = instance;
	wc.lpszClassName = "CranberryWindow";

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		"CranberryWindow",
		"Learn to Program Windows",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		instance,
		NULL
		);
	ShowWindow(hwnd, SW_SHOWNORMAL);

	unsigned int ctxSize = crang_ctx_size();
	unsigned int surfaceSize = crang_win32_surface_size();
	unsigned int graphicsDeviceSize = crang_graphics_device_size();
	unsigned int presentCtxSize = crang_present_size();
	unsigned int pipelineSize = crang_pipeline_size();

	void* graphicsMemory = malloc(ctxSize + surfaceSize + graphicsDeviceSize + presentCtxSize + pipelineSize);

	uint8_t* buffer = (uint8_t*)graphicsMemory;
	crang_ctx_t* ctx = crang_create_ctx(buffer);
	buffer += ctxSize;

	crang_surface_t* surface = crang_win32_create_surface(buffer, ctx, instance, hwnd);
	buffer += surfaceSize;

	crang_graphics_device_t* graphicsDevice = crang_create_graphics_device(buffer, ctx, surface);
	buffer += graphicsDeviceSize;

	crang_present_t* presentCtx = crang_create_present(buffer, graphicsDevice, surface);
	buffer += presentCtxSize;

	crang_shader_id_t vertShader = crang_request_shader_id(graphicsDevice);
	crang_shader_id_t fragShader = crang_request_shader_id(graphicsDevice);

	{
		FILE* file = fopen("../../../Shaders/SPIR-V/default.vspv", "rb");
		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		void* vertSource = malloc(fileSize);
		unsigned int vertSize = fileSize;
		fread(vertSource, fileSize, 1, file);
		fclose(file);

		crang_execute_commands_immediate(graphicsDevice,
			&(crang_cmd_buffer_t)
			{
				.commandDescs = (crang_cmd_e[]) { [0] = crang_cmd_create_shader },
				.commandDatas = (void*[])
				{
					[0] = &(crang_cmd_create_shader_t)
					{
						.shaderId = vertShader,
						.source = vertSource,
						.sourceSize = vertSize
					}
				},
				.count = 1
			});
		free(vertSource);
	}

	{
		FILE* file = fopen("../../../Shaders/SPIR-V/default.fspv", "rb");
		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		void* fragSource = malloc(fileSize);
		unsigned int fragSize = fileSize;
		fread(fragSource, fileSize, 1, file);
		fclose(file);

		crang_execute_commands_immediate(graphicsDevice,
			&(crang_cmd_buffer_t)
			{
				.commandDescs = (crang_cmd_e[]) { [0] = crang_cmd_create_shader },
				.commandDatas = (void*[])
				{
					[0] = &(crang_cmd_create_shader_t)
					{
						.shaderId = fragShader,
						.source = fragSource,
						.sourceSize = fragSize
					}
				},
				.count = 1
			});
		free(fragSource);
	}

	crang_pipeline_t* pipeline = crang_create_pipeline(buffer, graphicsDevice, &(crang_pipeline_desc_t)
	{
		.presentCtx = presentCtx,
		.shaders = 
		{
			[crang_shader_vertex] = vertShader,
			[crang_shader_fragment] = fragShader
		},

		.vertexInputs = 
		{
			.inputs[0] = { .binding = 0, .stride = sizeof(float) * 2 },
			.count = 1
		},

		.vertexAttributes = 
		{
			.attribs[0] = { .binding = 0, .location = 0, .offset = 0, .format = crang_vertex_format_f32_2 },
			.count = 1
		}
	});
	buffer += pipelineSize;

	MSG Msg;
	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		crang_render(&(crang_render_desc_t)
		{
			.graphicsDevice = graphicsDevice,
			.presentCtx = presentCtx,
			.surface = surface,
			.clearColor = { 0.7f, 0.2f, 0.1f }
		});

		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	crang_destroy_pipeline(graphicsDevice, pipeline);
	crang_destroy_present(graphicsDevice, presentCtx);
	crang_destroy_graphics_device(ctx, graphicsDevice);
	crang_win32_destroy_surface(ctx, surface);
	crang_destroy_ctx(ctx);
	free(graphicsMemory);
}
