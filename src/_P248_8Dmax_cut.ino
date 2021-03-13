#include "_Plugin_Helper.h"
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#ifdef USES_P248

// #######################################################################################################
// ###################   Plugin 248 - 7-segment display plugin TM1637/MAX7219       ######################
// #######################################################################################################
//
// Chips/displays supported:
//  0 - TM1637     -- 2 pins - 4 digits and colon in the middle (XX:XX)
//  1 - TM1637     -- 2 pins - 4 digits and dot on each digit (X.X.X.X.)
//  2 - TM1637     -- 2 pins - 6 digits and dot on each digit (X.X.X.X.X.X.)
//  3 - MAX7219/21 -- 3 pins - 8 digits and dot on each digit (X.X.X.X.X.X.X.X.)
//
// Plugin can be setup as:
//  - Manual        -- display is manually updated sending commands
//                     "7dn,<number>"        (number can be negative or positive, even with decimal)
//                     "7dt,<temperature>"   (temperature can be negative or positive and containing decimals)
//                     "7dst,<hh>,<mm>,<ss>" (show manual time -not current-, no checks done on numbers validity!)
//                     "7dsd,<dd>,<mm>,<yy>" (show manual date -not current-, no checks done on numbers validity!)
//                     "7dtext,<text>"       (show free text - supported chars 0-9,a-z,A-Z," ","-","=","_","/","^")
//  - Clock-Blink     -- display is automatically updated with current time and blinking dot/lines
//  - Clock-NoBlink   -- display is automatically updated with current time and steady dot/lines
//  - Clock12-Blink   -- display is automatically updated with current time (12h clock) and blinking dot/lines
//  - Clock12-NoBlink -- display is automatically updated with current time (12h clock) and steady dot/lines
//  - Date            -- display is automatically updated with current date
//
// Generic commands:
//  - "7don"      -- turn ON the display
//  - "7doff"     -- turn OFF the display
//  - "7db,<0-15> -- set brightness to specific value between 0 and 15
//

#define PLUGIN_248
#define PLUGIN_ID_248           73
#define PLUGIN_NAME_248         "[TEST] Display - 8-dot display"
#define PLUGIN_248_DEBUG        true // activate extra log info in the debug


#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
//#define MAX_CLK_ZONES 2
//#define ZONE_SIZE 4
//#define MAX_DEVICES (MAX_CLK_ZONES * ZONE_SIZE)

#define ZONE_UPPER  1
#define ZONE_LOWER  0

//#define CLK_PIN   13
//#define DATA_PIN  11
//#define CS_PIN    10


#define SPEED_TIME  75
#define PAUSE_TIME  0

#define MAX_MESG  6

bool invertUpperZone = false;
// #define  DEBUG  0



struct P248_data_struct : public PluginTaskData_base {
  P248_data_struct()
    : pin1(-1), pin2(-1), pin3(-1), zone(0), size(0) {


//    zone(0), size(0), offset_x(0), offset_y(0),
//    brightness(0), timesep(false), shift(false) {
//      ClearBuffer();
    }


//  uint8_t showbuffer[8];
//  byte    spidata[2];
  uint8_t pin1, pin2, pin3;
  byte    zone = 2;
  byte    size = 4;
//  byte    offset_x;
//  byte    offset_y;
//  byte    brightness;
//  bool    timesep;
//  bool    shift;
};

char  szTimeL[MAX_MESG];    // mm:ss\0
char  szTimeH[MAX_MESG];

void getTime(char *psz, bool f = true)
{
  uint16_t  h, m;

//  m = millis()/1000;
  h = node_time.hour();
  m = node_time.minute();
  sprintf(psz, "%02d%c%02d", h, (f ? ':' : ' '), m);
}

  void createHString(char *pH, char *pL) {
    for (; *pL != '\0'; pL++)

      *pH++ = *pL | 0x80;   // offset character
      *pH = '\0'; // terminate the string
  }


