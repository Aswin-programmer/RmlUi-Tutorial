#include <iostream>
#include <string>
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include "ADDITONAL/RmlUi_Backend.h"

// Global/static variable to track reload requests
static bool reload_requested = false;

// KeyDownCallback for handling F5
bool HandleKeyDown(Rml::Context* context, Rml::Input::KeyIdentifier key, int /*modifier*/, float /*native_dp_ratio*/, bool priority) {
    if (key == Rml::Input::KI_F5 && priority) {
        reload_requested = true;
        return true; // Block further processing of F5
    }
    return false;
}

void ReloadDocument(Rml::Context* context, Rml::ElementDocument*& document, const std::string& path) {
    if (document) {
        document->Close(); // Close the current document
    }

    // Clear cached stylesheets to force reload
    Rml::Factory::ClearStyleSheetCache(); // <-- Add this line

    document = context->LoadDocument(path); // Reload from disk
    if (document) {
        document->Show();
    }
    else {
        Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to reload document!");
    }
}

struct MyData
{
    std::string title = "Hello World!";
    std::string animal = "dog";
    bool show_text = true;

}MyData;

bool SetupDataBinding(Rml::Context* context, Rml::DataModelHandle& my_model)
{
    Rml::DataModelConstructor constructor = context->CreateDataModel("my_model");
    if (!constructor)
    {
        return false;
    }
    constructor.Bind("title", &MyData.title);
    constructor.Bind("animal", &MyData.animal);
    constructor.Bind("show_text", &MyData.show_text);

    my_model = constructor.GetModelHandle();
    return true;
}

int main() {
    // Initialize backend first
    std::cout << "Initializing backend" << std::endl;
    if (!Backend::Initialize("RmlUi Demo", 1280, 720, true)) {
        std::cout << "Backend initialization failed" << std::endl;
        return 1;
    }

    // Set up core interfaces BEFORE initializing RmlUi
    Rml::SetSystemInterface(Backend::GetSystemInterface());
    Rml::SetRenderInterface(Backend::GetRenderInterface());

    // Initialize RmlUi
    if (!Rml::Initialise()) {
        return 1;
    }

    // Load fonts
    // Load font faces
    if (!Rml::LoadFontFace("assets/LatoLatin-Regular.ttf")) {
        Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to load regular font!");
    }
    // Add these lines to load bold and italic variants if you have them
    if (!Rml::LoadFontFace("assets/LatoLatin-Bold.ttf")) {
        Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to load bold font!");
    }
    if (!Rml::LoadFontFace("assets/LatoLatin-Italic.ttf")) {
        Rml::Log::Message(Rml::Log::LT_WARNING, "Failed to load italic font!");
    }

    // Create context with explicit render dimensions
    Rml::Context* context = Rml::CreateContext("main", Rml::Vector2i(1280, 720));
    if (!context) {
        Rml::Log::Message(Rml::Log::LT_ERROR, "Context creation failed!");
        return 1;
    }

    // Initialize debugger AFTER context creation
    Rml::Debugger::Initialise(context);
    //Rml::Debugger::SetVisible(true);  // Start with debugger visible for testing

    //Setup Data Binding:
    Rml::DataModelHandle modelHandle;
    if (!SetupDataBinding(context, modelHandle))
    {
        std::cout << "Data model setup failed!" << std::endl;
        return 1;
    }

    // Load document
    std::cout << "About to load document" << std::endl;
    Rml::ElementDocument* document = context->LoadDocument("assets/demo.rml");
    if (!document) {
        std::cout << "Document load failed" << std::endl;
        Rml::Log::Message(Rml::Log::LT_ERROR, "Document load failed!");
        return 1;
    }
    document->Show();
    std::cout << "Document loaded successfully" << std::endl;
    // Main loop with proper error handling

    bool f5_was_pressed = false;

    try {
        while (Backend::ProcessEvents(context, nullptr, false)) {
            context->Update();

            // Detect F5 press without callbacks
            GLFWwindow* window = Backend::GetWindow();
            if (window) {
                const bool f5_currently_pressed = (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS);

                if (f5_currently_pressed && !f5_was_pressed) {
                    reload_requested = true;
                }
                f5_was_pressed = f5_currently_pressed;
            }

            // Check if F5 was pressed
            if (reload_requested) {
                ReloadDocument(context, document, "assets/demo.rml");
                std::cout << 1 << std::endl;
                reload_requested = false;
            }

            std::cout << MyData.animal << std::endl;

            Backend::BeginFrame();
            context->Render();
            Backend::PresentFrame();
        }
    }
    catch (const std::exception& e) {
        Rml::Log::Message(Rml::Log::LT_ERROR, "Runtime error: %s", e.what());
    }

    // Cleanup
    Rml::Shutdown();
    Backend::Shutdown();

    return 0;
}