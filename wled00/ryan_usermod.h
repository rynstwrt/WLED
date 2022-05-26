#pragma once
#include "wled.h"
#include <Arduino.h>
#include <U8x8lib.h>
#include <string>
#include <sstream>

// OLED
#define OLED_PIN_SCL 5  // D1
#define OLED_PIN_SDA 4  // D2
#define OLED_ROWS 8
#define OLED_COLS 16

// Rotary encoder
#define CLK_PIN 14  // D6
#define DT_PIN 12  // D5
#define SW_PIN 13  //D7
#define SCROLL_STEP 5

// Modes
#define NUM_STATES 5

// Effects
#define NUM_EFFECTS 118
#define EFFECT_INDEX_START 1

// Palettes
#define NUM_PALETTES 71
#define PALETTE_INDEX_START 6

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE, OLED_PIN_SCL, OLED_PIN_SDA); // Pins are Reset, SCL, SDA


class RyanUsermod : public Usermod 
{
    private:
        unsigned long currentTime;
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
        void setup()
        {
            pinMode(CLK_PIN, INPUT_PULLUP);
            pinMode(DT_PIN, INPUT_PULLUP);
            pinMode(SW_PIN, INPUT_PULLUP);

            currentTime = millis();
            loopTime = currentTime;

            // TODO: test this?
            CRGB fastled_col;
            col[0] = fastled_col.Black;
            col[1] = fastled_col.Black;
            col[2] = fastled_col.Black;
            effectCurrent = effectIndex;
            effectPalette = paletteIndex;

            colorUpdated(CALL_MODE_DIRECT_CHANGE);
            updateInterfaces(CALL_MODE_DIRECT_CHANGE);

            u8x8.begin();
            u8x8.setPowerSave(0);
            u8x8.setContrast(255);

            u8x8.setFont(u8x8_font_chroma48medium8_r);
            u8x8.drawString(0, 1, "Press & turn");
            u8x8.drawString(0, 2, "to change modes.");
            u8x8.drawString(0, 5, "Turn to change");
            u8x8.drawString(0, 6, "values.");
            u8x8.setFont(u8x8_font_8x13_1x2_f);
        }


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

            return (String) lineBuffer;        
        }


        void updateOled()
        {
            lastOledUpdate = millis();

            u8x8.clear(); // sets cursor to (0, 0), too.
            u8x8.println(friendlyStateNames[selectedStateIndex]);
            u8x8.setCursor(0, 3);

            if (selectedStateIndex == 0) // effect mode
            {
                String name = getCurrentEffectOrPaletteName(true);

                // Assumes that if the name is too long
                // that it contains a space (mostly for "Fireworks Starburst").
                int nameLength = name.length();
                if (nameLength > OLED_COLS) 
                {
                    char *cstr = const_cast<char*>(name.c_str());
                    char *token = strtok(cstr, " ");

                    int currentLine = 3;
                    while (token != NULL)
                    {
                        u8x8.print(token);

                        currentLine += 2;
                        u8x8.setCursor(0, currentLine);

                        token = strtok(NULL, " ");
                    }
                }
                else
                {
                    u8x8.print(getCurrentEffectOrPaletteName(true));
                } 
            }
            else if (selectedStateIndex == 1) // palette mode
            {
                u8x8.print(getCurrentEffectOrPaletteName(false));
            }
            else if (selectedStateIndex == 2) // brightness mode
            {
                double val = bri;
                val /= 255;
                val *= 100;
                val = floor(val);

                int truncVal = val;
                u8x8.print(truncVal);

                int numDigits = (truncVal == 0) ? 1 : int(log10(truncVal) + 1);
                u8x8.setCursor(numDigits, 3);
                u8x8.println("%");
            }
            else if (selectedStateIndex == 3) // speed mode
            {
                double val = effectSpeed;
                val /= 255;
                val *= 100;
                val = floor(val);

                int truncVal = val;
                u8x8.print(truncVal);

                int numDigits = (truncVal == 0) ? 1 : int(log10(truncVal) + 1);
                u8x8.setCursor(numDigits, 3);
                u8x8.println("%");
            }
            else if (selectedStateIndex == 4) // intensity mode (fx specific mode)
            {
                double val = effectIntensity;
                val /= 255;
                val *= 100;
                val = floor(val);

                int truncVal = val;
                u8x8.print(truncVal);

                int numDigits = (truncVal == 0) ? 1 : int(log10(truncVal) + 1);
                u8x8.setCursor(numDigits, 3);
                u8x8.println("%");
            }
        }


        /**
         * @brief Called when the rotary encoder is rotated on a mode 
         * that doesn't use numbers, such as the effect and palette modes.
         * 
         * @param direction True if clockwise, false if counterclockwise.
         * @param type True if effect, false if palette.
         */
        void onEncoderRotatedQualitative(bool direction, bool type)
        {
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
            {
                relevantIndex = startIndex;
            }

            if (!direction && relevantIndex < 0)
            {
                relevantIndex = numIndexes - 1;
            }
                
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

            colorUpdated(CALL_MODE_DIRECT_CHANGE);
            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
        }


        /**
         * @brief Called when the rotary encoder is rotated on a mode 
         * that uses numbers, such as the brightness and intensity modes.
         * 
         * @param direction True if clockwise, false if counterclockwise. 
         * @param type 0 -> brightness    1 -> speed    2 -> intensity
         */
        void onEncoderRotatedQuantitative(bool direction, int type)
        {
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
                stateUpdated(CALL_MODE_DIRECT_CHANGE);
            else
                colorUpdated(CALL_MODE_DIRECT_CHANGE);

            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
        }


        /**
         * @param direction true if clockwise, false if counterclockwise. 
         */
        void onEncoderRotatedWhilePressed(bool direction) // clockwise when true, counterclockwise when false.
        {
            direction ? ++selectedStateIndex : --selectedStateIndex;

            if (selectedStateIndex >= numStates)
                selectedStateIndex = 0;
            else if (selectedStateIndex < 0)
                selectedStateIndex = NUM_STATES - 1;
        }


        void loop()
        {
            currentTime = millis();

            if (currentTime >= (loopTime + 2))
            {
                int encoderA = digitalRead(DT_PIN);
                int encoderB = digitalRead(CLK_PIN);
                buttonState = digitalRead(SW_PIN);

                if ((!encoderA) && encoderAPrev)
                {
                    if (oledTurnedOff)
                    {
                        u8x8.setPowerSave(0);
                        oledTurnedOff = false;
                        return;
                    }

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

                    updateOled();
                }
                
                if (!oledTurnedOff && millis() - lastOledUpdate > 5 * 60 * 1000)
                {
                    u8x8.setPowerSave(1);
                    oledTurnedOff = true;
                }

                encoderAPrev = encoderA;
                loopTime = currentTime;
            }
        }
};