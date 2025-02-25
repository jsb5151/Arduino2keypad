// file USBprotocol.cpp - methods for converting data into plain text strings transferred over USB serial

// if you want to change the format of the messages exchanged with your CPU via the USB serial port, update 
// this class

#include "USBprotocol.h"
#include "KeypadSerial.h"

// when arduino code inits, use these initial keypad values
#define INIT_MSG  "F7 z=00 t=0 c=0 r=0 l=0 a=0 s=0 p=1 y=0 b=1 1=ECP Translator   2=Ready    v1.90b1"

// macros to determine if command starts with 'F7' or 'F7A'
#define F7_MSG_ALT(s)        (*((s)+0) == 'F' && *((s)+1) == '7' && *((s)+2) == 'A')
#define F7_MSG(s)            (*((s)+0) == 'F' && *((s)+1) == '7')

// init class
void USBprotocol::init(void)
{
    count = 0;
    altMsgActive = false;
    
    // init F7 message structs
    initF7(&msgF7[0]);
    initF7(&msgF7[1]);

    parseRecv(INIT_MSG, strlen(INIT_MSG));
}

// initialize F7 message struct
void USBprotocol::initF7(t_MesgF7 * pMsgF7)
{
    memset((void *)pMsgF7, 0, sizeof(t_MesgF7));

    // constant values
    pMsgF7->type    = 0xF7;
    pMsgF7->keypads = 0xFF;  // send to all keypads
    pMsgF7->addr4   = 0x00;  // unknown value, my alarm panel is observed to send 0x10, but zero works
    pMsgF7->prog    = 0x00;  // programming mode
    pMsgF7->prompt  = 0x00;  // prompt position
    pMsgF7->zone    = 0xFC;  // my keypad may need this to prevent fast beep problem?
}

// parse received command string
uint8_t USBprotocol::parseRecv(const char * msg, const uint8_t len)
{
    if (len > 4 && F7_MSG_ALT(msg)) // an alt F7 command only updates the secondary F7 message
    {
        altMsgActive = true;
        return parseF7(msg+4, len-4, &msgF7[1]);  // parse the command after 'F7A '
    }
    else if (len > 4 && F7_MSG(msg)) // a primary F7 command sets altMsg false, so send F7A msg second if needed
    {
        count = 0;  // zero count so primary F7 msg is the next one displayed
        altMsgActive = false;
        return parseF7(msg+3, len-3, &msgF7[0]);  // parse the command after 'F7 '
    }
    return 0x0;  // received unknown command
}

// generate message from data received from keypad
const char * USBprotocol::keyMsg(char * buf, uint8_t bufLen, uint8_t addr, uint8_t len, uint8_t * pData, uint8_t type)
{
    // format of message is KEYS_XX[N] key0 key1 ... keyN-1, where XX is keypad number, N is key count
    // or                   UNK__XX[N] byte0 byte1 .. byteN-1 for unknown message from keypad XX with N bytes

    uint8_t idx = 0;
    idx += sprintf(buf+idx, "%s_%2d[%02d] ", type == KEYS_MESG ? "KEYS" : "UNK_", addr, len);
    for (uint8_t i=0; i < len && bufLen - idx > 6; i++)
    {
        idx += sprintf(buf+idx, "0x%02x ", *(pData+i));
    }
    sprintf(buf+idx-1, "\n");
    return (const char *)buf;
}

// parse F7 command, form is F7[A] z=FC t=0 c=1 r=0 l=0 a=0 s=0 p=1 y=0 k=0 x=00 b=1 1=1234567890123456 2=ABCDEFGHIJKLMNOP
//   z - zone             (byte arg)
//   t - tone             (nibble arg)
//   c - chime            (bool arg)
//   r - ready            (bool arg)
//   l - lowbat           (bool arg)*
//   a - arm-away         (bool arg)
//   s - arm-stay         (bool arg)
//   p - power-on         (bool arg)
//   y - bypass           (bool arg)*
//   k - program mode     (bool arg)*
//   x - prompt position  (byte arg)*
//   b - lcd-backlight-on (bool arg)
//   1 - line1 text       (16-chars)
//   2 - line2 text       (16-chars)

// BYTE1 tone notes
//   ? ? ? n x t t t
//         | | +-+-+--- tone values 0-7 (see below)
//         | +--------- 0 = low, 1 = louder (tbd)        
//         +----------- night?  
//
//   00-03 - low two bits define chime count for each F7 msg (0 none, 1,2,3 chime count per msg)
//   04    - fast pulsing tone (like there is an error, or timeout almost done)
//   05-06 - slow pulsing tone (like when alarm is in arm-delay and it is time to leave)
//   07    - continous tone (not pulsing)
//   08-16 - similar to above but louder
//   bits above bottom 3 don't do anything, 0x40 bit causes incompat. con. error

// BYTE2 notes: bit(0x80) 1 -> ARMED-STAY, bit(0x10) 1 -> READY (1 when ok, 0 when exit delay)
// BYTE3 notes: bit(0x20) 1 -> chime on, bit(0x08) 1 -> ac power ok, bit(0x04) 1 -> ARMED_AWAY

