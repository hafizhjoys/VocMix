# VocMix (VST3, JUCE)

Build:
    git clone https://github.com/juce-framework/JUCE.git     # di dalam folder ini
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

Windows: butuh Visual Studio 2022 (workload C++). macOS: Xcode. Linux: paket dev JUCE (ALSA, X11, freetype, dll).
File .vst3 otomatis dicopy ke folder VST3 sistem (COPY_PLUGIN_AFTER_BUILD).
