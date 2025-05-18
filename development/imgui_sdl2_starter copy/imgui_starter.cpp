// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
#define MINIAUDIO_IMPLEMENTATION
#define CINTERFACE
#define COBJMACROS
#define UNICODE

#define NTH_SAMPLE 10
#define SAMPLE_RATE 44100

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#include "IconsFontaudio.h"

#include "implot.h"
#include "miniaudio.h"
#include "DropSource.h"
#include "oleidl.h"
#include <string>
#include <regex>
#include <filesystem>

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

void WindowFullscreen() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 pos = ImVec2(viewport->WorkPos.x, viewport->WorkPos.y);
    ImGui::SetNextWindowPos(pos);
    ImVec2 size = ImVec2(viewport->WorkSize.x, viewport->WorkSize.y);
    ImGui::SetNextWindowSize(size);

    // remove window padding
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
}

ma_result result;
ma_encoder_config encoderConfig;
ma_encoder encoder;
ma_decoder decoder;
ma_device_config deviceConfig;
ma_device device;
ma_backend backends[] = {
	ma_backend_wasapi /* Loopback mode is currently only supported on WASAPI. */
};
std::filesystem::path file("out.wav");
std::filesystem::path out_file_path = std::filesystem::temp_directory_path() / file;
std::vector<float> audioData;
std::vector<float> rollingBuffer;
int cursor;
std::vector<float> smolBuffer;
int smolBufferCursor;

auto recording_start_timestamp = std::chrono::high_resolution_clock::now();
auto recording_end_timestamp = std::chrono::high_resolution_clock::now();

ImPlotPoint PlotDataGetter(int idx, void*) {
    ImPlotPoint p;
    p.x = idx;
    p.y = rollingBuffer[idx * NTH_SAMPLE];
    return p;
}

void _record_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_encoder_write_pcm_frames((ma_encoder*)pDevice->pUserData, pInput, frameCount, NULL);

    (void)pOutput;
}

void record_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    auto laudioData = static_cast<std::vector<float>*>(pDevice->pUserData);
    const float* input = static_cast<const float*>(pInput);
    // Append incoming frames
    // laudioData->insert(laudioData->end(),
    //                   input,
    //                   input + frameCount * 2);

    // smol buffer
    for (int i = 0; i < (frameCount - frameCount % NTH_SAMPLE) / NTH_SAMPLE; i += NTH_SAMPLE) {
        smolBuffer[smolBufferCursor] = input[i];
        smolBufferCursor++;
    }
    // smolBufferCursor += frameCount % NTH_SAMPLE;

    // insert new frames into rolling buffer
    if (cursor + frameCount * 2 < rollingBuffer.size()) {
        // just copy; no risk of running out of bounds
        std::copy(input, input + frameCount * 2, rollingBuffer.begin() + cursor);
    } else if (cursor + frameCount * 2 == rollingBuffer.size()) {
        std::copy(input, input + frameCount * 2 - 1, rollingBuffer.begin() + cursor);
        cursor = 0;
    } else {
        // untested / unused
        int firstLength = rollingBuffer.size() - cursor;
        std::copy(input, input + firstLength - 1, rollingBuffer.begin() + cursor);
        std::copy(input + firstLength, input + frameCount * 2 - 1, rollingBuffer.begin());
        cursor = 0;
    }
    cursor += frameCount * 2;

    (void)pOutput;
}

void play_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) {
        return;
    }

    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);

    (void)pInput;
}

void startRecording() {
    if (ma_encoder_init_file(out_file_path.generic_string().c_str(), &encoderConfig, &encoder) != MA_SUCCESS) {
        printf("Failed to initialize output file.\n");
        return;
    }

    // prepare audio data vector
    audioData.clear();
    audioData.resize(1);
    // audioData.resize(SAMPLE_RATE * 2 * 20, 0.f); // reserve 20 seconds of space

    deviceConfig = ma_device_config_init(ma_device_type_loopback);
    deviceConfig.capture.pDeviceID = NULL; /* Use default device for this example. Set this to the ID of a _playback_ device if you want to capture from a specific device. */
    deviceConfig.capture.format    = encoder.config.format;
    deviceConfig.capture.channels  = encoder.config.channels;
    deviceConfig.sampleRate        = encoder.config.sampleRate;
    deviceConfig.dataCallback      = record_data_callback;
    deviceConfig.pUserData         = &rollingBuffer;

    result = ma_device_init_ex(backends, sizeof(backends)/sizeof(backends[0]), NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize loopback device.\n");
        return;
    }

    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        ma_device_uninit(&device);
        printf("Failed to start device.\n");
        return;
    }
    recording_start_timestamp = std::chrono::high_resolution_clock::now();
}

void stopRecording() {
    ma_device_stop(&device);
    ma_encoder_uninit(&encoder);
}

