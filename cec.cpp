/**
 *  Copyright © 2012 Ola Jeppsson
 *
 *  Subject to the conditions below, you may, without charge:
 *
 *  ·  Use, copy, modify and/or merge copies of this software and
 *     associated documentation files (the "Software")
 *
 *  ·  Publish, distribute, sub-licence and/or sell new software
 *     derived from or incorporating the Software.
 *
 *
 *  This file, unmodified, shall be included with all copies or
 *  substantial portions
 *  of the Software that are distributed in source code form.
 *
 *  Any person dealing with the Software shall not misrepresent
 *  the source of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *  KIND.
 */


extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#include <interface/vmcs_host/vc_cecservice.h>
#include <interface/vchiq_arm/vchiq_if.h>
}


#include "config.h"
#include "xbmcclient.h"
#include "cecxbmckeymap.h"


#ifndef VC_TRUE
#define VC_TRUE 1
#endif
#ifndef VC_FALSE
#define VC_FALSE 0
#endif

#ifndef VC_CEC_VENDOR_ID_NODEVICE
#define VC_CEC_VENDOR_ID_NODEVICE 0xffffff
#endif
#ifndef VC_CEC_VENDOR_ID_UNKNOWN
#define VC_CEC_VENDOR_ID_UNKNOWN 0
#endif

#define CEC_VENDOR_ID_LG 0xe091
#define CEC_VENDOR_ID_LG_QUIRK 0xffff

#define SL_COMMAND_UNKNOWN_01           0x01
#define SL_COMMAND_UNKNOWN_02           0x02

#define SL_COMMAND_REQUEST_POWER_STATUS 0xa0
#define SL_COMMAND_POWER_ON             0x03
#define SL_COMMAND_CONNECT_REQUEST      0x04
#define SL_COMMAND_SET_DEVICE_MODE      0x05


#define CEC_VENDOR_BUTTON_SAMSUNG_BACK 0x91

#define XBMC_KEY_ASCII 0xF100


class CECXBMCClient : public CXBMCClient
{
public:
    void SendButton(const char* Button,
            unsigned short Flags=(BTN_USE_NAME|BTN_NO_REPEAT))
    {
        // Simplest approach seems to be to emulate an XBOX R1 Remote.
        // The ideal solution imo is to send 'raw' CEC events
        // ie. "CEC_User_Control_Select" ... and then let XBMC
        // decide what to do.
        static char DeviceMap[] = "R1";
        CXBMCClient::SendButton(Button, DeviceMap, Flags);
    }

    void ping()
    {
        CXBMCClient::SendButton(XBMC_KEY_ASCII, BTN_NO_REPEAT);
    }
};

struct CECMessage {
    CECMessage(uint32_t param0, uint32_t param1, uint32_t param2,
            uint32_t param3, uint32_t param4)
    {
        reason = (VC_CEC_NOTIFY_T) CEC_CB_REASON(param0);
        retval = CEC_CB_RC(param0);

        VC_CEC_MESSAGE_T tmp;
        int res;
        res = vc_cec_param2message(param0, param1, param2, param3, param4, &tmp);
        if (res || tmp.length > CEC_MAX_XMIT_LENGTH) {
            length = 0;
            initiator = CEC_AllDevices_eUnRegistered;
            follower = CEC_AllDevices_eUnRegistered;
            memset(&payload[0], 0, sizeof(payload));
        } else {
            length = tmp.length;
            initiator = tmp.initiator;
            follower = tmp.follower;
            memcpy(&payload[0], &tmp.payload[0], sizeof(payload));
        }
    }

    VC_CEC_NOTIFY_T reason;
    uint8_t retval;

    uint32_t length;
    CEC_AllDevices_T initiator;
    CEC_AllDevices_T follower;
    uint8_t payload[CEC_MAX_XMIT_LENGTH+1];

    CEC_OPCODE_T opcode() const { return (CEC_OPCODE_T) payload[0]; }
    uint8_t operand1()    const { return payload[1]; }
    uint8_t operand2()    const { return payload[2]; }
};


