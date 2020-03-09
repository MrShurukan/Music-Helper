#include <iostream>
#include "olcNoiseMaker.h"
#include "helper.h"


struct ADSR
{
    double attackTime;
    double decayTime;
    double releaseTime;

    double sustainAmplitude;
    double startAmplitude;

    double triggerOnTime;
    double triggerOffTime;

    bool isNoteOn;

    ADSR()
    {
        attackTime = 0.02;
        decayTime = 0.01;
        startAmplitude = 1.0;
        sustainAmplitude = 0.8;
        releaseTime = 0.04;
        triggerOnTime = 0.0;
        triggerOffTime = 0.0;
        isNoteOn = false;
    }

    double getAmplitude(double dTime)
    {
        double amplitude = 0.0;

        if (isNoteOn)
        {
            double lifeTime = dTime - triggerOnTime;
            // Обработка ADS

            // Attack
            if (lifeTime <= attackTime)
            {
                // (lifeTime / attackTime) -> 0...1
                amplitude = (lifeTime / attackTime) * startAmplitude;
            }

            // Decay
            else if (lifeTime <= attackTime + decayTime)
            {
                double decayDifference = startAmplitude - sustainAmplitude;
                amplitude = startAmplitude - decayDifference * (lifeTime / (attackTime + decayTime));
            }

            // Sustain
            else
            {
                amplitude = sustainAmplitude;
            }
        }
        else
        {
            double lifeTime = dTime - triggerOffTime;
            // Обработка R

            // Release
            // sustain * (1...0)
            amplitude = sustainAmplitude * (1 - (lifeTime / releaseTime));
        }

        // Epsilon Check
        if (amplitude <= 0.0001)
        {
            amplitude = 0;
        }

        return amplitude;
    }

    void noteOn(double dTime)
    {
        triggerOnTime = dTime;
        isNoteOn = true;
    }

    void noteOff(double dTime)
    {
        triggerOffTime = dTime;
        isNoteOn = false;
    }
};

double base_frequency = 440.0; // A4
const double d12th_root_of_2 = pow(2.0, 1.0 / 12.0);

/*
 *  ---Octave 1---  ---Octave 2---  ---Octave 3 (+1)---
 *   S D    G H J    L :     2 3 4    6 7    9 0 -
 *  Z X C  V B N M  < > ?   Q W E R  T Y U  I O P {  }  
 *
 *  C_D_E__F_G_A_B__C_D_E   F_G_A_B__C_D_E__F_G_A_B__C
 */
const std::string keyboard = "ZSXDCVGBHNJM\xbcL\xbe\xba\xbfQ2W3E4RT6Y7UI9O0P\xbd\xdb\xdd";
// Массив состояний всех клавиш (нажата/нет)
bool* keysState = new bool[keyboard.length()];

// F2 - увеличение октавы, F1 - уменьшение
const std::string octaveKeys = { VK_F2 , VK_F1 }; // +-
// Состояние клавиш переключения октав в прошлом (нажаты/нет)
bool previousOctaveKeys[] = { false, false };

// Клавиша выхода из программы
const char exitKey = VK_NUMPAD0;

// std::atomic<double> amplitude = 0.5;
std::atomic<double> frequency = 0.0;
std::atomic<int> warmSawtoothSteps = 5;
ADSR envelope;

enum OscMode
{
    SIN,
    SQUARE,
    TRIANGLE,
    SAWTOOTH_WARM,
    SAWTOOTH_COARSE,
    NOISE
};

double osc(double hertzFreq, double dTime, OscMode mode)
{
    switch (mode)
    {
    case SIN:
        return sin(hertz(hertzFreq) * dTime);
    case SQUARE:
        return (sin(hertz(hertzFreq) * dTime) > 0.5 ? 1 : -1);
    case TRIANGLE:
        return (2 / PI) * asin(sin(hertz(hertzFreq) * dTime));
    case SAWTOOTH_WARM: {
        double sawValue = 0;
        for (int n = 1; n < warmSawtoothSteps; n++)
            sawValue += -sin(n * hertz(hertzFreq) * dTime) / static_cast<double>(n);

        return (2 / PI) * sawValue;
    }
    case SAWTOOTH_COARSE:
        return (2 / PI) * (hertzFreq * PI * fmod(dTime, 1.0 / hertzFreq) - PI / 2) + 1;
    case NOISE:
        return dRand(-1, 1);
    }
}

int getOctave()
{
    return log2(base_frequency / 55.0) + 1;
}

double SimpleSound(double dTime)
{
    return envelope.getAmplitude(dTime) * (
        + osc(frequency * 1, dTime, SAWTOOTH_WARM)
        + osc(frequency * 0.5, dTime, SIN)
    );
}

int main()
{
    vector<wstring> devices = olcNoiseMaker<short>::Enumerate();
    for (const auto& d : devices) std::wcout << "Found device: '" << d << "'\n";

    olcNoiseMaker<short> sound(devices[0]);
    sound.SetUserFunction(SimpleSound);

    // Инициализируем массив динамически, т.к. C++ не позволяет иначе с .length() :(
    keysState = new bool[keyboard.length()] {false,};

    std::cout << "\nThe synth is ready to go!\n";

    std::cout << "\n\n"
        "---Octave 1---  ---Octave 2---  ---Octave 3 (+1)---\n"
        " S D    G H J    L :     2 3 4    6 7    9 0 -\n"
        "Z X C  V B N M  < > ?   Q W E R  T Y U  I O P {  } \n"
        "\n"
        "C_D_E__F_G_A_B__C_D_E   F_G_A_B__C_D_E__F_G_A_B__C\n\n";

    std::cout << "\rOctave: " << getOctave();

    while (true)
    {
        bool isAnyKeyPlaying = false;
        for (int i = 0; i < keyboard.length(); i++)
        {
            if (keyIsHeld(keyboard[i]))
            {
                isAnyKeyPlaying = true;
                if (!keysState[i]) {
                    frequency = base_frequency * pow(d12th_root_of_2, i - 9);     // -9 преобразование из A в C.
                    std::cout << "\rKey On: " << frequency << " Hz       ";

                    envelope.noteOn(sound.GetTime());
                    keysState[i] = true;
                }
            }
            else keysState[i] = false;
        }

        //if (!isAnyKeyPlaying) amplitude = 0;
        if (!isAnyKeyPlaying && envelope.isNoteOn) {
            std::cout << "\rKey Off             ";
            envelope.noteOff(sound.GetTime());
        }

        /*-----------------------------*/

        // Переключение октавы наверх
        if (keyIsHeld(octaveKeys[0]))
        {
            if (!previousOctaveKeys[0]) {
                base_frequency *= 2;
                std::cout << "\rOctave: " << getOctave();

                previousOctaveKeys[0] = true;
                // Сбросить все клавиши, которые были нажаты
                for (int i = 0; i < keyboard.length(); i++) keysState[i] = false;
            }
        }
        else previousOctaveKeys[0] = false;

        // Переключение октавы вниз
        if (keyIsHeld(octaveKeys[1]))
        {
            if (!previousOctaveKeys[1]) {
                base_frequency /= 2;
                std::cout << "\rOctave: " << getOctave();

                previousOctaveKeys[1] = true;
                // Сбросить все клавиши, которые были нажаты
                for (int i = 0; i < keyboard.length(); i++) keysState[i] = false;
            }
        }
        else previousOctaveKeys[1] = false;


        if (keyIsHeld(exitKey))
        {
            sound.Stop();
            delete[] keysState;
            break;
        }
    }

    return 0;
}