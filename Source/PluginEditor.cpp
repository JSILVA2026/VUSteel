#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    // A arte foi desenhada em 1440 x 760 (2x da janela 720 x 380)
    constexpr float kArtW = 1440.0f;
    constexpr float kArtH = 760.0f;

    // Eixo de cada agulha, medido na arte
    const juce::Point<float> kPivotL (448.0f, 372.0f);
    const juce::Point<float> kPivotR (1005.0f, 372.0f);

    constexpr float kNeedleLen = 162.0f;

    struct ScalePoint { float vu; float deg; };

    // Calibração: valor VU -> ângulo em graus (0 = vertical).
    // Valores estimados pela arte; ajuste aqui depois do primeiro teste.
    constexpr ScalePoint kScale[] = {
        { -20.0f, -42.0f }, { -10.0f, -33.0f }, { -7.0f, -23.0f }, { -5.0f, -11.0f },
        {  -3.0f,   1.0f }, {  -2.0f,   9.0f }, { -1.0f,  14.0f }, {  0.0f,  20.0f },
        {   1.0f,  27.0f }, {   2.0f,  32.0f }, {  3.0f,  36.0f }, {  5.0f,  44.0f }
    };
    constexpr int kScaleCount = (int) (sizeof (kScale) / sizeof (kScale[0]));

    bool isLabelled (int v)
    {
        for (int i = 0; i < kScaleCount; ++i)
            if ((int) kScale[i].vu == v)
                return true;
        return false;
    }

    juce::Point<float> polar (juce::Point<float> pivot, float deg, float r)
    {
        const float a = juce::degreesToRadians (deg);
        return { pivot.x + r * std::sin (a), pivot.y - r * std::cos (a) };
    }
}

VUEditor::VUEditor (VUMeterProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      background (juce::ImageCache::getFromMemory (BinaryData::INTERFACE_OK_png,
                                                   BinaryData::INTERFACE_OK_pngSize)),
      modeAtt (p.apvts, "mode", modeBox),
      refAtt  (p.apvts, "ref",  refKnob),
      trimAtt (p.apvts, "trim", trimKnob),
      lampAtt (p.apvts, "lamp", lampKnob)
{
    modeBox.addItem ("VU", 1);
    modeBox.addItem ("PPM", 2);
    modeBox.addItem ("Peak", 3);

    for (auto* k : { &refKnob, &trimKnob, &lampKnob })
    {
        k->setSliderStyle (juce::Slider::RotaryVerticalDrag);
        k->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        k->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffd8d0b8));
        addAndMakeVisible (k);
    }
    addAndMakeVisible (modeBox);

    setSize (720, 380);
    startTimerHz (60);
}

VUEditor::~VUEditor()
{
    stopTimer();
}

float VUEditor::vuToDeg (float vu)
{
    if (vu <= kScale[0].vu)               return kScale[0].deg;
    if (vu >= kScale[kScaleCount - 1].vu) return kScale[kScaleCount - 1].deg;

    for (int i = 0; i < kScaleCount - 1; ++i)
    {
        if (vu <= kScale[i + 1].vu)
        {
            const float t = (vu - kScale[i].vu) / (kScale[i + 1].vu - kScale[i].vu);
            return kScale[i].deg + t * (kScale[i + 1].deg - kScale[i].deg);
        }
    }
    return kScale[kScaleCount - 1].deg;
}

void VUEditor::timerCallback()
{
    // Massa-mola amortecida (dt = 1/60 s): movimento mecânico da agulha
    const float dt = 1.0f / 60.0f, k = 400.0f, c = 34.0f;

    auto step = [dt, k, c] (float target, float& ang, float& vel)
    {
        const float acc = k * (target - ang) - c * vel;
        vel += acc * dt;
        ang += vel * dt;
    };

    step (vuToDeg (proc.levelL.load()), angL, velL);
    step (vuToDeg (proc.levelR.load()), angR, velR);
    repaint();
}

