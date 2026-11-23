#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    // A arte foi desenhada em 1440 x 760
    constexpr float kArtW = 1440.0f;
    constexpr float kArtH = 760.0f;

    // Eixo da agulha e centro do mostrador de cada lado, medidos na arte
    const juce::Point<float> kPivotL  (448.0f, 372.0f);
    const juce::Point<float> kCentreL (449.0f, 286.0f);
    const juce::Point<float> kPivotR  (1004.0f, 372.0f);
    const juce::Point<float> kCentreR (1005.0f, 286.0f);

    constexpr float kNeedleLen = 196.0f;

    struct ScalePoint { float vu; float deg; };

    // Calibração: valor VU -> ângulo em graus (0 = vertical).
    // Valores estimados pela arte; ajuste aqui depois do teste.
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

    void drawScrew (juce::Graphics& g, juce::Point<float> c)
    {
        juce::ColourGradient cg (juce::Colour (0xffe6e8ea), c.x - 4.0f, c.y - 4.0f,
                                 juce::Colour (0xff5d6168), c.x + 5.0f, c.y + 5.0f, false);
        g.setGradientFill (cg);
        g.fillEllipse (c.x - 13.0f, c.y - 13.0f, 26.0f, 26.0f);
        g.setColour (juce::Colour (0xff2a2b2e));
        g.drawLine (c.x - 8.0f, c.y, c.x + 8.0f, c.y, 2.0f);
        g.drawLine (c.x, c.y - 8.0f, c.x, c.y + 8.0f, 2.0f);
    }
}

// ---------------------------------------------------------------------------
// Knob estilo console
// ---------------------------------------------------------------------------
void NeveLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider&)
{
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    const auto c = area.getCentre();
    const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    // Aro escuro com escala de 0 a 10
    g.setColour (juce::Colour (0xff17181b));
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    g.setColour (juce::Colour (0xffe9e6dc));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        g.drawLine (juce::Line<float> (polar (c, a, r * 0.80f), polar (c, a, r * 0.96f)),
                    i % 5 == 0 ? 2.4f : 1.2f);
    }

    // Capa em metal escuro
    const float capR = r * 0.66f;
    juce::ColourGradient cap (juce::Colour (0xff5a5f67), c.x - capR * 0.4f, c.y - capR * 0.5f,
                              juce::Colour (0xff0b0c0e), c.x + capR * 0.7f, c.y + capR, true);
    g.setGradientFill (cap);
    g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.5f);

    // Indicador de posição
    g.setColour (juce::Colour (0xfff4f1e8));
    g.drawLine (juce::Line<float> (polar (c, angle, capR * 0.25f), polar (c, angle, capR * 0.88f)), 3.0f);
}

// ---------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------
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
    // Janela redimensionável, mantendo a proporção da arte
    setResizable (true, true);
    setResizeLimits (720, 380, 2160, 1140);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio (kArtW / kArtH);

    modeBox.addItem ("VU", 1);
    modeBox.addItem ("PPM", 2);
    modeBox.addItem ("Peak", 3);
    modeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1d1f23));
    modeBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffeeebe2));
    modeBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff4a4e55));
    modeBox.setColour (juce::ComboBox::arrowColourId,      juce::Colour (0xffd8d4c8));
    addAndMakeVisible (modeBox);

    for (auto* k : { &refKnob, &trimKnob, &lampKnob })
    {
        k->setLookAndFeel (&neve);
        k->setSliderStyle (juce::Slider::RotaryVerticalDrag);
        k->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
        k->setColour (juce::Slider::textBoxTextColourId,       juce::Colour (0xff111111));
        k->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xffe9e6dc));
        k->setColour (juce::Slider::textBoxOutlineColourId,    juce::Colour (0xff50545a));
        addAndMakeVisible (k);
    }

    for (auto* l : { &modeLabel, &refLabel, &trimLabel, &lampLabel })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, juce::Colour (0xff1a1a1a));
        addAndMakeVisible (l);
    }
    modeLabel.setText ("MODO DE LEITURA", juce::dontSendNotification);
    refLabel.setText  ("REFERÊNCIA",       juce::dontSendNotification);
    trimLabel.setText ("CALIBRAÇÃO",       juce::dontSendNotification);
    lampLabel.setText ("LÂMPADA",          juce::dontSendNotification);

    setSize (1080, 570);
    startTimerHz (60);
}

