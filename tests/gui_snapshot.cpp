// Offscreen GUI snapshot: runs the real processor on a test signal so the telemetry is live,
// advances the editor animation, and writes PNGs (closed + ADVANCED open).
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    const int preset = argc > 2 ? juce::String (argv[2]).getIntValue() : 4;
    OrbitAudioProcessor proc;
    proc.setCurrentProgram (preset);
    proc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi; juce::Random r (1);
    std::unique_ptr<OrbitEditor> ed (dynamic_cast<OrbitEditor*> (proc.createEditor()));
    auto shoot = [&] (const juce::String& name, int frames)
    {
        for (int f = 0; f < frames; ++f)
        {
            for (int k = 0; k < 3; ++k) // ~32 ms of audio per GUI frame
            {
                for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) buf.setSample (c, i, 0.2f * (r.nextFloat() * 2 - 1));
                proc.processBlock (buf, midi);
            }
            ed->getField().stepFrames (1);
        }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f = outDir.getChildFile (name); f.deleteFile();
        juce::FileOutputStream os (f); juce::PNGImageFormat png;
        const bool ok = os.openedOk() && png.writeImageToStream (img, os);
        std::printf ("%s %s (%dx%d)\n", ok ? "wrote" : "FAILED", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    };
    shoot ("gui_m2.png", 45);
    ed->setAdvancedOpen (true);
    shoot ("gui_m2_advanced.png", 20);
    ed.reset();
    return 0;
}