void VUEditor::drawMeter (juce::Graphics& g, juce::Point<float> pivot, float angleDeg,
                          const juce::String& channel, float lamp, int mode) const
{
    const auto red  = juce::Colour (0xffb0201c);
    const auto dark = juce::Colour (0xff1a1a1a);

    // Luz traseira, suave, sobre o mostrador
    if (lamp > 0.0f)
    {
        juce::ColourGradient glow (juce::Colour (0xffffd9a0).withAlpha (0.28f * lamp),
                                   pivot.x, pivot.y - 150.0f,
                                   juce::Colours::transparentBlack,
                                   pivot.x, pivot.y - 410.0f, true);
        g.setGradientFill (glow);
        g.fillEllipse (pivot.x - 260.0f, pivot.y - 410.0f, 520.0f, 520.0f);
    }

    // Marcas: maiores nos valores rotulados, menores nos intermediários
    for (int v = -20; v <= 5; ++v)
    {
        const float deg = vuToDeg ((float) v);
        const bool major = isLabelled (v);
        g.setColour (v > 0 ? red : dark);
        g.drawLine (juce::Line<float> (polar (pivot, deg, major ? 146.0f : 152.0f),
                                       polar (pivot, deg, 160.0f)),
                    major ? 2.2f : 1.2f);
    }

    // Faixa vermelha acima de 0 VU
    g.setColour (red.withAlpha (0.85f));
    for (float v = 0.0f; v <= 5.0f; v += 0.25f)
    {
        const float deg = vuToDeg (v);
        g.drawLine (juce::Line<float> (polar (pivot, deg, 152.0f), polar (pivot, deg, 160.0f)), 3.0f);
    }

    // Rótulos dos valores
    g.setFont (juce::Font (15.0f, juce::Font::bold));
    for (int i = 0; i < kScaleCount; ++i)
    {
        const float vu = kScale[i].vu;
        const juce::String txt (std::abs ((int) vu));
        g.setColour (vu > 0.0f ? red : dark);
        g.drawText (txt, juce::Rectangle<float> (40.0f, 22.0f)
                            .withCentre (polar (pivot, kScale[i].deg, 124.0f)),
                    juce::Justification::centred, false);
    }

    // Identificação do modo e do canal
    const juce::String modeName = mode == 0 ? "VU" : (mode == 1 ? "PPM" : "PEAK");
    g.setColour (dark.withAlpha (0.85f));
    g.setFont (juce::Font (26.0f, juce::Font::bold));
    g.drawText (modeName, juce::Rectangle<float> (140.0f, 40.0f)
                    .withCentre ({ pivot.x, pivot.y - 80.0f }),
                juce::Justification::centred, false);

    g.setFont (juce::Font (18.0f, juce::Font::plain));
    g.drawText (channel, juce::Rectangle<float> (40.0f, 26.0f)
                    .withCentre ({ pivot.x - 110.0f, pivot.y - 20.0f }),
                juce::Justification::centred, false);

    // Agulha: sombra e corpo, com o eixo no centro
    const auto tip = polar (pivot, angleDeg, kNeedleLen);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawLine (juce::Line<float> (pivot.translated (3.0f, 4.0f), tip.translated (3.0f, 4.0f)), 3.0f);
    g.setColour (juce::Colour (0xff141414));
    g.drawLine (juce::Line<float> (pivot, tip), 2.6f);
    g.fillEllipse (pivot.x - 9.0f, pivot.y - 9.0f, 18.0f, 18.0f);
}

void VUEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Fundo: a sua arte inteira
    g.drawImage (background, bounds);

    // Daqui em diante, desenhamos na escala da arte (1440 x 760)
    g.addTransform (juce::AffineTransform::scale (bounds.getWidth() / kArtW,
                                                  bounds.getHeight() / kArtH));

    const int mode   = (int) proc.apvts.getRawParameterValue ("mode")->load();
    const float lamp = proc.apvts.getRawParameterValue ("lamp")->load();

    drawMeter (g, kPivotL, angL, "L", lamp, mode);
    drawMeter (g, kPivotR, angR, "R", lamp, mode);
}

void VUEditor::resized()
{
    // Controles na faixa inferior da janela (a arte deixa essa área livre)
    auto row = getLocalBounds().removeFromBottom (96).reduced (40, 12);
    const int w = row.getWidth() / 4;
    modeBox.setBounds (row.removeFromLeft (w).reduced (20, 26));
    refKnob.setBounds  (row.removeFromLeft (w).reduced (10));
    trimKnob.setBounds (row.removeFromLeft (w).reduced (10));
    lampKnob.setBounds (row.removeFromLeft (w).reduced (10));
}