CECXBMCClient xbmc;
uint32_t tvVendorId;
uint32_t myVendorId;
volatile uint8_t myPowerState = 0xFF;
volatile uint16_t physicalAddress;

//Needed by HandleComboKeys
volatile  uint8_t prevcode = 0xFF;  
volatile  uint8_t curcode = 0xFF; 
volatile  uint8_t AlarmFunctionSet = 0xFF; 

//Local functions
void vc_cec_report_power_status(uint8_t dest, CEC_POWER_STATUS_T status);
void vc_cec_report_physicalAddress(uint8_t dest);
void debug(const char* s, const CECMessage& msg);
void LgCecInit();

void HandleComboKeys(int sig)
{ 

    //LG HACK to send back key
    if(CEC_User_Control_Stop == prevcode && CEC_User_Control_Select == curcode)
    {
        // We will use combination of STOP and select key to simulate back operation
        printf("UserControlPressed: combo key detected sending back\n");
        prevcode=0xff;  
        curcode=0xff; 
        xbmc.SendButton("back");
        AlarmFunctionSet=0xff; 
        return;
    }
    else if(CEC_User_Control_Stop == prevcode && CEC_User_Control_Pause == curcode)
    { 
        // We will use combination of STOP and pause key to simulate home operation
        printf("UserControlPressed: combo key detected sending home\n");
        prevcode=0xff;  
        curcode=0xff; 
        xbmc.SendButton("menu");
        AlarmFunctionSet=0xff; 
        return;
    }
    else
    {
        prevcode=0xff;  
        curcode=0xff; 
        xbmc.SendButton("stop");
        AlarmFunctionSet=0xff; 
    }

}


void UserControlPressed(const CECMessage& msg)
{
    uint8_t keycode = msg.operand1();
    const char *xbmcKey = NULL;

    for (unsigned int i=0; i < CECXBMCKeymapElements; ++i) {
        if (CECXBMCKeymap[i].cec == keycode) {
            xbmcKey = CECXBMCKeymap[i].xbmc;
            break;
        }
    }

    if (xbmcKey != NULL) {
        
        //LG Hack to support back and menu keys
        prevcode= curcode;
        curcode = keycode;
        if(CEC_User_Control_Stop == curcode)
        {
            //set alarm handler and raise a alarm after a delay
            signal(SIGALRM, HandleComboKeys);
            AlarmFunctionSet = 0x1; 
            alarm(1);
        }
        else if(AlarmFunctionSet != 1)    // End LG Hack    
            xbmc.SendButton(xbmcKey);
    } else {
        printf("UserControlPressed: keycode=0x%x has no binding\n", keycode);
    }
}

void MenuRequest(const CECMessage& msg)
{
    if (msg.operand1() == CEC_MENU_STATE_QUERY) {
        vc_cec_send_MenuStatus(msg.initiator, CEC_MENU_STATE_ACTIVATED,
                VC_TRUE);
    } else {
        printf("MenuRequest: operand1=0x%x unknown\n", msg.operand1());
        // TODO: send FeatureAbort?
    }
}

void Play(const CECMessage& msg) {
    switch (msg.operand1()) {
    case CEC_PLAY_FORWARD: xbmc.SendButton("play");  break;
    case CEC_PLAY_STILL:   xbmc.SendButton("pause"); break;
    default:
        printf("Play: operand1=0x%x not implemented\n", msg.operand1());
    }
}

void DeckControl(const CECMessage& msg) {
    if (msg.operand1() == CEC_DECK_CTRL_STOP) {
        xbmc.SendButton("stop");
    } else {
        printf("DeckControl: operand1=0x%x not implemented\n", msg.operand1());
    }
}

