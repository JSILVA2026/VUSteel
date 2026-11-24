#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    // Canvas = base da arte (INTERFACE_OK.png), 1440 x 760
    constexpr float kCanvasW = 1440.0f;
    constexpr float kCanvasH = 760.0f;

    // Posições e tamanhos exatos, na mesma escala do canvas
    const juce::Rectangle<float> kMeterRect[2] = {
        { 384.0f,  296.5f, 474.0f, 475.0f },
        { 1056.0f, 296.5f, 474.0f, 475.0f } };

    // Canto superior da base do ponteiro. O eixo fica no centro horizontal do medidor.
    const juce::Point<float> kBaseOrigin[2] = { { 383.0f, 400.0f }, { 1055.0f, 400.0f } };

    const juce::Rectangle<float> kPanelRect (1067.0f, 648.5f, 400.0f, 63.0f);

    const juce::Rectangle<float> kBtnRect[3] = {
        { 214.5f, 655.5f, 135.0f, 135.0f },
        { 419.0f, 655.5f, 135.0f, 135.0f },
        { 623.5f, 655.5f, 135.0f, 135.0f } };

    constexpr float kNeedleLen = 300.0f;

    struct ScalePoint { float vu; float deg; };

    // Calibração: valor VU -> ângulo em graus (0 = vertical). Ajustar após o teste.
    constexpr ScalePoint kScale[] = {
        { -20.0f, -42.0f }, { -10.0f, -33.0f }, { -7.0f, -23.0f }, { -5.0f, -11.0f },
        {  -3.0f,   1.0f }, {  -2.0f,   9.0f }, { -1.0f,  14.0f }, {  0.0f,  20.0f },
        {   1.0f,  27.0f }, {   2.0f,  32.0f }, {  3.0f,  36.0f }, {  5.0f,  44.0f }
    };
    constexpr int kScaleCount = (int) (sizeof (kScale) / sizeof (kScale[0]));

    juce::Point<float> polar (juce::Point<float> pivot, float deg, float r)
    {
        const float a = juce::degreesToRadians (deg);
        return { pivot.x + r * std::sin (a), pivot.y - r * std::cos (a) };
    }
}

// ---------------------------------------------------------------------------
// Knob com a imagem do botão, girando conforme o valor
// ---------------------------------------------------------------------------
void ImageKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float startAngle, float endAngle,
                                             juce::Slider&)
{
    if (! image.isValid())
        return;

    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    auto t = juce::AffineTransform::scale (bounds.getWidth()  / (float) image.getWidth(),
                                           bounds.getHeight() / (float) image.getHeight())
                 .translated (bounds.getX(), bounds.getY())
                 .rotated (angle, bounds.getCentreX(), bounds.getCentreY());

    g.drawImageTransformed (image, t);
}

// ---------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------
VUEditor::VUEditor (VUMeterProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      modeAtt (p.apvts, "mode", modeBox),
      refAtt  (p.apvts, "ref",  refKnob),
      trimAtt (p.apvts, "trim", trimKnob),
      lampAtt (p.apvts, "lamp", lampKnob)
{
    background = juce::ImageCache::getFromMemory (BinaryData::INTERFACE_OK_png, BinaryData::INTERFACE_OK_pngSize);

    medidorImg[0] = juce::ImageCache::getFromMemory (BinaryData::medidor_01_png, BinaryData::medidor_01_pngSize);
    medidorImg[1] = juce::ImageCache::getFromMemory (BinaryData::medidor_02_png, BinaryData::medidor_02_pngSize);

    btnImg[0] = juce::ImageCache::getFromMemory (BinaryData::botao_01_png, BinaryData::botao_01_pngSize);
    btnImg[1] = juce::ImageCache::getFromMemory (BinaryData::botao_02_png, BinaryData::botao_02_pngSize);
    btnImg[2] = juce::ImageCache::getFromMemory (BinaryData::botao_03_png, BinaryData::botao_03_pngSize);

    panelImg = juce::ImageCache::getFromMemory (BinaryData::panel_01_png, BinaryData::panel_01_pngSize);

    for (int i = 0; i < 3; ++i)
        knobLaf[i].setImage (btnImg[i]);

    setResizable (false, false);

    modeBox.addItem ("VU", 1);
    modeBox.addItem ("PPM", 2);
    modeBox.addItem ("Peak", 3);
    modeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1d1f23));
    modeBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffeeebe2));
    modeBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour (0xff4a4e55));
    addAndMakeVisible (modeBox);

    juce::Slider* knobs[3] = { &refKnob, &trimKnob, &lampKnob };
    for (int i = 0; i < 3; ++i)
    {
        knobs[i]->setLookAndFeel (&knobLaf[i]);
        knobs[i]->setSliderStyle (juce::Slider::RotaryVerticalDrag);
        knobs[i]->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (knobs[i]);
    }

    setSize (1080, 570);   // canvas 1440x760 a 75%
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
    return juce::AffineTransform::scale ((float) getWidth() / kCanvasW,
                                         (float) getHeight() / kCanvasH);
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

void VUEditor::drawNeedle (juce::Graphics& g, int i, float angleDeg) const
{
    const juce::Point<float> pivot (kMeterRect[i].getCentreX(), kBaseOrigin[i].y);

    // Base do ponteiro
    g.setColour (juce::Colour (0xff2a2d31));
    g.fillEllipse (pivot.x - 14.0f, pivot.y - 14.0f, 28.0f, 28.0f);

    // Agulha: sombra e corpo
    const auto tip = polar (pivot, angleDeg, kNeedleLen);
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.drawLine (juce::Line<float> (pivot.translated (3.0f, 4.0f), tip.translated (3.0f, 4.0f)), 3.0f);
    g.setColour (juce::Colour (0xff141414));
    g.drawLine (juce::Line<float> (pivot, tip), 2.8f);
}

void VUEditor::paint (juce::Graphics& g)
{
    // Daqui em diante, tudo na escala do canvas (1440 x 760)
    g.addTransform (artToLocal());

    // Fundo: a arte inteira, na mesma escala dos demais elementos
    g.drawImage (background, juce::Rectangle<float> (0.0f, 0.0f, kCanvasW, kCanvasH));

    // Ordem: ponteiro por baixo, medidor PNG por cima (parte transparente deixa o ponteiro aparecer)
    drawNeedle (g, 0, angL);
    drawNeedle (g, 1, angR);

    g.drawImage (medidorImg[0], kMeterRect[0]);
    g.drawImage (medidorImg[1], kMeterRect[1]);

    // Painel inferior
    g.drawImage (panelImg, kPanelRect);
}

void VUEditor::resized()
{
    const auto toL = [this] (juce::Rectangle<float> r)
    {
        return r.transformedBy (artToLocal()).toNearestInt();
    };

    juce::Slider* knobs[3] = { &refKnob, &trimKnob, &lampKnob };
    for (int i = 0; i < 3; ++i)
        knobs[i]->setBounds (toL (kBtnRect[i]));

    // Seletor de modo dentro do painel inferior
    modeBox.setBounds (toL ({ 1085.0f, 668.0f, 190.0f, 44.0f }));
}