// each char table is specific for each display and maps all numbers/symbols
MD_MAX72XX::fontType_t numeric7SegDouble[] PROGMEM =
{
    0,		// 0
    0,		// 1
    0,		// 2
    0,		// 3
    0,		// 4
    0,		// 5
    0,		// 6
    0,		// 7
    0,		// 8
    0,		// 9
    0,		// 10
    0,		// 11
    0,		// 12
    0,		// 13
    0,		// 14
    0,		// 15
    0,		// 16
    0,		// 17
    0,		// 18
    0,		// 19
    0,		// 20
    0,		// 21
    0,		// 22
    0,		// 23
    0,		// 24
    0,		// 25
    0,		// 26
    0,		// 27
    0,		// 28
    0,		// 29
    0,		// 30
    0,		// 31
    2, 0, 0,		// 32 - 'Space'
    0,		// 33 - '!'
    0,		// 34 - '"'
    0,		// 35 - '#'
    0,		// 36 - '$'
    0,		// 37 - '%'
    0,		// 38 - '&'
    0,		// 39 - '''
    0,		// 40 - '('
    0,		// 41 - ')'
    0,		// 42 - '*'
    0,		// 43 - '+'
    0,		// 44 - ','
    0,		// 45 - '-'
    2, 48, 48,		// 46 - '.'
    0,		// 47 - '/'
    10, 62, 127, 192, 192, 192, 192, 192, 192, 127, 62,		// 48 - '0'
    10, 0, 0, 0, 0, 0, 0, 0, 0, 127, 62,		// 49 - '1'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 64, 0,		// 50 - '2'
    10, 0, 65, 193, 193, 193, 193, 193, 193, 127, 62,		// 51 - '3'
    10, 0, 0, 1, 1, 1, 1, 1, 1, 127, 62,		// 52 - '4'
    10, 0, 64, 193, 193, 193, 193, 193, 193, 127, 62,		// 53 - '5'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 127, 62,		// 54 - '6'
    10, 0, 0, 0, 0, 0, 0, 0, 0, 127, 62,		// 55 - '7'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 127, 62,		// 56 - '8'
    10, 0, 64, 193, 193, 193, 193, 193, 193, 127, 62,		// 57 - '9'
    2, 6, 6,		// 58 - ':'
    0,		// 59 - ';'
    0,		// 60 - '<'
    0,		// 61 - '='
    0,		// 62 - '>'
    0,		// 63 - '?'
    0,		// 64 - '@'
    10, 62, 127, 1, 1, 1, 1, 1, 1, 127, 62,		// 65 - 'A'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 127, 62,		// 66 - 'B'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 65, 0,		// 67 - 'C'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 127, 62,		// 68 - 'D'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 65, 0,		// 69 - 'E'
    10, 62, 127, 1, 1, 1, 1, 1, 1, 1, 0,		// 70 - 'F'
    0,		// 71 - 'G'
    0,		// 72 - 'H'
    0,		// 73 - 'I'
    0,		// 74 - 'J'
    0,		// 75 - 'K'
    0,		// 76 - 'L'
    0,		// 77 - 'M'
    0,		// 78 - 'N'
    0,		// 79 - 'O'
    0,		// 80 - 'P'
    0,		// 81 - 'Q'
    0,		// 82 - 'R'
    0,		// 83 - 'S'
    0,		// 84 - 'T'
    0,		// 85 - 'U'
    0,		// 86 - 'V'
    0,		// 87 - 'W'
    0,		// 88 - 'X'
    0,		// 89 - 'Y'
    0,		// 90 - 'Z'
    0,		// 91 - '['
    0,		// 92 - '\'
    0,		// 93 - ']'
    0,		// 94 - '^'
    0,		// 95 - '_'
    0,		// 96 - '`'
    10, 62, 127, 1, 1, 1, 1, 1, 1, 127, 62,		// 97 - 'a'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 127, 62,		// 98 - 'b'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 65, 0,		// 99 - 'c'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 127, 62,		// 100 - 'd'
    10, 62, 127, 193, 193, 193, 193, 193, 193, 65, 0,		// 101 - 'e'
    10, 62, 127, 1, 1, 1, 1, 1, 1, 1, 0,		// 102 - 'f'
    0,		// 103 - 'g'
    0,		// 104 - 'h'
    0,		// 105 - 'i'
    0,		// 106 - 'j'
    0,		// 107 - 'k'
    0,		// 108 - 'l'
    0,		// 109 - 'm'
    0,		// 110 - 'n'
    0,		// 111 - 'o'
    0,		// 112 - 'p'
    0,		// 113 - 'q'
    0,		// 114 - 'r'
    0,		// 115 - 's'
    0,		// 116 - 't'
    0,		// 117 - 'u'
    0,		// 118 - 'v'
    0,		// 119 - 'w'
    0,		// 120 - 'x'
    0,		// 121 - 'y'
    0,		// 122 - 'z'
    0,		// 123 - '{'
    2, 255, 255,		// 124 - '|'
    0,		// 125
    0,		// 126
    0,		// 127
    0,		// 128
    0,		// 129
    0,		// 130
    0,		// 131
    0,		// 132
    0,		// 133
    0,		// 134
    0,		// 135
    0,		// 136
    0,		// 137
    0,		// 138
    0,		// 139
    0,		// 140
    0,		// 141
    0,		// 142
    0,		// 143
    0,		// 144
    0,		// 145
    0,		// 146
    0,		// 147
    0,		// 148
    0,		// 149
    0,		// 150
    0,		// 151
    0,		// 152
    0,		// 153
    0,		// 154
    0,		// 155
    0,		// 156
    0,		// 157
    0,		// 158
    0,		// 159
    2, 0, 0,		// 160
    0,		// 161
    0,		// 162
    0,		// 163
    0,		// 164
    0,		// 165
    0,		// 166
    0,		// 167
    0,		// 168
    0,		// 169
    0,		// 170
    0,		// 171
    0,		// 172
    0,		// 173
    2, 0, 0,		// 174
    0,		// 175
    10, 124, 254, 3, 3, 3, 3, 3, 3, 254, 124,		// 176
    10, 0, 0, 0, 0, 0, 0, 0, 0, 254, 124,		// 177
    10, 0, 2, 131, 131, 131, 131, 131, 131, 254, 124,		// 178
    10, 0, 130, 131, 131, 131, 131, 131, 131, 254, 124,		// 179
    10, 124, 254, 128, 128, 128, 128, 128, 128, 254, 124,		// 180
    10, 124, 254, 131, 131, 131, 131, 131, 131, 2, 0,		// 181
    10, 124, 254, 131, 131, 131, 131, 131, 131, 2, 0,		// 182
    10, 0, 2, 3, 3, 3, 3, 3, 3, 254, 124,		// 183
    10, 124, 254, 131, 131, 131, 131, 131, 131, 254, 124,		// 184
    10, 124, 254, 131, 131, 131, 131, 131, 131, 254, 124,		// 185
    2, 96, 96,		// 186
    0,		// 187
    0,		// 188
    0,		// 189
    0,		// 190
    0,		// 191
    0,		// 192
    10, 124, 254, 131, 131, 131, 131, 131, 131, 254, 124,		// 193
    10, 124, 254, 128, 128, 128, 128, 128, 128, 0, 0,		// 194
    10, 0, 0, 128, 128, 128, 128, 128, 128, 0, 0,		// 195
    10, 0, 0, 128, 128, 128, 128, 128, 128, 254, 124,		// 196
    10, 124, 254, 131, 131, 131, 131, 131, 131, 130, 0,		// 197
    10, 124, 254, 131, 131, 131, 131, 131, 131, 130, 0,		// 198
    0,		// 199
    0,		// 200
    0,		// 201
    0,		// 202
    0,		// 203
    0,		// 204
    0,		// 205
    0,		// 206
    0,		// 207
    0,		// 208
    0,		// 209
    0,		// 210
    0,		// 211
    0,		// 212
    0,		// 213
    0,		// 214
    0,		// 215
    0,		// 216
    0,		// 217
    0,		// 218
    0,		// 219
    0,		// 220
    0,		// 221
    0,		// 222
    0,		// 223
    0,		// 224
    10, 124, 254, 131, 131, 131, 131, 131, 131, 254, 124,		// 225
    10, 124, 254, 128, 128, 128, 128, 128, 128, 0, 0,		// 226
    10, 0, 0, 128, 128, 128, 128, 128, 128, 0, 0,		// 227
    10, 0, 0, 128, 128, 128, 128, 128, 128, 254, 124,		// 228
    10, 124, 254, 131, 131, 131, 131, 131, 131, 130, 0,		// 229
    10, 124, 254, 131, 131, 131, 131, 131, 131, 130, 0,		// 230
    0,		// 231
    0,		// 232
    0,		// 233
    0,		// 234
    0,		// 235
    0,		// 236
    0,		// 237
    0,		// 238
    0,		// 239
    0,		// 240
    0,		// 241
    0,		// 242
    0,		// 243
    0,		// 244
    0,		// 245
    0,		// 246
    0,		// 247
    0,		// 248
    0,		// 249
    0,		// 250
    0,		// 251
    2, 255, 255,		// 252
    0,		// 253
    0,		// 254
    0,		// 255
  };

