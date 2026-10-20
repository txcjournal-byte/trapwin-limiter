#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace bitera { class Content; }

// The BIT ERA window: a fixed 1100 x 700 design that scales with the window (keeps its aspect ratio).
class BitEraEditor : public juce::AudioProcessorEditor,
                     public juce::DragAndDropContainer
{
public:
    explicit BitEraEditor (BitEraProcessor&);
    ~BitEraEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void showPage (int page);   // 0 PLAY, 1 LAYERS

    static constexpr int designWidth = 1100, designHeight = 700;

private:
    BitEraProcessor& processor;
    std::unique_ptr<bitera::Content> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BitEraEditor)
};
