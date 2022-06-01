#pragma once
#include "wled.h"
#include <Arduino.h>
#include <U8x8lib.h>
#include <string>
#include <sstream>

// Main loop macros
#define LOOP_POLL_DELAY_MS 2

// OLED macros
#define OLED_PIN_SCL 5  // D1
#define OLED_PIN_SDA 4  // D2
#define OLED_SLEEP_AFTER_MS 5 * 60 * 1000
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE, OLED_PIN_SCL, OLED_PIN_SDA);

// Rotary encoder macros
#define CLK_PIN 14  // D6
#define DT_PIN 12  // D5
#define SW_PIN 13  //D7
#define BUTTON_PRESS_DEBOUNCE_MS 1000 / 4

// Effect macros
#define NUM_EFFECTS 118
#define EFFECT_INDEX_START 2
#define MAX_EFFECT_NAME_PARTS 3  // The max potential number of space-separated words in a given effect name.

// Palette macros
#define NUM_PALETTES 71
#define PALETTE_INDEX_START 6

// Font macros
#define STARTUP_FONT u8x8_font_victoriabold8_u 
#define GENERAL_FONT u8x8_font_victoriabold8_r

// Screen macros
#define NUM_SCREENS 3
#define TOTAL_NUM_OPTIONS 6


class RyanUsermod : public Usermod 
{
    private:
        unsigned long lastLoopTime;
        unsigned long loopTime;

        int effectIndex = EFFECT_INDEX_START;
        const int8_t bannedEffects[11] = { 0, 1, 32, 50, 62, 82, 83, 84, 96, 98, 116 };

        int paletteIndex = PALETTE_INDEX_START;
        const int8_t bannedPalettes[6] = { 0, 1, 2, 3, 4, 5 };
        
        unsigned char encoderAPrev = 0;
        unsigned char buttonState = HIGH;
        unsigned long lastButtonPressTime = 0;

        long lastOledUpdate = 0;
        bool oledTurnedOff = true;

        const int8_t numOptionsPerScreen[3] = { 2, 3, 1 };
        bool optionSelected = false;
        int optionIndex = 0;

        int brightnessIndex = 5;
        int speedIndex = 5;
        int intensityIndex = 5;
        

