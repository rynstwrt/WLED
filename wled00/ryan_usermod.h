#pragma once
#include "wled.h"
#include <Arduino.h>
#include <u8g2lib.h>
#include <string>
#include <sstream>

// OLED
#define OLED_PIN_SCL 5  // D1
#define OLED_PIN_SDA 4  // D2
#define OLED_COLS 16  // Has 8 rows as well.
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_PIN_SCL, OLED_PIN_SDA);

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

// Fonts
#define STARTUP_FONT u8g2_font_chroma48medium8_8r
#define HEADER_FONT u8g2_font_8x13B_tf
#define VALUE_FONT u8g2_font_8x13_tf
#define HORIZONTAL_FONT_PADDING 2  // The left margin of each text line.
#define VERTICAL_FONT_PADDING 5  // The top margin of the first line of text.
#define FONT_LINE_SPACING 5  // How far each line are spaced from eachother (in pixels).


class RyanUsermod : public Usermod 
{
    private:
        unsigned long currentTime;
        unsigned long loopTime;

        static const int numStates = 5;
        int selectedStateIndex = 0;
        const String friendlyStateNames[numStates] = { "EFFECT", "PALETTE", "BRIGHTNESS", "SPEED", "INTENSITY" };

        int effectIndex = EFFECT_INDEX_START;
        const int8_t bannedEffects[9] = { 0, 50, 62, 82, 83, 84, 96, 98, 116 };

        int paletteIndex = PALETTE_INDEX_START;
        const int8_t bannedPalettes[6] = { 0, 1, 2, 3, 4, 5 };
        
        unsigned char buttonState = HIGH;
        unsigned char prevButtonState = HIGH;

        unsigned char encoderAPrev = 0;

        long lastOledUpdate = 0;
        bool oledTurnedOff = true;
        const int fontPixelHeights[3] = {6, 10, 9}; // In order of STARTUP_FONT, HEADER_FONT, and VALUE_FONT.


    public:
        /**
         * @brief Called once at boot. 
         */
        void setup()
        {
            pinMode(CLK_PIN, INPUT_PULLUP);
            pinMode(DT_PIN, INPUT_PULLUP);
            pinMode(SW_PIN, INPUT_PULLUP);

            currentTime = millis();
            loopTime = currentTime;

            CRGB fastled_col;
            col[0] = fastled_col.Black;
            col[1] = fastled_col.Black;
            col[2] = fastled_col.Black;
            effectCurrent = effectIndex;
            effectPalette = paletteIndex;

            colorUpdated(CALL_MODE_DIRECT_CHANGE);
            updateInterfaces(CALL_MODE_DIRECT_CHANGE);

            u8g2.begin();
            u8g2.setPowerSave(0);
            u8g2.setContrast(255);
            u8g2.setFont(STARTUP_FONT);

            int fontHeight = fontPixelHeights[0];
            int startupCurrentHeight = VERTICAL_FONT_PADDING + fontHeight;
            
            u8g2.drawStr(HORIZONTAL_FONT_PADDING, startupCurrentHeight, "Press & turn");
            
            startupCurrentHeight += fontHeight + FONT_LINE_SPACING;
            u8g2.drawStr(HORIZONTAL_FONT_PADDING, startupCurrentHeight, "to change modes.");

            startupCurrentHeight += 2 * (fontHeight + FONT_LINE_SPACING);
            u8g2.drawStr(HORIZONTAL_FONT_PADDING, startupCurrentHeight, "Turn to change");

            startupCurrentHeight += fontHeight + FONT_LINE_SPACING;
            u8g2.drawStr(HORIZONTAL_FONT_PADDING, startupCurrentHeight, "values.");
            
            u8g2.sendBuffer();
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
         * Called when the rotary encoder is rotated on a mode 
         * that uses numbers, such as the brightness and intensity modes.
         * 
         * @param direction True if rotating clockwise, false if rotating counterclockwise. 
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
         * @brief Called when the rotary encoder is rotated while pressed down.
         * 
         * @param direction True if rotating clockwise, false if rotating counterclockwise. 
         */
        void onEncoderRotatedWhilePressed(bool direction)
        {
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
         * Updates the OLED to show the updated mode or value 
         * when the rotary encoder is turned.
         */
        void updateOled()
        {
            lastOledUpdate = millis();

            u8g2.clearBuffer();
            u8g2.setFont(HEADER_FONT);
            int headerFontHeight = fontPixelHeights[1];
            int currentHeight = VERTICAL_FONT_PADDING + headerFontHeight;
            u8g2.drawStr(HORIZONTAL_FONT_PADDING, currentHeight, friendlyStateNames[selectedStateIndex].c_str());

            u8g2.setFont(VALUE_FONT);
            int valueFontHeight = fontPixelHeights[2];
            currentHeight += 2 * (valueFontHeight + FONT_LINE_SPACING);

            if (selectedStateIndex == 0) // effect mode
            {
                // Assumes that if the name is too long
                // that it contains a space (mostly for "Fireworks Starburst").
                String name = getCurrentEffectOrPaletteName(true);
                int nameLength = name.length();

                if (nameLength > OLED_COLS) 
                {
                    char *cstr = const_cast<char*>(name.c_str());
                    char *token = strtok(cstr, " ");

                    while (token != NULL)
                    {
                        u8g2.drawStr(HORIZONTAL_FONT_PADDING, currentHeight, token);
                        currentHeight += valueFontHeight + FONT_LINE_SPACING;
                        token = strtok(NULL, " ");
                    }
                }
                else
                {
                    u8g2.drawStr(HORIZONTAL_FONT_PADDING, currentHeight, name.c_str());
                } 
            }
            else if (selectedStateIndex == 1) // palette mode
            {
                u8g2.drawStr(HORIZONTAL_FONT_PADDING, currentHeight, getCurrentEffectOrPaletteName(false).c_str());
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
                u8g2.drawUTF8(HORIZONTAL_FONT_PADDING, currentHeight, percent + "%");
            }

            u8g2.sendBuffer();
        }


        /**
         * @brief Called continuously. For reading events, reading sensors, etc.
         */
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
                        u8g2.setPowerSave(0);
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
                    u8g2.setPowerSave(1);
                    oledTurnedOff = true;
                }

                encoderAPrev = encoderA;
                loopTime = currentTime;
            }
        }
};