void playRecording() {
    result = ma_decoder_init_file(out_file_path.generic_string().c_str(), NULL, &decoder);
    if (result != MA_SUCCESS) {
        printf("Could not load file");
        return;
    }

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate        = decoder.outputSampleRate;
    deviceConfig.dataCallback      = play_data_callback;
    deviceConfig.pUserData         = &decoder;

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
        printf("Failed to open playback device.\n");
        ma_decoder_uninit(&decoder);
        return;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        return;
    }
}

// Main code
int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 2.0f;
    style.WindowRounding = 5.0f;
    style.FrameBorderSize = 1.0f;
    style.GrabRounding = 2.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = 13.0f;
    icons_config.GlyphOffset.y = 6.0f;
    static const ImWchar icons_ranges[] = { ICON_MIN_FAD, ICON_MAX_16_FAD, 0 };
    io.Fonts->AddFontFromFileTTF( FONT_ICON_FILE_NAME_FAD, 20.0f, &icons_config, icons_ranges );
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // miniaudio setup
    encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, SAMPLE_RATE);

    rollingBuffer.clear();
    rollingBuffer.resize(SAMPLE_RATE * 2 * 20);
    smolBuffer.clear();
    smolBuffer.resize(rollingBuffer.size() / NTH_SAMPLE);
    cursor = 0;
    printf("allocated memory\n");

	HRESULT oleResult = OleInitialize(NULL);
	if(oleResult != S_OK)
	{
		std::cout << "Couldn't initialize Ole" << std::endl;
		return false;
	}
	InitCommonControls();

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool isRecording = false;

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImPlot::ShowDemoWindow();

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            WindowFullscreen();
            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
            ImGui::PopStyleVar(2);

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Text(ICON_FAD_FFWD "  Fast Forward");
            if (ImGui::Button(ICON_FAD_RECORD)) {
                if (isRecording) {
                    stopRecording();
                    printf("stopped recording\n");
                } else {
                    startRecording();
                    printf("started recording\n");
                }
                isRecording = !isRecording;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_FAD_PLAY)) {
                playRecording();
            }
            ImGui::SameLine();
            ImGui::Button(ICON_FAD_COPY);
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImPlot::BeginPlot("Line Plot", ImVec2(-1, 150), ImPlotFlags_NoLegend | ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame)) {
                ImPlot::SetupAxes("sample", "amplitude", ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks);
                // ImPlot::SetupAxesLimits(0.0, 44100 * 2 * 20, -1.f, 1.f);
                // ImPlot::SetupAxesLimits(ImAxis_X1, 0, SAMPLE_RATE * 2 * 20, ImGuiCond_Always);
                // ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0.f, 44100 * 2 * 20);
                ImPlot::SetupAxisLimits(ImAxis_X1, 0, rollingBuffer.size() / NTH_SAMPLE);
                ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0, ImPlotCond_Always);
                ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0.0, rollingBuffer.size() / NTH_SAMPLE);
                ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, -1.0, 1.0);
                // ImPlot::PlotLine("f(x)", shortAudioData.cbegin().base(), shortAudioData.size());
                
                // ImPlot::PlotLine("f(x)", rollingBuffer.cbegin().base(), rollingBuffer.size(), NTH_SAMPLE);
                
                ImPlot::PlotLineG("f(x)", PlotDataGetter, nullptr, rollingBuffer.size() / NTH_SAMPLE);
                ImPlot::EndPlot();
            }

            ImGui::Text("Rolling Buffer Size: %d", rollingBuffer.size());
            ImGui::Text("Cursor: %d", cursor);
            ImGui::Text("Smol Buffer Size: %d", smolBuffer.size());
            ImGui::Text("Smol Buffer Cursor: %d", smolBufferCursor);

            if (ImGui::Button("Button")) {
                counter++;
            }
            if (ImGui::IsItemActive()) {
                // this procedure blocks the main thread, so don't worry about the rendering loop
                static std::filesystem::path out_file_path("C:\\Users\\miste\\Desktop\\miniaudio-loopback\\a.exe");
                IDataObject *pObj;
                IDropSource *pSrc;
                std::string p = std::regex_replace(out_file_path.generic_string(), std::regex("\\/"), "\\");
                pObj = (IDataObject*)GetFileUiObject(p.c_str(), IID_IDataObject);
                if (!pObj) {
                    printf("pObj went wrong");
                    break;
                }
        
                pSrc = CreateDropSource();
                if (!pSrc)
                {
                    printf("pSrc went wrong");
                    IDataObject_Release(pObj);
                    break;
                }
        
                DWORD dwEffect;
                HRESULT res = DoDragDrop(pObj, pSrc, DROPEFFECT_COPY | DROPEFFECT_LINK, &dwEffect);
                if (!res) {
                    printf("res went wrong");
                }

                IDropSource_Release(pSrc);
                IDataObject_Release(pObj);
            }
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