void VendorCommand_LG(const CECMessage& msg)
{
    uint8_t response[8];
    switch (msg.operand1()) {
    case SL_COMMAND_UNKNOWN_01:
        printf("VendorCommand_LG: SL_COMMAND_UNKNOWN_01 received\n");
        // Send 0205
        response[0] = CEC_Opcode_VendorCommand;
        response[1] = 0x02;
        response[2] = 0x01;
        vc_cec_send_message(msg.initiator, response, 3, VC_TRUE);
        printf("VendorCommand_LG: Sent 0205\n");
        break;

        case SL_COMMAND_CONNECT_REQUEST:
        printf("VendorCommand_LG:SL_COMMAND_CONNECT_REQUEST\n");
        // Set device mode
        response[0] = CEC_Opcode_VendorCommand;
        response[1] = SL_COMMAND_SET_DEVICE_MODE;
        response[2] = CEC_DeviceType_Rec ;
        vc_cec_send_message(msg.initiator, response, 3, VC_TRUE);
 
        // Opcode 04 : ImageViewOn
        response[0] = CEC_Opcode_ImageViewOn ;
        vc_cec_send_message(CEC_TV_ADDRESS, response, 1, VC_FALSE);

        // Active source
        response[0] =CEC_Opcode_ActiveSource ;
        response[1] = (uint8_t) ((physicalAddress) >> 8 & 0xff);
        response[2] = (uint8_t) ((physicalAddress) >> 0 & 0xff);
        vc_cec_send_message(CEC_BROADCAST_ADDR, response, 3, VC_FALSE);

        vc_cec_set_osd_name("XBMC");
        break;

    case SL_COMMAND_POWER_ON:
        printf("VendorCommand_LG: TODO HandleVendorCommandPowerOn(command)\n");
        break;


    case SL_COMMAND_REQUEST_POWER_STATUS:
        printf("VendorCommand_LG: TODO HandleVendorCommandPowerOnStatus(command) \n");
        break;

    default:
        printf("VendorCommand_LG: unhandled command operand1=0x%x\n", msg.operand1());
    }
}

void VendorCommand(const CECMessage& msg)
{
    if (myVendorId == CEC_VENDOR_ID_LG ||
        myVendorId == CEC_VENDOR_ID_LG_QUIRK) {
        VendorCommand_LG(msg);
    } else {
        printf("VendorCommand: unhandled vendor command operand1=0x%x "
                "for vendor=0x%x\n", msg.operand1(), tvVendorId);
    }
}

void VendorRemoteButtonDown(const CECMessage& msg)
{
    char str[64];
    snprintf(str, 63, "VendorRemoteButtonDown(vendor=0x%x, operand1=0x%x):",
            tvVendorId, msg.operand1());
    debug(str, msg);

    // Temporary fix
    if (msg.operand1() == CEC_VENDOR_BUTTON_SAMSUNG_BACK) {
        xbmc.SendButton("back");
    }
}

void GiveDeviceVendorID(const CECMessage& msg) {
    uint8_t response[4];
    response[0] = CEC_Opcode_DeviceVendorID;
    response[1] = (uint8_t) ((myVendorId >> 16) & 0xff);
    response[2] = (uint8_t) ((myVendorId >> 8) & 0xff);
    response[3] = (uint8_t) ((myVendorId >> 0) & 0xff);
    vc_cec_send_message(msg.initiator, response, 4, VC_TRUE);
}

void GiveDevicePowerStatus(const CECMessage& msg)
{
    printf("cec_callback: received power status query \n");
    // Send CEC_Opcode_ReportPowerStatus
    uint8_t response[2];
    response[0] = CEC_Opcode_ReportPowerStatus;
    response[1] = CEC_POWER_STATUS_ON;
    vc_cec_send_message(msg.initiator, response, 2, VC_TRUE);
    printf("cec_callback: sent powerstatus on\n");
}

void SetStreamPath(const CECMessage& msg)
{
    debug("SetStreamPath:", msg);

    uint16_t requestedAddress;
    uint16_t physicalAddress;

    vc_cec_get_physical_address(&physicalAddress);

    requestedAddress = (msg.payload[1] << 8) + msg.payload[2];
    printf("requestedAddress: 0x%x\n", requestedAddress);

    vc_cec_send_ActiveSource(physicalAddress, VC_FALSE);
    // According to the spec. this shouldn't be necessary
    vc_cec_send_MenuStatus(msg.initiator, CEC_MENU_STATE_ACTIVATED,
            VC_TRUE);
    xbmc.ping();
}

