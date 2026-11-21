# VU Steel (VST3)

VU meter estéreo com modos VU, PPM e Peak, interface de aço escovado.

## Baixar o plugin compilado
Aba **Actions** do repositório → último workflow verde → seção **Artifacts** → `VU-Steel-VST3`.
Extraia o zip e copie a pasta `VU Steel.vst3` para `C:\Program Files\Common Files\VST3\`.
No Cubase: Dispositivos → Gerenciador de Plug-ins → Rescan.

## Estrutura
- `CMakeLists.txt`: baixa o JUCE 8.0.4 automaticamente e gera o VST3.
- `.github/workflows/build.yml`: compila em Windows (Visual Studio 2022, x64).
- `Source/`: `PluginProcessor.h/.cpp` e `PluginEditor.h/.cpp` (copie do código da conversa anterior).

## Compilar localmente (opcional)
Requer Visual Studio 2022 com "Desenvolvimento para desktop com C++" e CMake.
    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --target VUSteel_VST3
