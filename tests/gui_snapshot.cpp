// Offscreen GUI snapshot of the education shell.
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    const int preset = argc > 2 ? juce::String (argv[2]).getIntValue() : 0;
    OrbitAudioProcessor proc;
    proc.setCurrentProgram (preset);
    proc.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    juce::Random r (1);
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    for (int k = 0; k < 90; ++k)
    {
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i)
                buf.setSample (c, i, 0.2f * (r.nextFloat() * 2.0f - 1.0f));
        proc.processBlock (buf, midi);
    }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File f = outDir.getChildFile ("gui_shell.png");
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat png;
    const bool ok = os.openedOk() && png.writeImageToStream (img, os);
    std::printf ("%s %s (%dx%d)\n", ok ? "wrote" : "FAILED", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    ed.reset();
    return ok ? 0 : 1;
}
