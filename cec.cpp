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

#define SL_COMMAND_UNKNOWN_01           0x01
#define SL_COMMAND_UNKNOWN_02           0x02

#define SL_COMMAND_REQUEST_POWER_STATUS 0xa0
#define SL_COMMAND_POWER_ON             0x03
#define SL_COMMAND_CONNECT_REQUEST      0x04
#define SL_COMMAND_SET_DEVICE_MODE      0x05

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
};


CECXBMCClient xbmc;
uint32_t tvVendorId;
uint32_t myVendorId;

void UserControlPressed(uint32_t param)
{

    uint32_t operand1  = CEC_CB_OPERAND1(param);
    const char *xbmcKey = NULL;

    for (unsigned int i=0; i < CECXBMCKeymapElements; ++i) {
        if (CECXBMCKeymap[i].cec == operand1) {
            xbmcKey = CECXBMCKeymap[i].xbmc;
            break;
        }
    }

    if (xbmcKey != NULL) {
        xbmc.SendButton(xbmcKey);
    } else {
        printf("UserControlPressed: operand1=0x%x has no binding\n", operand1);
    }
}

void MenuRequest(uint32_t param)
{
    uint32_t operand1 = CEC_CB_OPERAND1(param);
    if (operand1 == CEC_MENU_STATE_QUERY) {
        uint8_t msg[2];
        uint32_t initiator;
        initiator = CEC_CB_INITIATOR(param);
        msg[0] = CEC_Opcode_MenuStatus;
        msg[1] = CEC_MENU_STATE_ACTIVATED;
        vc_cec_send_message(initiator, msg, 2, VC_TRUE);
    } else {
        printf("MenuRequest: operand1=0x%x unknown\n", operand1);
    }
}

void Play(uint32_t param) {
    uint32_t operand1 = CEC_CB_OPERAND1(param);
    switch (operand1) {
    case CEC_PLAY_FORWARD:
        xbmc.SendButton("play");
        break;
    case CEC_PLAY_STILL:
        xbmc.SendButton("pause");
        break;
    default:
        printf("Play: operand1=0x%x not implemented\n", operand1);
    }
}

void DeckControl(uint32_t param) {
    uint32_t operand1 = CEC_CB_OPERAND1(param);
    if (operand1 == CEC_DECK_CTRL_STOP) {
        xbmc.SendButton("stop");
    } else {
        printf("DeckControl: operand1=0x%x not implemented\n", operand1);
    }
}

void VendorCommand_LG(uint32_t param)
{
    uint32_t operand1 = CEC_CB_OPERAND1(param);
    uint8_t initiator = CEC_CB_INITIATOR(param);
    uint8_t msg[8];
    switch (operand1) {
    case SL_COMMAND_UNKNOWN_01:
        printf("VendorCommand_LG: SL_COMMAND_UNKNOWN_01 received\n");
        // Send 0205
        msg[0] = CEC_Opcode_VendorCommand;
        msg[1] = 0x02;
        msg[2] = 0x5;
        vc_cec_send_message(initiator, msg, 3, VC_TRUE);
        printf("VendorCommand_LG: Sent 0205\n");
        break;

    case SL_COMMAND_POWER_ON:
        printf("VendorCommand_LG: TODO HandleVendorCommandPowerOn(command)\n");
        break;

    case SL_COMMAND_CONNECT_REQUEST:
        printf("VendorCommand_LG:SL_COMMAND_CONNECT_REQUEST\n");
        // Send 0205
        // Set device mode
        msg[0] = CEC_Opcode_VendorCommand;
        msg[1] = SL_COMMAND_SET_DEVICE_MODE;
        msg[2] = CEC_DeviceType_Playback;
        vc_cec_send_message(initiator, msg, 3, VC_TRUE);
        // Transmit Image View On
        msg[0] = CEC_Opcode_ImageViewOn;
        vc_cec_send_ImageViewOn(initiator, VC_TRUE);
        //Transmit Active source
        uint16_t physicalAddress;
        vc_cec_get_physical_address(&physicalAddress);
        vc_cec_send_ActiveSource(physicalAddress, VC_FALSE);
        // Transmit menu state CEC_DEVICE_TV
        vc_cec_send_MenuStatus(initiator, CEC_MENU_STATE_ACTIVATED, VC_TRUE);
        vc_cec_set_osd_name("XBMC");
        break;

    case SL_COMMAND_REQUEST_POWER_STATUS:
        printf("VendorCommand_LG: TODO HandleVendorCommandPowerOnStatus(command) \n");
        break;

    default:
        printf("VendorCommand_LG: unhandled command operand1=0x%x\n", operand1);
    }
}

