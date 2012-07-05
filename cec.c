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


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#include <curl/curl.h>


#include <interface/vmcs_host/vc_cecservice.h>
#include <interface/vchiq_arm/vchiq_if.h>

#ifdef __cplusplus
}
#endif

#include "config.h"
#include "Key.h"

#ifndef VC_TRUE
#define VC_TRUE 1
#endif

volatile int might_be_dimmed=0;
int port = 80;

size_t curl_write_nop(void *buffer, size_t size, size_t nmemb, void *userp)
{
    return size*nmemb;
}

void curl_get(char *url)
{
    CURL *curl;
    CURLcode err;

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_nop);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000);
    err = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if ( err ) {
        printf("curl_get: error=%d \"%s\"\n", err, curl_easy_strerror(err));
    }
}

void xbmc_sendkey(uint32_t keysym)
{
    char url[256];

    snprintf(url, 255,
            "http://localhost:%d/xbmcCmds/xbmcHttp?command=SendKey(%d)",
            port, keysym);
    curl_get(url);
}


void xbmc_sendaction(uint32_t action)
{
    char url[256];

    snprintf(url, 255,
            "http://localhost:%d/xbmcCmds/xbmcHttp?command=Action(%d)",
            port, action);
    curl_get(url);

}

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
        xbmc_sendkey(KEY_ASCII);
    }

    switch (operand1) {
        case CEC_User_Control_Number0:
            xbmc_sendaction(REMOTE_0);
            break;
        case CEC_User_Control_Number1:
            xbmc_sendaction(REMOTE_1);
            break;
        case CEC_User_Control_Number2:
            xbmc_sendaction(REMOTE_2);
            break;
        case CEC_User_Control_Number3:
            xbmc_sendaction(REMOTE_3);
            break;
        case CEC_User_Control_Number4:
            xbmc_sendaction(REMOTE_4);
            break;
        case CEC_User_Control_Number5:
            xbmc_sendaction(REMOTE_5);
            break;
        case CEC_User_Control_Number6:
            xbmc_sendaction(REMOTE_6);
            break;
        case CEC_User_Control_Number7:
            xbmc_sendaction(REMOTE_7);
            break;
        case CEC_User_Control_Number8:
            xbmc_sendaction(REMOTE_8);
            break;
        case CEC_User_Control_Number9:
            xbmc_sendaction(REMOTE_9);
            break;
        case CEC_User_Control_Select:
            xbmc_sendaction(ACTION_SELECT_ITEM);
            break;
        case CEC_User_Control_Up:
            xbmc_sendaction(ACTION_MOVE_UP);
            break;
        case CEC_User_Control_Down:
            xbmc_sendaction(ACTION_MOVE_DOWN);
            break;
        case CEC_User_Control_Left:
            xbmc_sendaction(ACTION_MOVE_LEFT);
            break;
        case CEC_User_Control_Right:
            xbmc_sendaction(ACTION_MOVE_RIGHT);
            break;
        case CEC_User_Control_RootMenu:
            xbmc_sendaction(ACTION_PREVIOUS_MENU);
            break;
        case CEC_User_Control_SetupMenu:
        case CEC_User_Control_DisplayInformation:
            xbmc_sendaction(ACTION_CONTEXT_MENU);
            break;
        case CEC_User_Control_Exit:
            xbmc_sendaction(ACTION_NAV_BACK);
            break;

        case CEC_User_Control_EPG:
            xbmc_sendaction(ACTION_SHOW_PLAYLIST);
            break; 

        case CEC_User_Control_Play:
        case CEC_User_Control_Pause:
            xbmc_sendaction(ACTION_PLAYER_PLAYPAUSE );
            break;
        case CEC_User_Control_Stop:
            xbmc_sendaction(ACTION_STOP);
            break;
        case CEC_User_Control_Rewind:
            xbmc_sendaction(ACTION_PLAYER_REWIND);
            break;
        case CEC_User_Control_FastForward:
            xbmc_sendaction(ACTION_PLAYER_FORWARD);
            break;
        case CEC_User_Control_Forward:
            xbmc_sendaction(ACTION_FORWARD);
            break;
        case CEC_User_Control_Backward:
            xbmc_sendaction(ACTION_REWIND);
            break;

            // Colored buttons from left to right
        case CEC_User_Control_F2Red:
            xbmc_sendaction(ACTION_TELETEXT_RED);
            break;
        case CEC_User_Control_F3Green:
            xbmc_sendaction(ACTION_TELETEXT_GREEN);
            break;
        case CEC_User_Control_F4Yellow:
            xbmc_sendaction(ACTION_TELETEXT_YELLOW);
            break;
        case CEC_User_Control_F1Blue:
            xbmc_sendaction(ACTION_TELETEXT_BLUE);
            break;

        default:
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
                xbmc_sendaction(ACTION_PLAYER_PLAYPAUSE );
            } else if (operand1 == CEC_PLAY_STILL) {
                xbmc_sendaction(ACTION_PLAYER_PLAYPAUSE );
            }
            break;
        case CEC_Opcode_DeckControl:
            if (operand1 == CEC_DECK_CTRL_STOP) {
                xbmc_sendaction(ACTION_STOP);
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
    CURLcode curlerr;

    VCHI_INSTANCE_T vchiq_instance;
    VCHI_CONNECTION_T *vchi_connection;
    CEC_AllDevices_T logical_address;
    uint16_t physical_address;

    /* Make sure logs are written to disk */
    setlinebuf(stdout);
    setlinebuf(stderr);


    if (argc > 2) {
        printf("usage: %s [port]\n", argv[0]);
        return -1;
    }

    if (argc == 2) {
        port = atoi(argv[1]);
    }

    curlerr = curl_global_init(CURL_GLOBAL_NOTHING);
    if ( curlerr ) {
        printf("failed to init curl error=%d \"%s\"\n", curlerr,
                curl_easy_strerror(curlerr));
        return -1;
    }


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


    while (1) {
        might_be_dimmed = 1;
        sleep(10);
    }

    vchi_exit();

    curl_global_cleanup();

    return 0;
}

