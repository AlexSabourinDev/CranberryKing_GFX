#define _CRT_SECURE_NO_WARNINGS

#define CRANBERRY_GFX_BACKEND_IMPLEMENTATION
#include "cranberry_gfx_backend.h"

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
		"CRANBERRIES",
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

	void* graphicsMemory = malloc(ctxSize + surfaceSize + graphicsDeviceSize + presentCtxSize);

	uint8_t* buffer = (uint8_t*)graphicsMemory;
	crang_ctx_t* ctx = crang_create_ctx(buffer);
	buffer += ctxSize;

	crang_surface_t* surface = crang_win32_create_surface(buffer, ctx, instance, hwnd);
	buffer += surfaceSize;

	crang_graphics_device_t* graphicsDevice = crang_create_graphics_device(buffer, ctx, surface);
	buffer += graphicsDeviceSize;

	crang_present_t* presentCtx = crang_create_present(buffer, graphicsDevice, surface);
	buffer += presentCtxSize;

	crang_shader_id_t vertShader = crang_request_shader_id(graphicsDevice, crang_shader_vertex);
	crang_shader_id_t fragShader = crang_request_shader_id(graphicsDevice, crang_shader_fragment);

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
				.commandDescs = (crang_cmd_e[])
				{
					[0] = crang_cmd_create_shader,
					[1] = crang_cmd_callback
				},
				.commandDatas = (void*[])
				{
					[0] = &(crang_cmd_create_shader_t)
					{
						.shaderId = vertShader,
						.source = vertSource,
						.sourceSize = vertSize,
						.shaderInputs =
						{
							.count = 0,
						}
					},
					[1] = &(crang_cmd_callback_t)
					{
						.callback = &free,
						.data = vertSource
					}
				},
				.count = 2
			});
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
				.commandDescs = (crang_cmd_e[])
				{
					[0] = crang_cmd_create_shader,
					[1] = crang_cmd_callback
				},
				.commandDatas = (void*[])
				{
					[0] = &(crang_cmd_create_shader_t)
					{
						.shaderId = fragShader,
						.source = fragSource,
						.sourceSize = fragSize,
						.shaderInputs =
						{
							.count = 0,
						}
					},
					[1] = &(crang_cmd_callback_t)
					{
						.callback = &free,
						.data = fragSource
					}
				},
				.count = 2
			});
	}

	// Vertex Buffer
	crang_buffer_id_t vertexBuffer = crang_request_buffer_id(graphicsDevice);
	crang_buffer_id_t indexBuffer = crang_request_buffer_id(graphicsDevice);

	{
		crang_execute_commands_immediate(graphicsDevice,
			&(crang_cmd_buffer_t)
			{
				.commandDescs = (crang_cmd_e[])
				{
					[0] = crang_cmd_create_buffer,
					[1] = crang_cmd_copy_to_buffer,
					[2] = crang_cmd_create_buffer,
					[3] = crang_cmd_copy_to_buffer
				},
				.commandDatas = (void*[])
				{
					[0] = &(crang_cmd_create_buffer_t)
					{
						.bufferId = vertexBuffer,
						.size = sizeof(float) * 3 * 4,
						.type = crang_buffer_vertex
					},
					[1] = &(crang_cmd_copy_to_buffer_t)
					{
						.bufferId = vertexBuffer,
						.data = (float[])
						{ 
							-0.5f, 0.5f, 0.0f,
							0.5f, 0.5f, 0.0f,
							0.5f, -0.5f, 0.0f,
							-0.5f, -0.5f, 0.0f
						},
						.size = sizeof(float) * 3 * 4,
						.offset = 0
					},
					[2] = &(crang_cmd_create_buffer_t)
					{
						.bufferId = indexBuffer,
						.size = sizeof(uint32_t) * 6,
						.type = crang_buffer_index
					},
					[3] = &(crang_cmd_copy_to_buffer_t)
					{
						.bufferId = indexBuffer,
						.data = (uint32_t[])
						{ 
							0, 1, 2, 0, 2, 3
						},
						.size = sizeof(uint32_t) * 6,
						.offset = 0
					},
				},
				.count = 4
			});
	}

	// Uniforms
	{
		crang_execute_commands_immediate(graphicsDevice,
			&(crang_cmd_buffer_t)
			{
				.commandDescs = (crang_cmd_e[])
				{
					[0] = crang_cmd_create_buffer,
					[1] = crang_cmd_copy_to_buffer,
					[2] = crang_cmd_create_buffer,
					[3] = crang_cmd_copy_to_buffer
				},
				.commandDatas = (void*[])
				{
					[0] = &(crang_cmd_create_buffer_t)
					{
						.bufferId = vertexBuffer,
						.size = sizeof(float) * 3 * 4,
						.type = crang_buffer_vertex
					},
					[1] = &(crang_cmd_copy_to_buffer_t)
					{
						.bufferId = vertexBuffer,
						.data = (float[])
						{ 
							-0.5f, 0.5f, 0.0f,
							0.5f, 0.5f, 0.0f,
							0.5f, -0.5f, 0.0f,
							-0.5f, -0.5f, 0.0f
						},
						.size = sizeof(float) * 3 * 4,
						.offset = 0
					},
					[2] = &(crang_cmd_create_buffer_t)
					{
						.bufferId = indexBuffer,
						.size = sizeof(uint32_t) * 6,
						.type = crang_buffer_index
					},
					[3] = &(crang_cmd_copy_to_buffer_t)
					{
						.bufferId = indexBuffer,
						.data = (uint32_t[])
						{ 
							0, 1, 2, 0, 2, 3
						},
						.size = sizeof(uint32_t) * 6,
						.offset = 0
					},
				},
				.count = 4
			});
	}

	crang_pipeline_id_t pipeline = crang_create_pipeline(graphicsDevice, &(crang_pipeline_desc_t)
	{
		.presentCtx = presentCtx,
		.shaders = 
		{
			[crang_shader_vertex] = vertShader,
			[crang_shader_fragment] = fragShader
		},

		.vertexInputs = 
		{
			.inputs = (crang_vertex_input_t[])
			{
				[0] = {.binding = 0,.stride = sizeof(float) * 3 }
			},
			.count = 1
		},

		.vertexAttributes = 
		{
			.attribs = (crang_vertex_attribute_t[])
			{
				[0] = {.binding = 0,.location = 0,.offset = 0,.format = crang_vertex_format_f32_3 }
			},
			.count = 1
		}
	});

	crang_recording_buffer_id_t rectangleDraw = crang_request_recording_buffer_id(graphicsDevice);
	crang_record_commands(graphicsDevice, presentCtx, rectangleDraw, 
		&(crang_cmd_buffer_t)
		{
			.commandDescs = (crang_cmd_e[])
			{
				[0] = crang_cmd_bind_pipeline,
				[1] = crang_cmd_bind_vertex_inputs,
				[2] = crang_cmd_bind_index_input,
				[3] = crang_cmd_draw_indexed,
			},
			.commandDatas = (void*[])
			{
				[0] = &(crang_cmd_bind_pipeline_t)
				{
					.pipelineId = pipeline
				},
				[1] = &(crang_cmd_bind_vertex_inputs_t)
				{
					.bindings = (crang_vertex_input_binding_t[])
					{
						[0] = { .bufferId = vertexBuffer, .binding = 0, .offset = 0 }
					},
					.count = 1
				},
				[2] = &(crang_cmd_bind_index_input_t)
				{
					.bufferId = indexBuffer,
					.offset = 0,
					.indexType = crang_index_type_u32
				},
				[3] = &(crang_cmd_draw_indexed_t)
				{
					.indexCount = 6,
					.instanceCount = 1,
				}
			},
			.count = 4
		});

	while (true)
	{
		bool done = false;

		MSG msg;
		while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				done = true;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		if (done)
		{
			break;
		}

		crang_render(&(crang_render_desc_t)
		{
			.graphicsDevice = graphicsDevice,
			.presentCtx = presentCtx,
			.surface = surface,
			.clearColor = { 0.7f, 0.2f, 0.1f },
			.recordedBuffers =
			{
				.buffers = (crang_recording_buffer_id_t[])
				{
					rectangleDraw
				},
				.count = 1
			}
		});
	}

	crang_destroy_present(graphicsDevice, presentCtx);
	crang_destroy_graphics_device(ctx, graphicsDevice);
	crang_win32_destroy_surface(ctx, surface);
	crang_destroy_ctx(ctx);
	free(graphicsMemory);
}