void VendorCommandWithID(const CECMessage& msg)
{
    debug("VendorCommandWithID:", msg);
}

void debug(const char *s, const CECMessage& msg)
{
#ifdef DEBUG
    static char standard[] = "CEC Message:";
    if (s == NULL) {
        s = &standard[0];
    }

    printf("%s reason=0x%04x, length=0x%02x, retval=0x%02x, initiator=0x%x, "
        "follower=0x%x, opcode=0x%02x, operand1=0x%02x, operand2=0x%02x",
        s, msg.reason, msg.length, msg.retval, msg.initiator,
        msg.follower, msg.opcode(), msg.operand1(), msg.operand2());
    printf(" payload=[(%x%x)", msg.initiator, msg.follower);
    for (unsigned int i=0; i < msg.length; ++i) {
        printf(":%02x", msg.payload[i]);
    }
    printf("]\n");
#endif
}

void vc_cec_send_deck_status(uint8_t dest, uint8_t status)
{
    uint8_t msg[4];
    msg[0] = CEC_Opcode_DeckStatus;
    msg[1] = status;
    vc_cec_send_message( dest, msg, 2, VC_TRUE);
}

void vc_cec_report_physicalAddress(uint8_t dest)
{
    uint8_t msg[4];
    msg[0] = CEC_Opcode_ReportPhysicalAddress;
    msg[1] = (uint8_t) ((physicalAddress) >> 8 & 0xff);
    msg[2] = (uint8_t) ((physicalAddress) >> 0 & 0xff);
    if (myVendorId == CEC_VENDOR_ID_LG) {
        msg[3] = CEC_DeviceType_Rec;
    }
    else {
        msg[3] = CEC_DeviceType_Tuner;
    }
    vc_cec_send_message(dest, msg, 4, VC_TRUE);
}

void vc_cec_report_power_status(uint8_t dest, CEC_POWER_STATUS_T status)
{
    uint8_t msg[4];
    msg[0] = CEC_Opcode_ReportPowerStatus;
    msg[1] = status;
    vc_cec_send_message( dest, msg, 2, VC_FALSE);
}

void LgCecInit()
{
    uint8_t msg[4];

    vc_cec_report_power_status(CEC_TV_ADDRESS,CEC_POWER_STATUS_ON_PENDING);

    printf("Setting Vendor Id to LG\n");
    myVendorId = CEC_VENDOR_ID_LG;
    vc_cec_set_vendor_id(myVendorId);

    msg[0] = CEC_Opcode_DeviceVendorID;
    msg[1] = (uint8_t) ((myVendorId) >> 16 & 0xff);
    msg[2] = (uint8_t) ((myVendorId) >> 8 & 0xff);
    msg[3] = (uint8_t) ((myVendorId) >> 0 & 0xff);
    vc_cec_send_message(CEC_BROADCAST_ADDR, msg, 4, VC_FALSE);

    // Report Physical Address
    msg[0] = CEC_Opcode_ReportPhysicalAddress;
    msg[1] = (uint8_t) ((physicalAddress) >> 8 & 0xff);
    msg[2] = (uint8_t) ((physicalAddress) >> 0 & 0xff);
    msg[3] = CEC_DeviceType_Rec;
    vc_cec_send_message(CEC_BROADCAST_ADDR, msg, 4, VC_FALSE);

    vc_cec_report_power_status(CEC_TV_ADDRESS,CEC_POWER_STATUS_ON);

    // Opcode 04 : ImageViewOn
    msg[0] = CEC_Opcode_ImageViewOn ;
    vc_cec_send_message(CEC_TV_ADDRESS, msg, 1, VC_FALSE);

    // Active source
    msg[0] =CEC_Opcode_ActiveSource ;
    msg[1] = (uint8_t) ((physicalAddress) >> 8 & 0xff);
    msg[2] = (uint8_t) ((physicalAddress) >> 0 & 0xff);
    vc_cec_send_message(CEC_BROADCAST_ADDR, msg, 3, VC_FALSE);
}

