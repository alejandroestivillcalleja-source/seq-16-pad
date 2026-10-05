#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Nombres e identificadores de todos los parametros del plugin.
// Cada pad (paso) tiene su propio juego completo de parametros: "p1_wave", "p2_wave", ...
namespace sq
{
    constexpr int numPads = 16;

    inline juce::String pid (int pad, const char* name)
    {
        return "p" + juce::String (pad + 1) + "_" + name;
    }

    inline const juce::StringArray sourceNames { "Oscilador", "Sample" };
    inline const juce::StringArray waveNames   { "Seno", "Sierra", "Cuadrada", "Triangulo", "Ruido" };

    enum Source { oscillator = 0, sample = 1 };
    enum Wave   { sine = 0, saw, square, triangle, noise };

    // Parametros por pad
    namespace id
    {
        inline constexpr const char* on      = "on";      // paso activo en la secuencia
        inline constexpr const char* source  = "src";     // Oscilador o Sample
        inline constexpr const char* wave    = "wave";
        inline constexpr const char* note    = "note";
        inline constexpr const char* fine    = "fine";
        inline constexpr const char* pEnv    = "penv";    // cantidad de envolvente de tono
        inline constexpr const char* pDecay  = "pdec";
        inline constexpr const char* start   = "start";   // inicio del sample
        inline constexpr const char* sPitch  = "spitch";  // tono del sample
        inline constexpr const char* attack  = "atk";
        inline constexpr const char* decay   = "dec";
        inline constexpr const char* sustain = "sus";
        inline constexpr const char* release = "rel";
        inline constexpr const char* gate    = "gate";
        inline constexpr const char* cutoff  = "cut";
        inline constexpr const char* reso    = "res";
        inline constexpr const char* level   = "lvl";
        inline constexpr const char* pan     = "pan";
    }

    // Parametros globales
    inline constexpr const char* bpmId    = "bpm";
    inline constexpr const char* masterId = "master";
    inline constexpr const char* swingId  = "swing";
}
