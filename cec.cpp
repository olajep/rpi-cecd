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
volatile int might_be_dimmed=0;


void button_pressed(uint32_t param)
{
    uint32_t initiator, follower, opcode, operand1, operand2;

    initiator = CEC_CB_INITIATOR(param);
    follower  = CEC_CB_FOLLOWER(param);
    opcode    = CEC_CB_OPCODE(param);
    operand1  = CEC_CB_OPERAND1(param);
    operand2  = CEC_CB_OPERAND2(param);

    if (opcode != CEC_Opcode_UserControlPressed) {
        printf("button_pressed: unknown operand operand1=0x%x: "
                "initiator=0x%x, follower=0x%x, opcode=0x%x, "
                "operand1=0x%x, operand2=0x%x\n",
                operand1, initiator, follower, opcode, operand1, operand2);
        return;
    }

    // Hack to make xbmc light up
    if (might_be_dimmed) {
        might_be_dimmed = 0;
        xbmc.SendButton(" ");
    }

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
        printf("button_pressed: operand1=0x%x has no binding\n", operand1);
    }
}

void cec_callback(void *callback_data, uint32_t param0,
        uint32_t param1, uint32_t param2,
        uint32_t param3, uint32_t param4)
{
    VC_CEC_NOTIFY_T reason;
    uint32_t len, retval;

    reason  = (VC_CEC_NOTIFY_T) CEC_CB_REASON(param0);
    len     = CEC_CB_MSG_LENGTH(param0);
    retval  = CEC_CB_RC(param0);

#ifdef DEBUG
    printf("cec_callback: debug: "
            "reason=0x%04x, len=0x%02x, retval=0x%02x, "
            "param1=0x%08x, param2=0x%08x, param3=0x%08x, param4=0x%08x\n",
            reason, len, retval, param1, param2, param3, param4);
#endif
    uint32_t opcode, operand1;
    switch (reason) {
    case VC_CEC_BUTTON_PRESSED:
        if ( len > 4 ) {
            printf("cec_callback: warning: len > 4, only using first parameter "
                    "reason=0x%04x, len=0x%02x, retval=0x%02x, "
                    "param1=0x%08x, param2=0x%08x, param3=0x%08x, param4=0x%08x\n",
                    reason, len, retval, param1, param2, param3, param4);
        }
        button_pressed(param1);
        break;
    case VC_CEC_RX:
        opcode   = CEC_CB_OPCODE(param1);
        operand1 = CEC_CB_OPERAND1(param1);
        switch (opcode) {
        case CEC_Opcode_MenuRequest:
            if (operand1 == CEC_MENU_STATE_QUERY) {
                uint8_t msg[2];
                uint32_t initiator;
                initiator = CEC_CB_INITIATOR(param1);
                msg[0] = CEC_Opcode_MenuStatus;
                msg[1] = CEC_MENU_STATE_ACTIVATED;
                vc_cec_send_message(initiator, msg, 2, VC_TRUE);
            }
            break;
        case CEC_Opcode_Play:
            if (operand1 == CEC_PLAY_FORWARD) {
                xbmc.SendButton("play");
            } else if (operand1 == CEC_PLAY_STILL) {
                xbmc.SendButton("pause");
            }
            break;
        case CEC_Opcode_DeckControl:
            if (operand1 == CEC_DECK_CTRL_STOP) {
                xbmc.SendButton("stop");
            }
        }
        break;
    case VC_CEC_BUTTON_RELEASE:
        break;
    default:
        printf("cec_callback: unknown event: "
            "reason=0x%04x, len=0x%02x, retval=0x%02x, "
            "param1=0x%08x, param2=0x%08x, param3=0x%08x, param4=0x%08x\n",
            reason, len, retval, param1, param2, param3, param4);
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

    vc_vchi_cec_init(vchiq_instance, &vchi_connection, 1);
    if ( res != 0 ) {
        printf( "VCHI CEC connection failed\n" );
        return -1;
    }

    vc_cec_register_callback(((CECSERVICE_CALLBACK_T) cec_callback), NULL);


#if 0
    vc_cec_register_all();
#endif

    vc_cec_register_command(CEC_Opcode_MenuRequest);
    vc_cec_register_command(CEC_Opcode_Play);
    vc_cec_register_command(CEC_Opcode_DeckControl);

    vc_cec_get_logical_address(&logical_address);
    printf("logical_address: 0x%x\n", logical_address);

    vc_cec_set_vendor_id(CEC_VENDOR_ID_BROADCOM);
    vc_cec_set_osd_name("XBMC");

    vc_cec_get_physical_address(&physical_address);
    printf("physical_address: 0x%x\n", physical_address);

    vc_cec_send_ActiveSource(physical_address, 0);

    xbmc.SendHELO("rpi-cecd", ICON_NONE);

    while (1) {
        might_be_dimmed = 1;
        sleep(10);
    }

    vchi_exit();

    return 0;
}

