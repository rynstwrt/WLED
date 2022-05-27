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
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE, OLED_PIN_SCL, OLED_PIN_SDA);

// Rotary encoder macros
#define CLK_PIN 14  // D6
#define DT_PIN 12  // D5
#define SW_PIN 13  //D7
#define SCROLL_STEP 5

// Mode macros
#define NUM_STATES 5

// Effect macros
#define NUM_EFFECTS 118
#define EFFECT_INDEX_START 1
#define MAX_EFFECT_NAME_PARTS 3  // The max potential number of space-separated words in a given effect name.

// Palette macros
#define NUM_PALETTES 71
#define PALETTE_INDEX_START 6

// Font macros
#define STARTUP_FONT u8x8_font_chroma48medium8_r
#define HEADER_FONT u8x8_font_8x13B_1x2_r
#define VALUE_FONT u8x8_font_8x13_1x2_r


class RyanUsermod : public Usermod 
{
    private:
        unsigned long lastLoopTime;
        unsigned long loopTime;

        static const int numStates = 5;
        int selectedStateIndex = 0;
        const String friendlyStateNames[numStates] = { "EFFECT:", "PALETTE:", "BRIGHTNESS:", "SPEED:", "INTENSITY:" };

        int effectIndex = EFFECT_INDEX_START;
        const int8_t bannedEffects[9] = { 0, 50, 62, 82, 83, 84, 96, 98, 116 };

        int paletteIndex = PALETTE_INDEX_START;
        const int8_t bannedPalettes[6] = { 0, 1, 2, 3, 4, 5 };
        
        unsigned char buttonState = HIGH;
        unsigned char prevButtonState = HIGH;

        unsigned char encoderAPrev = 0;

        long lastOledUpdate = 0;
        bool oledTurnedOff = true;


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

            CRGB fastled_col;
            col[0] = fastled_col.Black;
            col[1] = fastled_col.Black;
            col[2] = fastled_col.Black;
            effectCurrent = effectIndex;
            effectPalette = paletteIndex;

            colorUpdated(CALL_MODE_NO_NOTIFY);
            updateInterfaces(CALL_MODE_NO_NOTIFY);

            u8x8.begin();
            u8x8.setPowerSave(0);
            u8x8.setContrast(255);
            u8x8.setFont(STARTUP_FONT);