VUEditor::~VUEditor()
{
    stopTimer();
    for (auto* k : { &refKnob, &trimKnob, &lampKnob })
        k->setLookAndFeel (nullptr);
}

juce::AffineTransform VUEditor::artToLocal() const
{
    return juce::AffineTransform::scale ((float) getWidth() / kArtW,
                                         (float) getHeight() / kArtH);
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

void VUEditor::drawMeter (juce::Graphics& g, juce::Point<float> pivot, juce::Point<float> centre,
                          float angleDeg, const juce::String& channel, float lamp, int mode) const
{
    const auto red  = juce::Colour (0xffb0201c);
    const auto dark = juce::Colour (0xff1a1a1a);

    // Flange preto com parafusos
    const juce::Rectangle<float> flange (centre.x - 231.0f, 62.0f, 462.0f, 463.0f);
    g.setColour (juce::Colour (0xff0c0c0e));
    g.fillRoundedRectangle (flange, 28.0f);
    g.setColour (juce::Colour (0xff3d4147));
    g.drawRoundedRectangle (flange.reduced (1.0f), 28.0f, 2.0f);
    drawScrew (g, { flange.getX() + 52.0f,     112.0f });
    drawScrew (g, { flange.getRight() - 52.0f, 112.0f });
    drawScrew (g, { flange.getX() + 52.0f,     470.0f });
    drawScrew (g, { flange.getRight() - 52.0f, 470.0f });

    // Anel do mostrador
    g.setColour (juce::Colour (0xff15161a));
    g.fillEllipse (centre.x - 224.0f, centre.y - 224.0f, 448.0f, 448.0f);
    g.setColour (juce::Colour (0xff4a4e55));
    g.drawEllipse (centre.x - 224.0f, centre.y - 224.0f, 448.0f, 448.0f, 2.0f);

    // Face branca, cortada na base
    g.saveState();
    g.reduceClipRegion (juce::Rectangle<int> (0, 0, 1440, 412));

    juce::ColourGradient faceGrad (juce::Colour (0xfffbfbf6), centre.x, centre.y - 120.0f,
                                   juce::Colour (0xffdedbd0), centre.x + 200.0f, centre.y + 200.0f, true);
    g.setGradientFill (faceGrad);
    g.fillEllipse (centre.x - 192.0f, centre.y - 192.0f, 384.0f, 384.0f);

    // Luz traseira
    if (lamp > 0.0f)
    {
        juce::ColourGradient glow (juce::Colour (0xffffd9a0).withAlpha (0.22f * lamp),
                                   centre.x, centre.y - 60.0f,
                                   juce::Colours::transparentBlack,
                                   centre.x, centre.y - 300.0f, true);
        g.setGradientFill (glow);
        g.fillEllipse (centre.x - 192.0f, centre.y - 192.0f, 384.0f, 384.0f);
    }

    // Marcas: maiores nos valores rotulados, menores nos intermediários
    for (int v = -20; v <= 5; ++v)
    {
        const float deg = vuToDeg ((float) v);
        const bool major = isLabelled (v);
        g.setColour (v > 0 ? red : dark);
        g.drawLine (juce::Line<float> (polar (pivot, deg, major ? 176.0f : 184.0f),
                                       polar (pivot, deg, 196.0f)),
                    major ? 2.6f : 1.4f);
    }

    // Faixa vermelha acima de 0 VU
    g.setColour (red.withAlpha (0.9f));
    for (float v = 0.0f; v <= 5.0f; v += 0.25f)
    {
        const float deg = vuToDeg (v);
        g.drawLine (juce::Line<float> (polar (pivot, deg, 184.0f), polar (pivot, deg, 196.0f)), 4.0f);
    }

    // Rótulos dos valores
    g.setFont (juce::Font (24.0f, juce::Font::bold));
    for (int i = 0; i < kScaleCount; ++i)
    {
        const float vu = kScale[i].vu;
        g.setColour (vu > 0.0f ? red : dark);
        g.drawText (juce::String (std::abs ((int) vu)),
                    juce::Rectangle<float> (60.0f, 34.0f)
                        .withCentre (polar (pivot, kScale[i].deg, 214.0f)),
                    juce::Justification::centred, false);
    }

    // Nome do modo
    const juce::String modeName = mode == 0 ? "VU" : (mode == 1 ? "PPM" : "PEAK");
    g.setColour (dark);
    g.setFont (juce::Font (40.0f, juce::Font::bold));
    g.drawText (modeName,
                juce::Rectangle<float> (200.0f, 60.0f).withCentre ({ pivot.x, pivot.y - 80.0f }),
                juce::Justification::centred, false);

    g.restoreState();

    // Base metálica e eixo
    const juce::Rectangle<float> bridge (centre.x - 115.0f, 382.0f, 230.0f, 30.0f);
    juce::ColourGradient metal (juce::Colour (0xffe6e8ea), bridge.getX(), bridge.getY(),
                                juce::Colour (0xff6f747b), bridge.getX(), bridge.getBottom(), false);
    g.setGradientFill (metal);
    g.fillRoundedRectangle (bridge, 6.0f);
    drawScrew (g, { centre.x - 95.0f, 397.0f });
    drawScrew (g, { centre.x + 95.0f, 397.0f });

    g.setColour (juce::Colour (0xff2a2d31));
    g.fillEllipse (pivot.x - 16.0f, pivot.y - 16.0f, 32.0f, 32.0f);

    // Botão decorativo inferior e identificação do canal
    g.setColour (juce::Colour (0xff1d1e22));
    g.fillEllipse (centre.x - 26.0f, 436.0f, 52.0f, 52.0f);
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawText (channel, juce::Rectangle<float> (60.0f, 30.0f).withCentre ({ centre.x, 506.0f }),
                juce::Justification::centred, false);

    // Agulha: sombra e corpo
    const auto tip = polar (pivot, angleDeg, kNeedleLen);
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.drawLine (juce::Line<float> (pivot.translated (3.0f, 4.0f), tip.translated (3.0f, 4.0f)), 3.0f);
    g.setColour (juce::Colour (0xff141414));
    g.drawLine (juce::Line<float> (pivot, tip), 2.8f);
}

void VUEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Fundo: a sua arte inteira
    g.drawImage (background, bounds);

    // Desenhos seguem a escala da arte (1440 x 760)
    g.addTransform (artToLocal());

    const int mode   = (int) proc.apvts.getRawParameterValue ("mode")->load();
    const float lamp = proc.apvts.getRawParameterValue ("lamp")->load();

    drawMeter (g, kPivotL, kCentreL, angL, "L", lamp, mode);
    drawMeter (g, kPivotR, kCentreR, angR, "R", lamp, mode);
}

void VUEditor::resized()
{
    const auto toL = [this] (juce::Rectangle<float> r)
    {
        return r.transformedBy (artToLocal()).toNearestInt();
    };

    modeBox.setBounds (toL ({ 220.0f, 600.0f, 240.0f, 56.0f }));

    refKnob.setBounds  (toL ({ 570.0f, 552.0f, 140.0f, 150.0f }));
    trimKnob.setBounds (toL ({ 730.0f, 552.0f, 140.0f, 150.0f }));
    lampKnob.setBounds (toL ({ 890.0f, 552.0f, 140.0f, 150.0f }));

    modeLabel.setBounds (toL ({ 190.0f, 556.0f, 300.0f, 36.0f }));
    refLabel.setBounds  (toL ({ 560.0f, 706.0f, 160.0f, 34.0f }));
    trimLabel.setBounds (toL ({ 720.0f, 706.0f, 160.0f, 34.0f }));
    lampLabel.setBounds (toL ({ 880.0f, 706.0f, 160.0f, 34.0f }));

    const juce::Font labelFont ((22.0f * (float) getWidth()) / kArtW, juce::Font::bold);
    for (auto* l : { &modeLabel, &refLabel, &trimLabel, &lampLabel })
        l->setFont (labelFont);
}