void cec_callback(void *callback_data, uint32_t param0,
        uint32_t param1, uint32_t param2,
        uint32_t param3, uint32_t param4)
{
    CECMessage msg(param0, param1, param2, param3, param4);

    if (msg.reason == VC_CEC_TX) {
        if (msg.retval) {
            debug("cec_callback: failed transmission:", msg);
        }
        return;
    }

    debug("cec_callback: debug:", msg);

    //Actually if we have not indicated we are powered on then it should be safe
    //to not reply to any message till then
    if(myPowerState != CEC_POWER_STATUS_ON)
    {
        return;
    }

    switch (msg.opcode()) {
    case CEC_Opcode_UserControlPressed:      UserControlPressed(msg);     break;
    case CEC_Opcode_UserControlReleased:     /* NOP */                    break;
    case CEC_Opcode_MenuRequest:             MenuRequest(msg);            break;
    case CEC_Opcode_Play:                    Play(msg);                   break;
    case CEC_Opcode_DeckControl:             DeckControl(msg);            break;
    case CEC_Opcode_VendorRemoteButtonDown:  VendorRemoteButtonDown(msg); break;
    case CEC_Opcode_GiveDeviceVendorID:      GiveDeviceVendorID(msg);     break;
    case CEC_Opcode_GiveDevicePowerStatus:   GiveDevicePowerStatus(msg);  break;
    case CEC_Opcode_SetStreamPath:           SetStreamPath(msg);          break;
    case CEC_Opcode_VendorCommandWithID:     VendorCommandWithID(msg);    break;
    case CEC_Opcode_GivePhysicalAddress:     vc_cec_report_physicalAddress(msg.initiator); break;
    case CEC_Opcode_VendorCommand:           VendorCommand(msg);          break;

    case CEC_Opcode_GiveDeckStatus:
    //Status code is invalid as per CEC standard but seems to be required for LG
        if (tvVendorId == CEC_VENDOR_ID_LG) {
            vc_cec_send_deck_status(CEC_TV_ADDRESS,0x20);
        } else {
            vc_cec_send_deck_status(CEC_TV_ADDRESS,CEC_DECK_INFO_PLAY);
        }
    default:
        debug("cec_callback: unknown event:", msg);
    }

}

bool probeForTvVendorId(uint32_t& vendorId)
{
    uint32_t responses[6] = { 0, 0, 0, 0, 0, 0 };
    unsigned const int giveUp = 500;
    unsigned int i = 1;
    unsigned int n = 0;
    int res = 0;
    printf("Probing for TV vendor ID\n");
    while (n < 6 && i < giveUp) {
        printf(".");
        if (!(i % 40)) {
            printf("\n");
        }
        uint32_t response;
        res = vc_cec_get_vendor_id(CEC_AllDevices_eTV, &response);
        if ( res != 0 ) {
            printf( "An error occured when trying to get TV vendor ID\n" );
            sleep(1);
            continue;
        }
        if (response < VC_CEC_VENDOR_ID_NODEVICE &&
            response != VC_CEC_VENDOR_ID_UNKNOWN) {
            bool consistent = true;
            for (unsigned int j=0; j < n; ++j) {
                if (responses[j] != response) {
                    consistent = false;
                    break;
                }
            }
            if (consistent) {
                responses[n] = response;
                ++n;
            } else {
                responses[0] = responses[1] = responses[2] =
                responses[3] = responses[4] = responses[5] = 0;
                n = 0;
            }
        }
        usleep(100000);
        ++i;
    }
    if (i == giveUp) {
        printf(" failed.\n");
        return false;
    }
    else {
        printf(" done.\n");
        vendorId = responses[0];
        return true;
    }
}