            u8x8.drawString(0, 0, "Press & turn");
            u8x8.drawString(0, 1, "to change modes.");
            u8x8.drawString(0, 3, "Turn to change");
            u8x8.drawString(0, 4, "values.");
        }


        /**
         * Called when the rotary encoder is rotated on a mode 
         * that doesn't use numbers, such as the effect and palette modes.
         * 
         * @param direction True if rotating clockwise, false if rotating counterclockwise.
         * @param type True if effect, false if palette.
         */
        void onEncoderRotatedQualitative(bool direction, bool type)
        {
            if (oledTurnedOff)
            {
                u8x8.setPowerSave(0);
                oledTurnedOff = false;
                return;
            }

            int relevantIndex = type ? effectIndex : paletteIndex;
            direction ? ++relevantIndex : --relevantIndex;

            int numBanned = type ? (sizeof(bannedEffects) / sizeof(*bannedEffects)) : (sizeof(bannedPalettes) / sizeof(*bannedPalettes));

            if (type)
            {
                while (std::find(bannedEffects, bannedEffects + numBanned, relevantIndex) != bannedEffects + numBanned)
                    direction ? ++relevantIndex : --relevantIndex;
            }
            else
            {
                while (std::find(bannedPalettes, bannedPalettes + numBanned, relevantIndex) != bannedPalettes + numBanned)
                    direction ? ++relevantIndex : --relevantIndex;
            }

            int numIndexes = type ? NUM_EFFECTS : NUM_PALETTES;
            int startIndex = type ? EFFECT_INDEX_START : PALETTE_INDEX_START;

            if (direction && relevantIndex >= numIndexes)
                relevantIndex = startIndex;
            else if (!direction && relevantIndex < 0)
                relevantIndex = numIndexes - 1;
                
            if (type)
            {
                effectIndex = relevantIndex;
                effectCurrent = effectIndex;
                strip.restartRuntime();
            }
            else
            {
                paletteIndex = relevantIndex;
                effectPalette = paletteIndex;
            }

            colorUpdated(CALL_MODE_NO_NOTIFY);
            updateInterfaces(CALL_MODE_NO_NOTIFY);
        }


        /**
         * Called when the rotary encoder is rotated on a mode 
         * that uses numbers, such as the brightness and intensity modes.
         * 
         * @param direction True if rotating clockwise, false if rotating counterclockwise. 
         * @param type 0 -> brightness    1 -> speed    2 -> intensity
         */
        void onEncoderRotatedQuantitative(bool direction, int type)
        {
            if (oledTurnedOff)
            {
                u8x8.setPowerSave(0);
                oledTurnedOff = false;
                return;
            }

            int currentVal = 0;

            if (type == 0)
                currentVal = bri;
            else if (type == 1)
                currentVal = effectSpeed;
            else if (type == 2)
                currentVal = effectIntensity;

            currentVal = (direction) ? currentVal + SCROLL_STEP : currentVal - SCROLL_STEP;

            if (direction && currentVal > 255)
                currentVal = 255;

            if (!direction && currentVal < 0)
                currentVal = 0;

            if (type == 0)
                bri = currentVal;
            else if (type == 1)
                effectSpeed = currentVal;
            else if (type == 2)
                effectIntensity = currentVal;

            if (type == 0)
                stateUpdated(CALL_MODE_NO_NOTIFY);
            else
                colorUpdated(CALL_MODE_NO_NOTIFY);

            updateInterfaces(CALL_MODE_NO_NOTIFY);
        }


        /**
         * @brief Called when the rotary encoder is rotated while pressed down.
         * 
         * @param direction True if rotating clockwise, false if rotating counterclockwise. 
         */
        void onEncoderRotatedWhilePressed(bool direction)
        {
            if (oledTurnedOff)
            {
                u8x8.setPowerSave(0);
                oledTurnedOff = false;
                return;
            }

            direction ? ++selectedStateIndex : --selectedStateIndex;

            if (selectedStateIndex >= numStates)
                selectedStateIndex = 0;
            else if (selectedStateIndex < 0)
                selectedStateIndex = NUM_STATES - 1;
        }


        /**
         * @brief Get the Current Effect Or Palette Name object
         * 
         * @param type True when effect, false when palette.
         * @return String The current effect or palette name.
         */
        String getCurrentEffectOrPaletteName(bool type) // effect when true, palette when false.
        {
            char lineBuffer[21];

            const char* relatedJSON  = type ? JSON_mode_names : JSON_palette_names;
            const uint8_t currentEffectOrMode = type ? effectCurrent : effectPalette;

            uint8_t printedChars = extractModeName(currentEffectOrMode, relatedJSON, lineBuffer, 19);

            if (lineBuffer[0]=='*' && lineBuffer[1]==' ')
            {
                for (byte i=2; i<=printedChars; i++)
                    lineBuffer[i-2] = lineBuffer[i]; //include '\0'

                printedChars -= 2;
            }

            return lineBuffer;
        }


        /**
         * @brief Converts a value to a percentage.
         * 
         * @param value The value in the range of [0, 255] to convert.
         * @return int The calculated percentage.
         */
        int valueToPercent(double value)
        {
            double val = value;
            val /= 255;
            val *= 100;
            val = floor(val);
            return (int) val;
        }


        /**
         * @brief Concat 2 datatypes into 1 string.
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
         * Updates the OLED to show the updated mode or value 
         * when the rotary encoder is turned.
         */
        void updateOled()
        {
            lastOledUpdate = millis();

            u8x8.setFont(HEADER_FONT);
            u8x8.drawString(0, 0, friendlyStateNames[selectedStateIndex].c_str());
            u8x8.setFont(VALUE_FONT);
            int valueLineNum = 2;

            if (selectedStateIndex == 0) // effect mode
            {
                // Assumes that if the name is too long that it contains a space (e.g. "Fireworks Starburst").
                // Also assumes that the first 2 words will fit on 1 line
                // if the effect is a 3 space-separated words.
                String name = getCurrentEffectOrPaletteName(true);
                int nameLength = name.length(); 

                if (nameLength > u8x8.getCols()) 
                {
                    char* cstr = const_cast<char*>(name.c_str());
                    char* token = strtok(cstr, " ");

                    int numTokens = 0;
                    char* tokens[MAX_EFFECT_NAME_PARTS] = { (char*) "" };

                    while (token != NULL)
                    {
                        tokens[numTokens] = token;
                        ++numTokens;
                        token = strtok(NULL, " ");
                    }

                    if (numTokens == 2)
                    {
                        u8x8.drawString(0, valueLineNum, tokens[0]);
                        u8x8.drawString(0, valueLineNum + 2, tokens[1]);
                    }
                    else if (numTokens == 3)
                    {
                        std::string firstLine = tokens[0] + (std::string) " " + (std::string) tokens[1];
                        u8x8.drawString(0, valueLineNum, firstLine.c_str());
                        u8x8.drawString(0, valueLineNum + 2, tokens[2]);
                    }
                }
                else
                {
                    u8x8.drawString(0, valueLineNum, name.c_str());
                } 
            }
            else if (selectedStateIndex == 1) // palette mode
            {
                u8x8.drawString(0, valueLineNum, getCurrentEffectOrPaletteName(false).c_str());
            }
            else // brightness, speed, and intensity modes
            {
                double value;

                switch(selectedStateIndex)
                {
                    case 2: // brightness
                        value = bri;
                        break;
                    case 3: // speed
                        value = effectSpeed;
                        break;
                    case 4: // intensity
                        value = effectIntensity;
                        break;
                    default:
                        value = NAN;
                }

                if (value == NAN) return;

                int percent = valueToPercent(value);
                std::string concat = typeConcat(percent, "%");
                u8x8.drawString(0, valueLineNum, concat.c_str());
            }
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

                if ((!encoderA) && encoderAPrev)
                {
                    if (encoderB == HIGH && buttonState == LOW) // Rotating clockwise while pressed
                    {
                        onEncoderRotatedWhilePressed(true);
                    }
                    else if (encoderB == HIGH) // Rotating clockwise
                    {
                        if (selectedStateIndex == 0) // effect
                            onEncoderRotatedQualitative(true, true);
                        else if (selectedStateIndex == 1) // palette
                            onEncoderRotatedQualitative(true, false);
                        else if (selectedStateIndex == 2) // brightness
                            onEncoderRotatedQuantitative(true, 0);
                        else if (selectedStateIndex == 3) // speed
                            onEncoderRotatedQuantitative(true, 1);
                        else if (selectedStateIndex == 4) // intensity
                            onEncoderRotatedQuantitative(true, 2);
                    }
                    else if (encoderB == LOW && buttonState == LOW) // Rotating counterclockwise while pressing.
                    {
                        onEncoderRotatedWhilePressed(false);
                    }
                    else if (encoderB == LOW) // Rotating counterclockwise.
                    {
                        if (selectedStateIndex == 0) // effect
                            onEncoderRotatedQualitative(false, true);
                        else if (selectedStateIndex == 1) // palette
                            onEncoderRotatedQualitative(false, false);
                        else if (selectedStateIndex == 2) // brightness
                            onEncoderRotatedQuantitative(false, 0);
                        else if (selectedStateIndex == 3) // speed
                            onEncoderRotatedQuantitative(false, 1);
                        else if (selectedStateIndex == 4) // intensity
                            onEncoderRotatedQuantitative(false, 2);
                    }

                    u8x8.clear();
                    updateOled();
                }
                else if (buttonState == LOW && oledTurnedOff) // Allow for button press down to wake OLED when asleep.
                {
                    u8x8.setPowerSave(0);
                    oledTurnedOff = false;
                    u8x8.clear();
                    updateOled();
                    return;
                }
                
                if (!oledTurnedOff && millis() - lastOledUpdate > 5 * 60 * 1000)
                {
                    u8x8.setPowerSave(1);
                    oledTurnedOff = true;
                }

                encoderAPrev = encoderA;
                lastLoopTime = millis();
            }
        }
};