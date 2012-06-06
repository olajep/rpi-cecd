#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#include <curl/curl.h>


#include <interface/vmcs_host/vc_cecservice.h>

#include "Key.h"


#define VCHIQ_SUCCESS 0
extern int32_t vchi_initialise( VCHI_INSTANCE_T *instance_handle );

extern int32_t vchi_exit( void );

extern int32_t vchi_connect( VCHI_CONNECTION_T **connections,
        const uint32_t num_connections,
        VCHI_INSTANCE_T instance_handle );


volatile int might_be_dimmed=0;
CURL *curl;

size_t curl_write_nop(void *buffer, size_t size, size_t nmemb, void *userp)
{
    return size*nmemb;
}

void xbmc_sendkey(uint32_t keysym)
{
    CURLcode err;
    char url[256];

    snprintf(url, 255,
            "http://localhost:80/xbmcCmds/xbmcHttp?command=SendKey(%d)",
            keysym);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    err = curl_easy_perform(curl);
    if ( err ) {
        printf("curl error=%d \"%s\"\n", err, curl_easy_strerror(err));
    }
}

void xbmc_sendaction(uint32_t action)
{
    CURLcode err;
    char url[256];

    snprintf(url, 255,
            "http://localhost:80/xbmcCmds/xbmcHttp?command=Action(%d)",
            action);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    err = curl_easy_perform(curl);
    if ( err ) {
        printf("curl error=%d \"%s\"\n", err, curl_easy_strerror(err));
    }
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
        printf("button_pressed: unknown operand operand1=0x%x\n", operand1);
        printf("\tinitiator=0x%x, follower=0x%x, opcode=0x%x, " \
                "operand1=0x%x, operand2=0x%x\n",
                initiator, follower, opcode, operand1, operand2);
        return;
    }

    // Hack to make xbmc light up
    if (might_be_dimmed) {
        might_be_dimmed = 0;
        xbmc_sendkey(KEY_ASCII);
    }

    switch (operand1) {
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
            xbmc_sendaction(ACTION_SHOW_OSD);
            break;
        case CEC_User_Control_Exit:
            xbmc_sendaction(ACTION_NAV_BACK);
            break;

        case CEC_User_Control_EPG:
            xbmc_sendaction(ACTION_SHOW_PLAYLIST);
            break; 

        case CEC_User_Control_Play:
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
    VC_CEC_NOTIFY_T reason = 0;
    uint32_t len = 0;
    uint32_t retval = 0;

    reason  = CEC_CB_REASON(param0);
    len     = CEC_CB_MSG_LENGTH(param0);
    retval  = CEC_CB_RC(param0);

    /* TODO: Check len! */
    if ( reason == VC_CEC_BUTTON_PRESSED ) {
        button_pressed(param1);
    } else if ( reason != VC_CEC_BUTTON_RELEASE ) {
        printf("Unknown event: reason=0x%04x, len=0x%02x, retval=0x%02x, " \
                "param1=0x%08x, param2=0x%08x, param3=0x%08x, param4=0x%08x\n",
                reason, len, retval, param1, param2, param3, param4);
    }
}

int main ()
{
    int res = 0;

    VCHI_INSTANCE_T vchiq_instance;
    VCHI_CONNECTION_T *vchi_connection;
    CEC_AllDevices_T logical_address;
    uint16_t physical_address;


    curl = curl_easy_init();
    if ( curl == NULL ) {
        printf("failed to init curl.\n");
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_nop);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000);


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

    curl_easy_cleanup(curl);

    return 0;
}