void VendorCommand(uint32_t param1)
{
    uint32_t operand1 = CEC_CB_OPERAND1(param1);
    if (myVendorId == CEC_VENDOR_ID_LG) {
        VendorCommand_LG(param1);
    } else {
        printf("VendorCommand: unhandled vendor command operand1=0x%x "
                "for vendor=0x%x\n", operand1, tvVendorId);
    }
}

void GiveDeviceVendorID(uint32_t param1) {
    uint8_t initiator = CEC_CB_INITIATOR(param1);
    uint8_t msg[4];
    msg[0] = CEC_Opcode_DeviceVendorID;
    msg[1] = (uint8_t) ((myVendorId >> 16) & 0xff);
    msg[2] = (uint8_t) ((myVendorId >> 8) & 0xff);
    msg[3] = (uint8_t) ((myVendorId >> 0) & 0xff);
    vc_cec_send_message(initiator, msg, 4, VC_TRUE);
}

void GiveDevicePowerStatus(uint32_t param1)
{
    uint8_t initiator = CEC_CB_INITIATOR(param1);
    printf("cec_callback: received power status query \n");
    // Send CEC_Opcode_ReportPowerStatus
    uint8_t msg[4];
    msg[0] = CEC_Opcode_ReportPowerStatus;
    msg[1] = CEC_POWER_STATUS_ON;
    vc_cec_send_message(initiator, msg, 2, VC_TRUE);
    printf("cec_callback: sent powerstatus on\n");
}

void debug(const char *s, uint32_t param0,
        uint32_t param1, uint32_t param2,
        uint32_t param3, uint32_t param4)
{
#ifdef DEBUG
    VC_CEC_NOTIFY_T reason  = (VC_CEC_NOTIFY_T) CEC_CB_REASON(param0);
    uint8_t len       = CEC_CB_MSG_LENGTH(param0);
    uint8_t retval    = CEC_CB_RC(param0);
    uint8_t initiator = CEC_CB_INITIATOR(param1);
    uint8_t follower  = CEC_CB_FOLLOWER(param1);
    uint8_t opcode    = CEC_CB_OPCODE(param1);
    uint8_t operand1  = CEC_CB_OPERAND1(param1);

    uint8_t operand2  = CEC_CB_OPERAND2(param1);

    static char standard[] = "CEC Message:";
    if (s == NULL) {
        s = &standard[0];
    }

    printf("%s reason=0x%04x, len=0x%02x, retval=0x%02x, initiator=0x%x, "
        "follower=0x%x, opcode=0x%0x, operand1=0x%0x, operand2=0x%0x",
        s, reason, len, retval, initiator, follower, opcode, operand1,
        operand2);
    if (len > 4) {
        printf(" param1=0x%08x, param2=0x%08x, param3=0x%08x, param4=0x%08x",
        param1, param2, param3, param4);
    }
    printf("\n");
#endif
}

