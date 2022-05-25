#pragma once
#include "wled.h"
#include <Arduino.h>
#include <U8x8lib.h>
#include <string>

// OLED
#define OLED_PIN_SCL 5  // D1
#define OLED_PIN_SDA 4  // D2

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
        const String friendlyStateNames[numStates] = { "EFFECT:", "PALETTE:", "BRIGHTNESS:", "SPEED:", "FX SPECIFIC:" };

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

            // *** A graphics display with 128x64 pixel has 16 colums and 8 rows ***
            u8x8.begin();
            u8x8.setPowerSave(0);
            u8x8.setContrast(255);

            /*
                u8x8_font_8x13B_1x2_r:  TOO BIG
                u8x8_font_lucasarts_scumm_subtitle_o_2x2_r:  TOO BIG
                u8x8_font_lucasarts_scumm_subtitle_r_2x2_r:  TOO BIG
                u8x8_font_px437wyse700a_2x2_r:  TOO BIG

                u8x8_font_chroma48medium8_r:  GOOD BUT WANT BIGGER
                u8x8_font_pressstart2p_r:  GOOD BUT WANT BIGGER
                u8x8_font_torussansbold8_r: GOOD BUT WANT BIGGER
            */
           //
            u8x8.setFont(u8x8_font_pcsenior_u);

            u8x8.drawString(0, 1, "PRESS + TURN");
            u8x8.drawString(0, 2, "TO CHANGE");
            u8x8.drawString(0, 3, "THE MODE");
            u8x8.drawString(0, 5, "TURN TO");
            u8x8.drawString(0, 6, "CHANGE VALUE");
        }


        // effect when true, palette when false.
        String getCurrentEffectOrPaletteName(bool effect) 
        {
            char lineBuffer[21];

            const char* relatedJSON  = effect ? JSON_mode_names : JSON_palette_names;
            const uint8_t currentEffectOrMode = effect ? effectCurrent : effectPalette;

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
            u8x8.setCursor(0, 2);

            // effect mode
            if (selectedStateIndex == 0)
            {
                u8x8.print(getCurrentEffectOrPaletteName(true));
            }
            // palette mode
            else if (selectedStateIndex == 1)
            {
                u8x8.print(getCurrentEffectOrPaletteName(false));
            }
            // brightness mode
            else if (selectedStateIndex == 2)
            {
                double val = bri;
                val /= 255;
                val *= 100;
                val = floor(val);

                int truncVal = val;
                u8x8.print(truncVal);

                int numDigits = (truncVal == 0) ? 1 : int(log10(truncVal) + 1);
                u8x8.setCursor(numDigits, 2);
                u8x8.println("%");
            }
            // speed mode
            else if (selectedStateIndex == 3)
            {
                double val = effectSpeed;
                val /= 255;
                val *= 100;
                val = floor(val);

                int truncVal = val;
                u8x8.print(truncVal);

                int numDigits = (truncVal == 0) ? 1 : int(log10(truncVal) + 1);
                u8x8.setCursor(numDigits, 2);
                u8x8.println("%");
            }
            // intensity mode (fx specific mode)
            else if (selectedStateIndex == 4)
            {
                double val = effectIntensity;
                val /= 255;
                val *= 100;
                val = floor(val);

                int truncVal = val;
                u8x8.print(truncVal);

                int numDigits = (truncVal == 0) ? 1 : int(log10(truncVal) + 1);
                u8x8.setCursor(numDigits, 2);
                u8x8.println("%");
            }
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
                    // Rotating right while pressed
                    if (encoderB == HIGH && buttonState == LOW)
                    {
                        ++selectedStateIndex;

                        if (selectedStateIndex >= numStates)
                            selectedStateIndex = 0;

                        Serial.println("Rotating right while pressing.");
                    }
                    // Rotating right
                    else if (encoderB == HIGH) 
                    {
                        Serial.println("Rotating right.");

                        if (oledTurnedOff)
                        {
                            u8x8.setPowerSave(0);
                            oledTurnedOff = false;
                        }
                        // effect
                        else if (selectedStateIndex == 0)
                        {
                            Serial.print("effect change: ");

                            ++effectIndex;

                            int numBannedEffects = sizeof(bannedEffects) / sizeof(*bannedEffects);

                            while (std::find(bannedEffects, bannedEffects + numBannedEffects, effectIndex) != bannedEffects + numBannedEffects)
                            {
                                ++effectIndex;
                            }

                            if (effectIndex >= NUM_EFFECTS)
                            {
                                effectIndex = EFFECT_INDEX_START;
                            }

                            effectCurrent = effectIndex;

                            Serial.println(effectCurrent);

                            strip.restartRuntime();
                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }
                        // palette
                        else if (selectedStateIndex == 1)
                        {
                            Serial.print("palette change: ");
                            ++paletteIndex;

                            int numBannedPalettes = sizeof(bannedPalettes) / sizeof(*bannedPalettes);

                            while (std::find(bannedPalettes, bannedPalettes + numBannedPalettes, paletteIndex) != bannedPalettes + numBannedPalettes)
                            {
                                ++paletteIndex;
                            }

                            if (paletteIndex >= NUM_PALETTES)
                            {
                                paletteIndex = PALETTE_INDEX_START;
                            }
                            
                            effectPalette = paletteIndex;

                            Serial.println(effectPalette);

                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                            
                        }
                        // brightness
                        else if (selectedStateIndex == 2)
                        {
                            Serial.print("brightness change: ");

                            int currentBrightness = bri;
                            currentBrightness += SCROLL_STEP;

                            if (currentBrightness > 255)
                                currentBrightness = 255;

                            bri = currentBrightness;

                            Serial.println(bri);

                            stateUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }
                        // speed
                        else if (selectedStateIndex == 3)
                        {
                            Serial.print("speed change: ");
                            
                            int currentSpeed = effectSpeed;
                            currentSpeed += SCROLL_STEP;

                            if (currentSpeed > 255)
                                currentSpeed = 255;

                            effectSpeed = currentSpeed;

                            Serial.println(effectSpeed);

                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }
                        // intensity
                        else if (selectedStateIndex == 4)
                        {
                            Serial.print("intensity change: ");

                            int currentIntensity = effectIntensity;
                            currentIntensity += SCROLL_STEP;

                            if (currentIntensity > 255)
                                currentIntensity = 255;

                            effectIntensity = currentIntensity;

                            Serial.println(effectIntensity);

                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }
                    }
                    // Rotating left while pressing.
                    else if (encoderB == LOW && buttonState == LOW)
                    {
                        --selectedStateIndex;

                        if (selectedStateIndex < 0)
                            selectedStateIndex = numStates - 1;

                        Serial.println("Rotating left while pressing.");
                    }
                    // Rotating left.
                    else if (encoderB == LOW)
                    {
                        Serial.println("Rotating left.");

                        // effect
                        if (selectedStateIndex == 0)
                        {
                            Serial.print("effect change: ");
                            
                            --effectIndex;

                            int numBannedEffects = sizeof(bannedEffects) / sizeof(*bannedEffects);

                            while (std::find(bannedEffects, bannedEffects + numBannedEffects, effectIndex) != bannedEffects + numBannedEffects)
                            {
                                --effectIndex;
                            }

                            if (effectIndex < 0)
                            {
                                effectIndex = NUM_EFFECTS - 1;
                            }

                            effectCurrent = effectIndex;

                            Serial.println(effectCurrent);

                            strip.restartRuntime();
                            colorUpdated(CALL_MODE_BUTTON);
                            updateInterfaces(CALL_MODE_BUTTON);
                        }
                        // palette
                        else if (selectedStateIndex == 1)
                        {
                            Serial.print("palette change: ");

                            --paletteIndex;

                            int numBannedPalettes = sizeof(bannedPalettes) / sizeof(*bannedPalettes);

                            while (std::find(bannedPalettes, bannedPalettes + numBannedPalettes, paletteIndex) != bannedPalettes + numBannedPalettes)
                            {
                                --paletteIndex;
                            }

                            if (paletteIndex < 0)
                            {
                                paletteIndex = NUM_PALETTES - 1;
                            }

                            effectPalette = paletteIndex;

                            Serial.println(effectPalette);

                            colorUpdated(CALL_MODE_BUTTON);
                            updateInterfaces(CALL_MODE_BUTTON);
                        }
                        // brightness
                        else if (selectedStateIndex == 2)
                        {

                            Serial.print("brightness change: ");

                            int currentBrightness = bri;
                            currentBrightness -= SCROLL_STEP;

                            if (currentBrightness < 0)
                                currentBrightness = 0;

                            bri = currentBrightness;

                            Serial.println(bri);

                            stateUpdated(CALL_MODE_BUTTON);
                            updateInterfaces(CALL_MODE_BUTTON);
                        }
                        // speed
                        else if (selectedStateIndex == 3)
                        {
                            Serial.print("speed change: ");
                            
                            int currentSpeed = effectSpeed;
                            currentSpeed -= SCROLL_STEP;

                            if (currentSpeed < 0)
                                currentSpeed = 0;

                            effectSpeed = currentSpeed;

                            Serial.println(effectSpeed);

                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }
                        // intensity
                        else if (selectedStateIndex == 4)
                        {
                            Serial.print("intensity change: ");

                            int currentIntensity = effectIntensity;
                            currentIntensity -= SCROLL_STEP;

                            if (currentIntensity < 0)
                                currentIntensity = 0;

                            effectIntensity = currentIntensity;

                            Serial.println(effectIntensity);

                            colorUpdated(CALL_MODE_DIRECT_CHANGE);
                            updateInterfaces(CALL_MODE_DIRECT_CHANGE);
                        }

                        
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