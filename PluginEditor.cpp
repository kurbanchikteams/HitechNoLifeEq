"  > Эквалайзер и Моды разделены: EQ просто 'слушает' Micro-DAW систему.\n"
            "  > Скоро: Ультралегкая автономная Micro-DAW без запуска FL Studio!\n\n"
            "--------------------------------------------------------------------------------\n"
            " Введите '/exit' или нажмите ESC для закрытия справки.";

        g.drawFittedText (helpText, 25, 20, (int) w - 50, (int) h - 40, juce::Justification::topLeft, 30);
    }
}

void SpectralEqAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    float modBoxX = getWidth() - 340.0f;
    float modBoxY = 8.0f;

    if (e.position.x >= modBoxX && e.position.x <= modBoxX + 320.0f &&
        e.position.y >= modBoxY && e.position.y <= modBoxY + 22.0f)
    {
        bool currentState = audioProcessor.modsEnabled.load();
        audioProcessor.modsEnabled.store (!currentState);
        repaint();
        return;
    }

    if (showHelpOverlay)
    {
        showHelpOverlay = false;
        repaint();
    }
}

void SpectralEqAudioProcessorEditor::textEditorReturnKeyPressed (juce::TextEditor& editor)
{
    juce::String command = editor.getText().trim().toLowerCase();
    editor.clear();

    if (command == "/help" || command == "help")
    {
        showHelpOverlay = true;
        repaint();
        return;
    }

    if (command == "/exit" || command == "clear")
    {
        showHelpOverlay = false;
        repaint();
        return;
    }

    processConsoleCommand (command);
}

void SpectralEqAudioProcessorEditor::processConsoleCommand (const juce::String& command)
{
    if (command == "reset")
    {
        for (auto& g : audioProcessor.binGains) g.store (1.0f);
        repaint();
        return;
    }

    if (command.startsWith ("cut "))
    {
        juce::String freqStr = command.substring (4).replace ("hz", "").trim();
        float freq = freqStr.getFloatValue();

        if (freq > 0.0f)
        {
            float binWidth = (float) audioProcessor.getSampleRate() / (float) SpectralEqAudioProcessor::fftSize;
            int targetBin = (int) (freq / binWidth);

            if (targetBin >= 0 && targetBin < (int) audioProcessor.binGains.size())
            {
                audioProcessor.binGains[targetBin].store (0.0f);
            }
        }
    }
    
    repaint();
}
