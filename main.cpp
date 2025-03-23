#include <iostream>
#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include "ADDITONAL/RmlUi_Backend.h"

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
    Rml::Debugger::SetVisible(true);  // Start with debugger visible for testing

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
    try {
        while (Backend::ProcessEvents(context, nullptr, true)) {
            context->Update();
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