void cec_callback(void *callback_data, uint32_t param0,
        uint32_t param1, uint32_t param2,
        uint32_t param3, uint32_t param4)
{
    VC_CEC_NOTIFY_T reason;
    uint32_t retval;

    reason  = (VC_CEC_NOTIFY_T) CEC_CB_REASON(param0);
    retval  = CEC_CB_RC(param0);

    if (reason == VC_CEC_TX) {
        if (retval) {
            debug("cec_callback: failed transmission: ",
                param0, param1, param2, param3, param4);
        }
        return;
    }

    debug("cec_callback: debug: ",
        param0, param1, param2, param3, param4);

    uint32_t opcode = CEC_CB_OPCODE(param1);
    switch (opcode) {
    case CEC_Opcode_UserControlPressed:  UserControlPressed(param1); break;
    case CEC_Opcode_UserControlReleased: /* NOP */                   break;
    case CEC_Opcode_MenuRequest:         MenuRequest(param1);        break;
    case CEC_Opcode_Play:                Play(param1);               break;
    case CEC_Opcode_DeckControl:         DeckControl(param1);        break;
    case CEC_Opcode_VendorCommand:       VendorCommand(param1);      break;
    case CEC_Opcode_GiveDeviceVendorID:  GiveDeviceVendorID(param1); break;
    case CEC_Opcode_GiveDevicePowerStatus:
            GiveDevicePowerStatus(param1); break;
    default:
        debug("cec_callback: unknown event: ",
            param0, param1, param2, param3, param4);
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
        if (response <= VC_CEC_VENDOR_ID_NODEVICE &&
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

            if (n==6) {
                if ( (responses[0] != responses[1] ||
                      responses[0] != responses[2] ||
                      responses[0] != responses[3] ||
                      responses[0] != responses[4] ||
                      responses[0] != responses[5] )) {
                    n=0;
                }
            }
        }
        usleep(10000);
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
    uint16_t physical_address;


    /* Make sure logs are written to disk */
    setlinebuf(stdout);
    setlinebuf(stderr);

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

    vc_cec_register_callback(((CECSERVICE_CALLBACK_T) cec_callback), NULL);
    vc_cec_register_command(CEC_Opcode_MenuRequest);
    vc_cec_register_command(CEC_Opcode_Play);
    vc_cec_register_command(CEC_Opcode_DeckControl);
    vc_cec_register_command(CEC_Opcode_GiveDeviceVendorID);
    vc_cec_register_command(CEC_Opcode_VendorCommand);
    vc_cec_register_command(CEC_Opcode_GiveDevicePowerStatus);

    physical_address = CEC_CLEAR_ADDR;
    while (physical_address == CEC_CLEAR_ADDR) {
        res = vc_cec_get_physical_address(&physical_address);
        if (res != 0) {
            printf("failed to get physical address");
            return -1;
        }
        if (physical_address != CEC_CLEAR_ADDR) {
            break;
        }
        printf("CEC is currently disabled. Will retry in 60 seconds. "
               "Make sure the TV is on and connected to the RPi\n");
        sleep(60);
    }
    printf("Physical Address: 0x%x\n", physical_address);
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
    vc_cec_register_all();
#endif


    if (tvVendorId == CEC_VENDOR_ID_LG) {
        printf("Setting Vendor Id to LG\n");
        myVendorId = tvVendorId;
        vc_cec_set_vendor_id(myVendorId);
        uint8_t msg[4];
        msg[0] = CEC_Opcode_DeviceVendorID;
        msg[1] = (uint8_t) ((myVendorId) >> 16 & 0xff);
        msg[2] = (uint8_t) ((myVendorId) >> 8 & 0xff);
        msg[3] = (uint8_t) ((myVendorId) >> 0 & 0xff);
        vc_cec_send_message(CEC_BROADCAST_ADDR, msg, 4, VC_FALSE);
    } else {
        myVendorId = CEC_VENDOR_ID_BROADCOM;
        vc_cec_set_vendor_id(myVendorId);
    }

    vc_cec_set_osd_name("XBMC");


    vc_cec_send_ActiveSource(physical_address, 0);

    xbmc.SendHELO("rpi-cecd", ICON_NONE);

    while (1) {
        sleep(10);
    }

    vc_vchi_cec_stop();

    return 0;
}

