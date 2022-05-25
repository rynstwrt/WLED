#pragma once

#include "wled.h"
#include <Arduino.h>
#include <U8x8lib.h>
#include <string>

#define OLED_PIN_SCL 5  // D1
#define OLED_PIN_SDA 4  // D2
#define OLED_REFRESH_RATE_MS 125

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE, OLED_PIN_SCL, OLED_PIN_SDA); // Pins are Reset, SCL, SDA


class RyanUsermod : public Usermod 
{
    private:
        unsigned long currentTime;
        unsigned long loopTime;

        static const int numStates = 5;
        int selectedStateIndex = 0;
        const String friendlyStateNames[numStates] = { "EFFECT", "PALETTE", "BRIGHTNESS", "SPEED", "FX SPECIFIC" };

        static const int numEffects = 118;
        static const int effectIndexStart = 1;
        int effectIndex = effectIndexStart;
        const int8_t bannedEffects[9] = { 0, 50, 62, 82, 83, 84, 96, 98, 116 };

        static const int numPalettes = 71;
        static const int paletteIndexStart = 6;
        int paletteIndex = paletteIndexStart;
        const int8_t bannedPalettes[6] = { 0, 1, 2, 3, 4, 5 };
        
        unsigned char buttonState = HIGH;
        unsigned char prevButtonState = HIGH;

        unsigned char encoderAPrev = 0;

        int clkPin = 14;  // d6
        int dtPin = 12;    // d5
        int swPin = 13;    // d7

        unsigned int scrollStep = 5;

        long lastOledUpdate = 0;
        bool oledTurnedOff = true;

        
    public:
        void setup()
        {
            pinMode(clkPin, INPUT_PULLUP);
            pinMode(dtPin, INPUT_PULLUP);
            pinMode(swPin, INPUT_PULLUP);

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
            //u8x8.setContrast(125);
            u8x8.setFont(u8x8_font_chroma48medium8_r);

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

            u8x8.clear(); // sets cursor to 0, 0.
            u8x8.println(friendlyStateNames[selectedStateIndex]);
            u8x8.setCursor(0, 1);

            // effect mode
            if (selectedStateIndex == 0)
            {
                u8x8.println("MODE:");
                u8x8.setCursor(1, 3);
                u8x8.print(getCurrentEffectOrPaletteName(true));
            }
            // palette mode
            else if (selectedStateIndex == 1)
            {
                u8x8.println("NAME:");
                u8x8.setCursor(1, 3);
                u8x8.print(getCurrentEffectOrPaletteName(false));
            }
            // brightness mode
            else if (selectedStateIndex == 2)
            {
                u8x8.println("PERCENT:");

                double val = bri;
                val /= 255;
                val *= 100;
                val = floor(val);

                u8x8.setCursor(1, 3);
                u8x8.print((int) val);
            }
            // speed mode
            else if (selectedStateIndex == 3)
            {
                u8x8.println("PERCENT:");

                double val = effectSpeed;
                val /= 255;
                val *= 100;
                val = floor(val);

                u8x8.setCursor(1, 3);
                u8x8.print((int) val);
            }
            // intensity mode (fx specific mode)
            else if (selectedStateIndex == 4)
            {
                u8x8.println("OPTION PERCENT:");

                double val = effectIntensity;
                val /= 255;
                val *= 100;
                val = floor(val);

                u8x8.setCursor(1, 3);
                u8x8.print((int) val);
            }
        }


        void loop()
        {
            currentTime = millis();

            if (currentTime >= (loopTime + 2))
            {
                int encoderA = digitalRead(dtPin);
                int encoderB = digitalRead(clkPin);
                buttonState = digitalRead(swPin);

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

                            if (effectIndex >= numEffects)
                            {
                                effectIndex = effectIndexStart;
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

                            if (paletteIndex >= numPalettes)
                            {
                                paletteIndex = paletteIndexStart;
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
                            currentBrightness += scrollStep;

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
                            currentSpeed += scrollStep;

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
                            currentIntensity += scrollStep;

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
                                effectIndex = numEffects - 1;
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
                                paletteIndex = numPalettes - 1;
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
                            currentBrightness -= scrollStep;

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
                            currentSpeed -= scrollStep;

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
                            currentIntensity -= scrollStep;

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