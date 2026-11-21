#include "PluginEditor.h"

VUEditor::VUEditor (VUMeterProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      modeAtt  (p.apvts, "mode",  modeBox),
      refAtt   (p.apvts, "ref",   refKnob),
      trimAtt  (p.apvts, "trim",  trimKnob),
      lampAtt  (p.apvts, "lamp",  lampKnob)
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

float VUEditor::degFor (float vu)
{
    // Escala -20 a +3 VU, varrendo de -45° a +45°
    const float t = juce::jlimit (0.0f, 1.0f, (vu + 20.0f) / 23.0f);
    return -45.0f + 90.0f * t;
}

void VUEditor::timerCallback()
{
    // Massa-mola amortecida (dt = 1/60 s): comportamento mecânico da agulha
    const float dt = 1.0f / 60.0f, k = 400.0f, c = 34.0f;

    auto step = [dt, k, c] (float target, float& ang, float& vel)
    {
        const float acc = k * (target - ang) - c * vel;
        vel += acc * dt;
        ang += vel * dt;
    };

    step (degFor (proc.levelL.load()), angL, velL);
    step (degFor (proc.levelR.load()), angR, velR);
    repaint();
}

juce::Image VUEditor::makeSteel (int w, int h) const
{
    juce::Image img (juce::Image::ARGB, w, h, true);
    juce::Graphics g (img);

    juce::ColourGradient base (juce::Colour (0xffb9bcc0), 0.0f, 0.0f,
                               juce::Colour (0xff8a8e94), 0.0f, (float) h, false);
    g.setGradientFill (base);
    g.fillAll();

    // Escovado: linhas horizontais finas e aleatórias (seed fixa = textura estável)
    juce::Random rnd (7);
    for (int i = 0; i < 3000; ++i)
    {
        const float x = rnd.nextFloat() * w;
        const float y = rnd.nextFloat() * h;
        const float len = 40.0f + rnd.nextFloat() * 260.0f;
        const float a = 0.02f + rnd.nextFloat() * 0.06f;
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (a));
        g.drawHorizontalLine ((int) y, x, x + len);
    }

    // Desgaste: arranhões pontuais e manchas de oxidação leve
    for (int i = 0; i < 40; ++i)
    {
        const float x = rnd.nextFloat() * w, y = rnd.nextFloat() * h;
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.drawLine (x, y, x + rnd.nextFloat() * 30.0f - 15.0f, y + rnd.nextFloat() * 6.0f, 0.6f);
    }
    for (int i = 0; i < 12; ++i)
    {
        const float x = rnd.nextFloat() * w, y = rnd.nextFloat() * h;
        g.setColour (juce::Colour (0xff6b5a44).withAlpha (0.05f));
        g.fillEllipse (x, y, 30.0f + rnd.nextFloat() * 60.0f, 20.0f + rnd.nextFloat() * 40.0f);
    }

    // Bordas levemente escurecidas pelo uso
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawRect (img.getBounds().toFloat(), 3.0f);

    return img;
}

juce::Image VUEditor::makePaper (int w, int h) const
{
    juce::Image img (juce::Image::ARGB, w, h, true);
    juce::Graphics g (img);

    juce::ColourGradient cg (juce::Colour (0xfff3ecd8), 0.0f, 0.0f,
                             juce::Colour (0xffd8ccae), 0.0f, (float) h, false);
    g.setGradientFill (cg);
    g.fillAll();

    juce::Random rnd (21);
    for (int i = 0; i < 1500; ++i)
    {
        g.setColour (juce::Colour (0x14000000));
        g.fillEllipse (rnd.nextFloat() * w, rnd.nextFloat() * h, 1.5f, 1.5f);
    }
    // Manchas de envelhecimento
    for (int i = 0; i < 6; ++i)
    {
        g.setColour (juce::Colour (0xff9b7f4a).withAlpha (0.05f));
        g.fillEllipse (rnd.nextFloat() * w, rnd.nextFloat() * h, 80.0f, 60.0f);
    }
    return img;
}

void VUEditor::drawScrew (juce::Graphics& g, juce::Point<float> c) const
{
    juce::ColourGradient cg (juce::Colour (0xffe0e2e6), c.x - 3, c.y - 3,
                             juce::Colour (0xff5d6168), c.x + 4, c.y + 4, false);
    g.setGradientFill (cg);
    g.fillEllipse (c.x - 6, c.y - 6, 12.0f, 12.0f);
    g.setColour (juce::Colour (0xff2a2b2e));
    g.drawLine (c.x - 4, c.y, c.x + 4, c.y, 1.2f);
}

void VUEditor::resized()
{
    auto b = getLocalBounds();
    steel = makeSteel (b.getWidth(), b.getHeight());

    auto meters = b.removeFromTop (b.getHeight() - 96).reduced (28);
    auto leftArea  = meters.removeFromLeft (meters.getWidth() / 2).reduced (10);
    auto rightArea = meters.reduced (10);

    faceL = leftArea.toFloat();
    faceR = rightArea.toFloat();
    paper = makePaper ((int) faceL.getWidth(), (int) faceL.getHeight());

    auto row = b.reduced (40, 12);
    const int w = row.getWidth() / 4;
    modeBox.setBounds (row.removeFromLeft (w).reduced (20, 26));
    refKnob.setBounds  (row.removeFromLeft (w).reduced (10));
    trimKnob.setBounds (row.removeFromLeft (w).reduced (10));
    lampKnob.setBounds (row.removeFromLeft (w).reduced (10));
}