int main(int argc, char **argv)
{
    int res = 0;

    VCHI_INSTANCE_T vchiq_instance;
    VCHI_CONNECTION_T *vchi_connection;
    CEC_AllDevices_T logical_address;


    /* Make sure logs are written to disk */
    setlinebuf(stdout);
    setlinebuf(stderr);

    printf(
            "rpi-cecd starting\n"
            "Use http://www.cec-o-matic.com to debug the payload. Remove the\n"
            "parantheses surrounding the first byte. (it's the header)\n"
          );

    // Do not call bcm_host_init() or vcos_init()
    // Otherwise we'll stop receiving CEC data

    res = vchi_initialise(&vchiq_instance);
    if ( res != VCHIQ_SUCCESS ) {
        printf("failed to open vchiq instance\n");
        return -1;
    }

    res = vchi_connect( NULL, 0, vchiq_instance );
    if ( res != 0 ) {
        printf( "VCHI connection failed\n" );
        return -1;
    }


    // vc_vchi_cec_init sets vchi_connection to NULL. Thats fine
    vc_vchi_cec_init(vchiq_instance, &vchi_connection, 1);

    physicalAddress = CEC_CLEAR_ADDR;
    while (physicalAddress == CEC_CLEAR_ADDR) {
        res = vc_cec_get_physical_address((uint16_t*)&physicalAddress);
        if (res != 0) {
            printf("failed to get physical address");
            return -1;
        }
        if (physicalAddress != CEC_CLEAR_ADDR) {
            break;
        }
        printf("CEC is currently disabled. Will retry in 60 seconds. "
               "Make sure the TV is on and connected to the RPi\n");
        sleep(60);
    }
    printf("Physical Address: 0x%x\n", physicalAddress);
    sleep(1);

    logical_address = CEC_AllDevices_eUnRegistered;
    while (logical_address == CEC_AllDevices_eUnRegistered) {
        vc_cec_get_logical_address(&logical_address);
        if (logical_address != CEC_AllDevices_eUnRegistered) {
            break;
        }
        printf("No logical address. Will retry in 60s"
               "Make sure the TV is on and connected to the RPi\n");
        vc_cec_alloc_logical_address();
        sleep(60);
    }
    printf("Logical Address: 0x%x\n", logical_address);

    while (!probeForTvVendorId(tvVendorId)) {
        printf("Probing failed. Will retry in 60 seconds. "
               "Make sure the TV is on and connected to the RPi\n");
        sleep(60);
    }
    printf("TV Vendor ID: 0x%x\n", tvVendorId);


#if 0
    // only use this for temporary debugging
    vc_cec_register_all();
#endif
    vc_cec_register_callback(((CECSERVICE_CALLBACK_T) cec_callback), NULL);
    vc_cec_register_command(CEC_Opcode_MenuRequest);
    vc_cec_register_command(CEC_Opcode_Play);
    vc_cec_register_command(CEC_Opcode_DeckControl);
    vc_cec_register_command(CEC_Opcode_GiveDeviceVendorID);
    vc_cec_register_command(CEC_Opcode_VendorCommand);
    vc_cec_register_command(CEC_Opcode_GiveDevicePowerStatus);
    vc_cec_register_command(CEC_Opcode_VendorRemoteButtonDown);
    vc_cec_register_command(CEC_Opcode_SetStreamPath);
    vc_cec_register_command(CEC_Opcode_VendorCommandWithID);


    if (tvVendorId == CEC_VENDOR_ID_LG ||
        tvVendorId == CEC_VENDOR_ID_LG_QUIRK) {
            //Do LG specific intialization
            LgCecInit();
            // only use this for temporary debugging
            vc_cec_register_all();

        } else {
        myVendorId = CEC_VENDOR_ID_BROADCOM;
        vc_cec_set_vendor_id(myVendorId);
    }

    vc_cec_send_ActiveSource(physicalAddress, 0);

    vc_cec_report_power_status(CEC_TV_ADDRESS,CEC_POWER_STATUS_ON);
    myPowerState = CEC_POWER_STATUS_ON;

    vc_cec_set_osd_name("XBMC");
    xbmc.SendHELO("rpi-cecd", ICON_NONE);

    while (1) {
        sleep(10);
    }

    vc_vchi_cec_stop();

    return 0;
}