boolean Plugin_248(byte function, struct EventStruct *event, String& string) {
  boolean success = false;

  switch (function) {
    case PLUGIN_DEVICE_ADD: {
      Device[++deviceCount].Number           = PLUGIN_ID_248;
      Device[deviceCount].Type               = DEVICE_TYPE_TRIPLE;
      Device[deviceCount].VType              = Sensor_VType::SENSOR_TYPE_NONE;
      Device[deviceCount].Ports              = 0;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = false;
      Device[deviceCount].ValueCount         = 0;
      Device[deviceCount].SendDataOption     = false;
      Device[deviceCount].TimerOption        = false;
      Device[deviceCount].TimerOptional      = false;
      Device[deviceCount].GlobalSyncOption   = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME: {
      string = F(PLUGIN_NAME_248);
      break;
    }

    case PLUGIN_WEBFORM_LOAD: {
      addFormNote(F("MAX7219: 1st=DIN-Pin, 2nd=CLK-Pin, 3rd=CS-Pin"));
      addFormNumericBox(F("Zone"), F("plugin_248_zone"), PCONFIG(0),
                        0, 5);
      addFormNumericBox(F("Size"), F("plugin_248_size"), PCONFIG(1),
                        0, 5);
  //    addFormNumericBox(F("Offset X"), F("plugin_248_offset_x"), PCONFIG(2),
  //                      0, 7);
  //    addFormNumericBox(F("Offset Y"), F("plugin_248_offset_y"), PCONFIG(3),
  //                      0, 7);
  //    addFormNumericBox(F("Brightness"), F("plugin_248_brightness"), PCONFIG(4),
  //                      0, 15);
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE: {
      PCONFIG(0) = getFormItemInt(F("plugin_248_zone"));
      PCONFIG(1) = getFormItemInt(F("plugin_248_size"));
//      PCONFIG(2) = getFormItemInt(F("plugin_248_offset_x"));
//      PCONFIG(3) = getFormItemInt(F("plugin_248_offset_y"));
//      PCONFIG(4) = getFormItemInt(F("plugin_248_brightness"));
      P248_data_struct *P248_data =
        static_cast<P248_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P248_data) {
        P248_data->pin1         = CONFIG_PIN1;
        P248_data->pin2         = CONFIG_PIN2;
        P248_data->pin3         = CONFIG_PIN3;
        P248_data->zone         = PCONFIG(0);
        P248_data->size         = PCONFIG(1);
//        P248_data->offset_x     = PCONFIG(2);
//        P248_data->offset_y     = PCONFIG(3);
//        P248_data->brightness   = PCONFIG(4);
//        P248_data->timesep      = true;

//        switch (PCONFIG(0)) {
//          p248_fillScreen(0);
//          p248_setIntensity(PCONFIG(4));

//        }
      }
      success = true;
      break;
    }

    case PLUGIN_EXIT: {
      success = true;
      break;
    }

    case PLUGIN_INIT: {
      initPluginTaskData(event->TaskIndex, new (std::nothrow) P248_data_struct());
      P248_data_struct *P248_data =
        static_cast<P248_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr == P248_data) {
        return success;
      }

      P248_data->pin1         = CONFIG_PIN1;
      P248_data->pin2         = CONFIG_PIN2;
      P248_data->pin3         = CONFIG_PIN3;
      P248_data->zone         = PCONFIG(0);
      P248_data->size         = PCONFIG(1);
  //    P248_data->offset_x     = PCONFIG(2);
  //    P248_data->offset_y     = PCONFIG(3);
  //    P248_data->brightness   = PCONFIG(4);

   uint8_t numDevices = PCONFIG(0) * PCONFIG(1);

          MD_Parola P = MD_Parola(HARDWARE_TYPE, CONFIG_PIN1, CONFIG_PIN2, CONFIG_PIN3, numDevices);




//     invertUpperZone = (HARDWARE_TYPE == MD_MAX72XX::GENERIC_HW || HARDWARE_TYPE == MD_MAX72XX::PAROLA_HW);

        // initialise the LED display
        P.begin(PCONFIG(0));

        // Set up zones for 2 halves of the display
        P.setZone(ZONE_LOWER, 0, PCONFIG(1) - 1);
        P.setZone(ZONE_UPPER, PCONFIG(1), numDevices - 1);
        P.setFont(numeric7SegDouble);

        P.setCharSpacing(P.getCharSpacing() * 2); // double height --> double spacing

  //      if (invertUpperZone)
  //      {
  //        P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_UD);
  //        P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_LR);

  //        P.displayZoneText(ZONE_LOWER, szTimeL, PA_RIGHT, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
  //        P.displayZoneText(ZONE_UPPER, szTimeH, PA_LEFT, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
  //      }
  //      else
  //      {
          P.displayZoneText(ZONE_LOWER, szTimeL, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
          P.displayZoneText(ZONE_UPPER, szTimeH, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
  //      }

      success = true;
      break;
    }

    case PLUGIN_WRITE: {

     String cmd = parseString(string, 1);
    //  cmd.toLowerCase();
    //  int text = parseStringToEndKeepCase(string, 2);
    //  if (cmd.equals("8dtext")) {
    //    return p248_plugin_write_8dtext(event, text); }
      if (cmd.equalsIgnoreCase(F("8dpix"))) {
//          p248_plugin_write_8dot(event, event->Par2, event->Par3);
         }

      success = true;
      break;
    }

    case PLUGIN_ONCE_A_SECOND: {
      P248_data_struct *P248_data =
        static_cast<P248_data_struct *>(getPluginTaskData(event->TaskIndex));

        P248_data->pin1         = CONFIG_PIN1;
        P248_data->pin2         = CONFIG_PIN2;
        P248_data->pin3         = CONFIG_PIN3;
        P248_data->zone         = PCONFIG(0);
        P248_data->size         = PCONFIG(1);


      MD_Parola P = MD_Parola(HARDWARE_TYPE, CONFIG_PIN1, CONFIG_PIN2, CONFIG_PIN3, PCONFIG(0) * PCONFIG(1));


//      if (nullptr == P248_data) {
//        break;
//      }

//      static uint32_t	lastTime = 0; // millis() memory
        static bool	flasher = false;  // seconds passing flasher




        P.displayAnimate();
       if (P.getZoneStatus(ZONE_LOWER) && P.getZoneStatus(ZONE_UPPER))
      {
          // Adjust the time string if we have to. It will be adjusted
          // every second at least for the flashing colon separator.
//          if (millis() - lastTime >= 1000)
//          {
//            lastTime = millis();
            getTime(szTimeL, flasher);
            createHString(szTimeH, szTimeL);
            flasher = !flasher;

            P.displayReset();

            // synchronise the start
            P.synchZoneStart();
      }
//        }



  }
  }
  return success;
}





#endif // USES_P248