uint8_t USBprotocol::parseF7(const char * msg, uint8_t len, t_MesgF7 * pMsgF7)
{   
    bool success = true;
    bool lcd_backlight = false;

    t_MesgF7 newF7;
    t_MesgF7 * pNewF7 = &newF7;
    memcpy(pNewF7, pMsgF7, sizeof(t_MesgF7)); // copy existing F7 mesg struct

    for (uint8_t i=0; i < len && *(msg+i) != '\0'; i++)  // msg pointer starts after 'F7 ' or 'F7A '
    {
        if (*(msg+i) != ' ')  // skip over spaces
        {
            char parm = *(msg+i);
            i += 2;  // move past parm and '=', msg+i now points at arg 

             switch (parm)
            {
            case 'A': // testing for byte 1
                pNewF7->byte1 = GET_BYTE(*(msg+i), *(msg+i+1)); i += 2;
                break;
            case 'B': // testing for byte 2
                pNewF7->byte2 = GET_BYTE(*(msg+i), *(msg+i+1)); i += 2;
                break;
            case 'C': // testing for byte 3
                pNewF7->byte3 = GET_BYTE(*(msg+i), *(msg+i+1)); i += 2;
                break;
            case 'z': // zone
                pNewF7->zone = GET_BYTE(*(msg+i), *(msg+i+1)); i += 2;
                break;
            case 't': // tone
                pNewF7->byte1 = GET_NIBBLE(*(msg+i)); i++;
                break;
            case 'c': // chime
                pNewF7->byte3 = SET_CHIME(pNewF7->byte3, GET_BOOL(*(msg+i))); i++;
                break;
            case 'r': // ready state
                pNewF7->byte2 = SET_READY(pNewF7->byte2, GET_BOOL(*(msg+i))); i++;
                break;
            case 'l': // low battery
                pNewF7->byte2 = SET_LOWBAT(pNewF7->byte2, GET_BOOL(*(msg+i))); i++;
                break;
            case 'a': // arm away
                pNewF7->byte3 = SET_ARMED_AWAY(pNewF7->byte3, GET_BOOL(*(msg+i))); i++;
                break;
            case 's': // arm stay
                pNewF7->byte2 = SET_ARMED_STAY(pNewF7->byte2, GET_BOOL(*(msg+i))); i++;
                break;
            case 'p': // power
                pNewF7->byte3 = SET_POWER(pNewF7->byte3, GET_BOOL(*(msg+i))); i++;
                break;
            case 'y': // bypass
                pNewF7->byte3 = SET_BYPASS(pNewF7->byte3, GET_BOOL(*(msg+i))); i++;
                break;
            case 'k': // programing mode
                pNewF7->prog = GET_NIBBLE(*(msg+i)); i++;
                break;
            case 'x': // programming cursor position
                pNewF7->prompt = GET_BYTE(*(msg+i), *(msg+i+1)); i += 2;
                break;            
            case 'b': // backlight
                lcd_backlight = GET_BOOL(*(msg+i)); i++;
                break;
            case '1':  // line1 arg must occur after 'b' parameter for this code to work
                memset(pNewF7->line1, 0, LCD_LINE_LEN);
                for (uint8_t j=0; j < LCD_LINE_LEN && i < len; j++)
                {
                    pNewF7->line1[j] = *(msg+i) & 0x7f; 
                    i++;
                }
                pNewF7->line1[0] |= lcd_backlight ? 0x80 : 0x00;  // or in backlight bit
                break;
            case '2':
                memset(pNewF7->line2, 0, LCD_LINE_LEN);
                for (uint8_t j=0; j < LCD_LINE_LEN && i < len; j++)
                {
                    pNewF7->line2[j] = *(msg+i) & 0x7f; 
                    i++;
                }
                break;
            default:
                success = false;
                break;
            }
        }
    }

    if (success)
    {
        memcpy(pMsgF7, pNewF7, sizeof(t_MesgF7)); // replace existing F7 mesg with updated version

        pMsgF7->chksum = 0;

        for (uint8_t i=0; i < 44; i++)
        {
            pMsgF7->chksum += *(((uint8_t *)pMsgF7) + i);
        }

        pMsgF7->chksum = 0x100 - pMsgF7->chksum;  // two's compliment

        return 0xF7;
    }
    return 0;  // failed to parse message
}

// returned mesg always alternates between 2 stored messages (which may be the same)
const uint8_t * USBprotocol::getF7(void)
{ 
    if (altMsgActive)
        return (const uint8_t *)&(msgF7[count++ & 0x1]);
    else
        return (const uint8_t *)&(msgF7[0]);
}


#if 0
// debug: print message struct into buf (broken, needs to be updated)
const char * USBprotocol::printF7(char * buf)
{
    uint8_t idx = 0;
    idx += sprintf(buf+idx, "%02x msg -> kp[%02x]\n", pMsgF7->type, pMsgF7->keypads);
    idx += sprintf(buf+idx, "  zone=%02x, tone=%1x, chime=%c, power=%c\n", pMsgF7->zone, pMsgF7->byte1,
               GET_CHIME(pMsgF7->byte3) ? '1' : '0', GET_POWER(pMsgF7->byte3) ? '1' : '0');
    idx += sprintf(buf+idx, "  ready=%c, armed-away=%c, armed-stay=%c\n", GET_READY(pMsgF7->byte2) ? '1' : '0',
        GET_ARMED_AWAY(pMsgF7->byte3) ? '1' : '0', GET_ARMED_STAY(pMsgF7->byte2) ? '1' : '0');
    idx += sprintf(buf+idx, "  line1='");
    for (uint8_t i=0; i < 16; i++)
        *(buf+idx+i) = *(pMsgF7->line1+i) & 0x7F;
    idx += 16;
    idx += sprintf(buf+idx, "'\n");
    idx += sprintf(buf+idx, "  line2='");
    for (uint8_t i=0; i < 16; i++)
        *(buf+idx+i) = *(pMsgF7->line2+i) & 0x7F;
    idx += 16;
    idx += sprintf(buf+idx, "'\n");
    idx += sprintf(buf+idx, "checksum %02x\n", pMsgF7->chksum);
    return (const char *)buf;
}
#endif
