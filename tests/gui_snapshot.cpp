// Offscreen shots of the five education screens.
#include <JuceHeader.h>
#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    OrbitAudioProcessor proc;
    proc.setCurrentProgram (0);
    proc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    juce::Random random (1);
    auto* editor = dynamic_cast<OrbitEditor*> (proc.createEditor());
    std::unique_ptr<OrbitEditor> owned (editor);
    for (int k = 0; k < 40; ++k)
    {
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i)
                buf.setSample (c, i, 0.25f * std::sin (juce::MathConstants<float>::twoPi * 220.0f * (float) (k * 512 + i) / 48000.0f));
        proc.processBlock (buf, midi);
    }
    struct Shot { const char* name; bool learn, meter, advanced, zones; } shots[] = {
        { "default.png", false, false, false, false },
        { "learn.png", true, false, false, false },
        { "meter.png", false, true, false, false },
        { "advanced.png", false, false, true, false },
        { "zones.png", false, false, false, true },
    };
    int failed = 0;
    for (const auto& shot : shots)
    {
        editor->setModes (shot.learn, shot.meter, shot.advanced, shot.zones);
        for (int frame = 0; frame < (shot.meter ? 90 : 8); ++frame)
        {
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 512; ++i)
                    buf.setSample (c, i, 0.25f * std::sin (juce::MathConstants<float>::twoPi * (180.0f + 40.0f * std::sin (frame * 0.2f)) * (float) i / 48000.0f));
            proc.processBlock (buf, midi);
            editor->captureFrame();
        }
        auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        auto file = outDir.getChildFile (shot.name);
        file.deleteFile();
        juce::FileOutputStream stream (file);
        juce::PNGImageFormat png;
        const bool ok = stream.openedOk() && png.writeImageToStream (image, stream);
        std::printf ("%s %s (%dx%d)\n", ok ? "wrote" : "FAILED", file.getFullPathName().toRawUTF8(), image.getWidth(), image.getHeight());
        if (! ok) ++failed;
    }
    owned.reset();
    return failed == 0 ? 0 : 1;
}
