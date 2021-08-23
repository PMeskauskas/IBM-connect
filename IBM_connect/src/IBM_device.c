#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <iotp_device.h>
#include <syslog.h>
#include "IBM_device.h"
#include "IBM_invoke.h"
#include <libubox/blobmsg_json.h>
#include <libubus.h>
volatile int interrupt = 0;

void cleanup()
{
    closelog();
    exit(1);
}

void cleanupAll(IoTPDevice *device, IoTPConfig *config)
{
    IoTPDevice_destroy(device);
    IoTPConfig_clear(config);
    cleanup();
}

void usage(void) {
    syslog(LOG_ERR, "Usage: \"orgId\" \"typeId\" \"deviceId\" \"authToken\"");
    cleanup();
}

void sigHandler(int signo) {
    signal(SIGINT, NULL);
    syslog(LOG_INFO, "Received signal: %d", signo);
    interrupt = 1;
}

void MQTTTraceCallback (int level, char * message)
{
    if ( level > 0 )
        syslog(LOG_INFO, "%s", message ? message:"NULL");
}

void configCreate(IoTPConfig **config, char *argv[])
{
    int rc = 0;
    rc = IoTPConfig_create(config, NULL);
    CheckConfig(rc,*config);
    rc = IoTPConfig_setProperty(*config, "identity.orgId", argv[1]);
    CheckConfig(rc,*config);
    rc = IoTPConfig_setProperty(*config, "identity.typeId", argv[2]);
    CheckConfig(rc,*config);
    rc = IoTPConfig_setProperty(*config, "identity.deviceId", argv[3]);
    CheckConfig(rc,*config);
    rc = IoTPConfig_setProperty(*config, "auth.token", argv[4]);
    CheckConfig(rc,*config);
}
void CheckConfig(int rc, IoTPConfig *config){
     if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to initialize configuration: rc=%d", rc);
        IoTPConfig_clear(config);
        cleanup();
    }
}
void deviceCreate(IoTPDevice **device, IoTPConfig *config)
{
    int rc = 0;
    rc = IoTPDevice_create(device, config);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to configure IoTP device: rc=%d", rc);
        cleanupAll(*device, config);
    }
}

void deviceConnect(IoTPDevice **device, IoTPConfig *config)
{
    int rc = 0;
    rc = IoTPDevice_connect(*device);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to connect to Watson IoT Platform: rc=%d", rc);
        syslog(LOG_ERR, "Returned error reason: %s\n", IOTPRC_toString(rc));
        cleanupAll(*device, config);
    }
}

void deviceDisconnect(IoTPDevice *device, IoTPConfig *config)
{
    int rc = 0;
    rc = IoTPDevice_disconnect(device);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to disconnect from  Watson IoT Platform: rc=%d", rc);
        cleanupAll(device, config);
    }
}
void deviceSendEventloop(IoTPDevice *device, IoTPConfig *config){
    int rc = 0;
    struct ubus_context *ctx;
    struct memoryData memory={0, 0};
    if(connectUbus(&ctx) != 0){
        syslog(LOG_ERR, "Failed to connect to ubus");
        cleanupAll(device, config);
    }
    while(!interrupt){
        if(getMemoryDataFromUbus(&ctx,&memory) != 0){
            syslog(LOG_ERR, "Failed to get data from ubus");
            cleanupAll(device, config);
        }
        deviceSendEvent(device, memory);
        memset(&memory, 0, sizeof(memory));
        sleep(10);
    }
    ubus_free(ctx);
}
void deviceSendEvent(IoTPDevice *device, struct memoryData memory)
{
    char data[255];
    int rc = 0;
    sprintf(data,"{\"Memory usage\": \"%0.2f MB / %0.2f MB\"}", ((memory.totalMemory-memory.freeMemory)/1000000.0), memory.totalMemory/1000000.0);
    rc = IoTPDevice_sendEvent(device,"status", data, "json", QoS0, NULL);
    syslog(LOG_INFO, "RC from sendEvent(): %d\n", rc);
}

int main(int argc, char *argv[])
{
    IoTPConfig *config = NULL;
    IoTPDevice *device = NULL;
    openlog(NULL, LOG_CONS, LOG_USER);
    syslog(LOG_INFO,"orgId: %s typeId: %s deviceId: %s authToken: %s",argv[1],argv[2],argv[3],argv[4]);
    
    if ( argc != 5 )
        usage();
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    
    configCreate(&config, argv);
    
    deviceCreate(&device, config);
    deviceConnect(&device, config);

    deviceSendEventloop(device, config);

    deviceDisconnect(device, config);

    cleanupAll(device, config);
    return 0;
}