    public:
        /**
         * @brief Called once at boot. 
         */
        void setup()
        {
            pinMode(CLK_PIN, INPUT_PULLUP);
            pinMode(DT_PIN, INPUT_PULLUP);
            pinMode(SW_PIN, INPUT_PULLUP);

            lastLoopTime = millis();
            loopTime = lastLoopTime;

            // Start with the matrix on black (off).
            CRGB fastled_col;
            col[0] = fastled_col.Black;
            col[1] = fastled_col.Black;
            col[2] = fastled_col.Black;
            effectCurrent = 0; 
            effectPalette = paletteIndex;
            colorUpdated(CALL_MODE_NO_NOTIFY);
            updateInterfaces(CALL_MODE_NO_NOTIFY);

            u8x8.begin();
            u8x8.setPowerSave(0);
            u8x8.setContrast(255);
            u8x8.setFont(STARTUP_FONT);

            std::string startupHeader = "LED MATRIX";
            u8x8.drawString(0, 0, startupHeader.c_str());
            
            uint8_t tiles[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
            int startupHeaderLength = startupHeader.length();
            for (int i = 0; i < startupHeaderLength; ++i) // Draws a thin underline to the header.
            {
                u8x8.drawTile(i, 2, 1, tiles);
            }

            u8x8.drawString(0, 3, "CLICK TO");
            u8x8.drawString(0, 5, "BEGIN.");
            u8x8.setFont(GENERAL_FONT);
        }


        /**
         * @brief Called continuously. For reading events, reading sensors, etc.
         */
        void loop()
        {
            if (millis() - lastLoopTime > LOOP_POLL_DELAY_MS) 
            {
                int encoderA = digitalRead(DT_PIN);
                int encoderB = digitalRead(CLK_PIN);
                buttonState = digitalRead(SW_PIN);

                if (buttonState == LOW && oledTurnedOff) // Allow for button press down to wake OLED when asleep.
                {
                    lastButtonPressTime = millis();
                    u8x8.setPowerSave(0);
                    oledTurnedOff = false;
                    u8x8.clear();
                    updateOled();
                    return;
                }
                else if (buttonState == LOW && millis() - lastButtonPressTime > BUTTON_PRESS_DEBOUNCE_MS) // Button pressed down to select an option.
                {
                    lastButtonPressTime = millis();
                    optionSelected = !optionSelected;
                    updateOled();
                }
                else if (!encoderA && encoderAPrev) // If knob rotated.
                {
                    if (oledTurnedOff) // Allow for rotation to wake OLED when asleep.
                    {
                        u8x8.setPowerSave(0);
                        oledTurnedOff = false;
                        u8x8.clear();
                        updateOled();
                        return;
                    }

                    bool clockwise = (encoderB == HIGH);

                    if (optionSelected) 
                    {
                        if (optionIndex == 0) // Effect
                        {
                            clockwise ? ++effectIndex : --effectIndex;

                            int numBanned = sizeof(bannedEffects) / sizeof(*bannedEffects);
                            
                            while (std::find(bannedEffects, bannedEffects + numBanned, effectIndex) != bannedEffects + numBanned)
                                clockwise ? ++effectIndex : --effectIndex;

                            if (clockwise && effectIndex >= NUM_EFFECTS)
                                effectIndex = EFFECT_INDEX_START;
                            else if (!clockwise && effectIndex < 0)
                                effectIndex = NUM_EFFECTS - 1;

                            effectCurrent = effectIndex;
                            strip.restartRuntime();
                            colorUpdated(CALL_MODE_NO_NOTIFY);
                        }
                        else if (optionIndex == 1) // Palette
                        {
                            clockwise ? ++paletteIndex : --paletteIndex;

                            int numBanned = sizeof(bannedPalettes) / sizeof(*bannedPalettes);
                            
                            while (std::find(bannedPalettes, bannedPalettes + numBanned, paletteIndex) != bannedPalettes + numBanned)
                                clockwise ? ++paletteIndex : --paletteIndex;

                            if (clockwise && paletteIndex >= NUM_EFFECTS)
                                paletteIndex = EFFECT_INDEX_START;
                            else if (!clockwise && paletteIndex < 0)
                                paletteIndex = NUM_EFFECTS - 1;

                            effectPalette = paletteIndex;
                            colorUpdated(CALL_MODE_NO_NOTIFY);
                        }
                        else if (optionIndex == 2) // Brightness
                        {
                            clockwise ? ++brightnessIndex : --brightnessIndex;

                            if (brightnessIndex > 10)
                                brightnessIndex = 10;
                            else if (brightnessIndex < 0)
                                brightnessIndex = 0;

                            bri = quantitativeIndexToRange(brightnessIndex);
                            stateUpdated(CALL_MODE_NO_NOTIFY);
                        }
                        else if (optionIndex == 3) // Speed
                        {
                            clockwise ? ++speedIndex : --speedIndex;

                            if (speedIndex > 10)
                                speedIndex = 10;
                            else if (speedIndex < 0)
                                speedIndex = 0;

                            effectSpeed = quantitativeIndexToRange(speedIndex);
                            strip.restartRuntime();
                            colorUpdated(CALL_MODE_NO_NOTIFY);
                        }
                        else if (optionIndex == 4) // Intensity
                        {
                            clockwise ? ++intensityIndex : --intensityIndex;

                            if (intensityIndex > 10)
                                intensityIndex = 10;
                            else if (intensityIndex < 0)
                                intensityIndex = 0;

                            effectIntensity = quantitativeIndexToRange(intensityIndex);
                            colorUpdated(CALL_MODE_NO_NOTIFY);
                        }

                        updateInterfaces(CALL_MODE_NO_NOTIFY);
                    }
                    else 
                    {
                        (clockwise) ? ++optionIndex : --optionIndex;

                        if (clockwise && optionIndex >= TOTAL_NUM_OPTIONS)
                            optionIndex = 0;

                        if (!clockwise && optionIndex < 0)
                            optionIndex = TOTAL_NUM_OPTIONS - 1;
                    }

                    updateOled();
                }
                
                if (!oledTurnedOff && millis() - lastOledUpdate > OLED_SLEEP_AFTER_MS) // Sleep OLED if inactive.
                {
                    u8x8.setPowerSave(1);
                    oledTurnedOff = true;
                }

                encoderAPrev = encoderA;
                lastLoopTime = millis();
            }
        }


        /**
         * Updates the OLED to show the updated mode or value 
         * when the rotary encoder is turned.
         */
        void updateOled()
        {
            lastOledUpdate = millis();
            u8x8.clear();

            if (effectCurrent != effectIndex) // Handle resuming after startup on mode 0 (1/2).
            {
                effectCurrent = effectIndex;
                colorUpdated(CALL_MODE_NO_NOTIFY);
                updateInterfaces(CALL_MODE_NO_NOTIFY);
            }

            int screenIndex = 0;
            int optionCount = 0;
            for (int i = 0; i < NUM_SCREENS; ++i) // Calculate screenIndex from optionIndex.
            {
                int numOptions = numOptionsPerScreen[i];

                if (optionCount + numOptions >= optionIndex + 1)
                {
                    screenIndex = i;
                    break;
                }
                else
                {
                    optionCount += numOptions;
                }
            }

            if (screenIndex == 0) // Effect and palette screen.
            {
                std::string effectLabel = " EFFECT: ";
                if (optionIndex == 0)
                    effectLabel = ">EFFECT: ";
                if (optionIndex == 0 && optionSelected)
                    u8x8.setInverseFont(1);
                u8x8.drawString(0, 0, effectLabel.c_str());
                u8x8.setInverseFont(0);
                u8x8.drawString(0, 2, typeConcat(" ", getCurrentEffectOrPaletteName(true).c_str()).c_str());

                std::string paletteLabel = " PALETTE: ";
                if (optionIndex == 1)
                    paletteLabel = ">PALETTE: ";
                if (optionIndex == 1 && optionSelected)
                    u8x8.setInverseFont(1);
                u8x8.drawString(0, 5, paletteLabel.c_str());
                u8x8.setInverseFont(0);
                u8x8.drawString(0, 7, typeConcat(" ", getCurrentEffectOrPaletteName(false).c_str()).c_str());
            }
            else if (screenIndex == 1) // Brightness, speed, and intensity screen.
            {
                std::string brightnessLabel = " BRI: ";
                if (optionIndex == 2)
                    brightnessLabel = ">BRI: ";
                if (optionIndex == 2 && optionSelected)
                    u8x8.setInverseFont(1);
                u8x8.drawString(0, 1, typeConcat(brightnessLabel.c_str(), typeConcat(brightnessIndex, "/10").c_str()).c_str());
                u8x8.setInverseFont(0);

                std::string speedLabel = " SPEED: ";
                if (optionIndex == 3)
                    speedLabel = ">SPEED: ";
                if (optionIndex == 3 && optionSelected)
                    u8x8.setInverseFont(1);
                u8x8.drawString(0, 3, typeConcat(speedLabel.c_str(), typeConcat(speedIndex, "/10").c_str()).c_str());
                u8x8.setInverseFont(0);

                std::string intensityLabel = " INTSTY: ";
                if (optionIndex == 4)
                    intensityLabel = ">INTSTY: ";
                if (optionIndex == 4 && optionSelected)
                    u8x8.setInverseFont(1);
                u8x8.drawString(0, 5, typeConcat(intensityLabel.c_str(), typeConcat(intensityIndex, "/10").c_str()).c_str());
                u8x8.setInverseFont(0);
            }
            else if (screenIndex == 2) // Save default screen.
            {
                // TODO:

                u8x8.drawString(0, 0, "save TODO uwu");
            }
        }


        /**
         * @brief Get the Current Effect Or Palette Name object
         * 
         * @param type True when effect, false when palette.
         * @return String The current effect or palette name.
         */
        std::string getCurrentEffectOrPaletteName(bool type) // effect when true, palette when false.
        {
            char lineBuffer[21];

            const char* relatedJSON  = type ? JSON_mode_names : JSON_palette_names;
            const uint8_t currentEffectOrMode = type ? effectCurrent : effectPalette;

            uint8_t printedChars = extractModeName(currentEffectOrMode, relatedJSON, lineBuffer, 19);

            if (lineBuffer[0] == '*' && lineBuffer[1] == ' ')
            {
                for (byte i = 2; i <= printedChars; i++)
                    lineBuffer[i - 2] = lineBuffer[i]; //include '\0'

                printedChars -= 2;
            }

            return lineBuffer;
        }


        /**
         * Concat 2 datatypes (mostly "int/const char*" and "std::string/const char*" pairs) 
         * into a single std::string.
         * 
         * @tparam T The datatype of the second parameter.
         * @tparam U The datatype of the second parameter.
         * @param a The value of the first argument.
         * @param b The value of the second argument.
         * @return std::string The concatinated result.
         */
        template<class T, class U>
        std::string typeConcat(T a, U b)
        {
            std::ostringstream oss;
            oss << a;
            oss << b;
            return oss.str();
        }


        /**
         * @brief Convert a value in [0, 10] to be a value in the range of [0, 255].
         * 
         * @param index The brightness, speed, or intensity index.
         * @return int The index value scaled to be in [0, 255].
         */
        int quantitativeIndexToRange(int index)
        {
            return (index * 255) / (10) + 0;
        }
};