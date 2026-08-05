#include "MainComponent.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace smartchord::ui
{

class SmartChordArpApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Smart Chord & Arpeggiator"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override { mainWindow = nullptr; }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name)
            : juce::DocumentWindow (name, Palette::background, juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

} // namespace smartchord::ui

START_JUCE_APPLICATION (smartchord::ui::SmartChordArpApplication)