void VUEditor::drawFace (juce::Graphics& g, juce::Rectangle<float> f,
                         float angleDeg, const juce::String& channel, float lamp, int mode)
{
    // Moldura escura em metal
    g.setColour (juce::Colour (0xff1c1d20));
    g.fillRoundedRectangle (f.expanded (7.0f), 8.0f);
    g.setColour (juce::Colour (0xff50545a));
    g.drawRoundedRectangle (f.expanded (7.0f), 8.0f, 1.5f);

    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle (f, 4.0f);
    g.reduceClipRegion (clip);

    // Papel do mostrador
    g.drawImageAt (paper, (int) f.getX(), (int) f.getY());

    // Iluminação traseira
    if (lamp > 0.0f)
    {
        juce::ColourGradient glow (juce::Colour (0xffffd9a0).withAlpha (0.35f * lamp),
                                   f.getCentreX(), f.getBottom(),
                                   juce::Colours::transparentBlack,
                                   f.getCentreX(), f.getY(), true);
        g.setGradientFill (glow);
        g.fillRect (f);
    }

    const auto piv = juce::Point<float> (f.getCentreX(), f.getBottom() - f.getHeight() * 0.10f);
    const float R = f.getHeight() * 0.80f;

    auto polar = [piv] (float deg, float rad)
    {
        const float a = juce::degreesToRadians (deg);
        return juce::Point<float> (piv.x + rad * std::sin (a), piv.y - rad * std::cos (a));
    };

    // Escala
    for (int v = -20; v <= 3; ++v)
    {
        const float d = degFor ((float) v);
        const bool major = (v % 5 == 0);
        const float r0 = R * (major ? 0.84f : 0.88f);
        const auto p0 = polar (d, r0), p1 = polar (d, R * 0.94f);

        g.setColour (v > 0 ? juce::Colour (0xffb0201c) : juce::Colour (0xff1a1a1a));
        g.drawLine (juce::Line<float> (p0, p1), major ? 2.0f : 1.0f);

        if (major)
        {
            const auto lp = polar (d, R * 0.68f);
            juce::String txt = (v > 0 ? "+" : "") + juce::String (v);
            if (v == 0) txt = "0";
            g.setColour (v > 0 ? juce::Colour (0xffb0201c) : juce::Colour (0xff1a1a1a));
            g.setFont (juce::Font (13.0f, juce::Font::bold));
            g.drawText (txt, juce::Rectangle<float> (34.0f, 16.0f).withCentre (lp),
                        juce::Justification::centred, false);
        }
    }

    // Arco vermelho na zona acima de 0 VU
    g.setColour (juce::Colour (0xffb0201c).withAlpha (0.8f));
    for (float d = degFor (0.0f); d <= degFor (3.0f); d += 0.8f)
        g.drawLine (juce::Line<float> (polar (d, R * 0.93f), polar (d, R * 0.98f)), 3.0f);

    // Rótulo do modo
    const juce::String modeName = mode == 0 ? "VU" : (mode == 1 ? "PPM" : "PEAK");
    g.setColour (juce::Colour (0xff1a1a1a).withAlpha (0.8f));
    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.drawText (modeName, juce::Rectangle<float> (60.0f, 16.0f)
                    .withCentre ({ f.getCentreX(), f.getBottom() - 34.0f }),
                juce::Justification::centred, false);

    // Agulha: sombra + corpo
    const auto tip = polar (angleDeg, R * 0.95f);
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.drawLine (juce::Line<float> (piv.translated (2.0f, 3.0f), tip.translated (2.0f, 3.0f)), 2.5f);
    g.setColour (juce::Colour (0xff111111));
    g.drawLine (juce::Line<float> (piv, tip), 2.0f);
    g.fillEllipse (piv.x - 6.0f, piv.y - 6.0f, 12.0f, 12.0f);

    // Reflexo do vidro na metade superior
    juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.10f), f.getCentreX(), f.getY(),
                                juce::Colours::transparentWhite, f.getCentreX(), f.getCentreY(), false);
    g.setGradientFill (gloss);
    g.fillRect (f.withHeight (f.getHeight() * 0.5f));

    g.restoreState();

    // Identificação do canal
    g.setColour (juce::Colour (0xff1a1a1a));
    g.setFont (juce::Font (11.0f, juce::Font::plain));
    g.drawText (channel, juce::Rectangle<float> (40.0f, 14.0f)
                    .withCentre ({ f.getX() + 26.0f, f.getBottom() - 16.0f }),
                juce::Justification::centred, false);
}

void VUEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (steel, 0, 0);

    const int mode = (int) proc.apvts.getRawParameterValue ("mode")->load();
    const float lamp = proc.apvts.getRawParameterValue ("lamp")->load();

    drawFace (g, faceL, angL, "L", lamp, mode);
    drawFace (g, faceR, angR, "R", lamp, mode);

    // Parafusos nos cantos
    const auto b = getLocalBounds().toFloat();
    for (auto p : { juce::Point<float> (14, 14), juce::Point<float> (b.getRight() - 14, 14),
                    juce::Point<float> (14, b.getBottom() - 14),
                    juce::Point<float> (b.getRight() - 14, b.getBottom() - 14) })
        drawScrew (g, p);

    // Placa de identificação
    g.setColour (juce::Colour (0xff2a2b2e));
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.drawText ("STEEL  VU  STEREO", juce::Rectangle<float> (300.0f, 14.0f)
                    .withCentre ({ b.getCentreX(), b.getBottom() - 92.0f }),
                juce::Justification::centred, false);
}
