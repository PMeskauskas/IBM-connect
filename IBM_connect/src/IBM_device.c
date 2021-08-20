#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <iotp_device.h>
#include <syslog.h>
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
    if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to initialize configuration: rc=%d", rc);
        IoTPConfig_clear(*config);
        cleanup();
    }
    IoTPConfig_setProperty(*config, "identity.orgId", argv[1]);
    IoTPConfig_setProperty(*config, "identity.typeId", argv[2]);
    IoTPConfig_setProperty(*config, "identity.deviceId", argv[3]);
    IoTPConfig_setProperty(*config, "auth.token", argv[4]);
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

void calculateMemory(float *memtotal, float *memfree)
{
    char buff[256];
    FILE *fp;
    fp = fopen("/proc/meminfo", "r");
    if(fp != NULL){
    fscanf(fp, "%s", buff);
    fscanf(fp, "%s", buff);
    *memtotal = atoi(buff)/1000.0;
    fscanf(fp, "%s", buff);
    fscanf(fp, "%s", buff);
    fscanf(fp, "%s", buff);
    *memfree = atoi(buff)/1000.0;
    }
    else
    {
        memtotal = 0;
        memfree = 0;
    }
    fclose(fp);
}

void deviceSendEvent(IoTPDevice *device)
{
    char data[256];
    int rc = 0;
    while(!interrupt){
        float memtotal = 0;
        float memfree = 0;
        calculateMemory(&memtotal, &memfree);
        sprintf(data,"{\"Memory usage\": \"%.2f MB/ %.2f MB\"}", memtotal-memfree, memtotal);
        rc = IoTPDevice_sendEvent(device,"status", data, "json", QoS0, NULL);
        syslog(LOG_INFO, "RC from sendEvent(): %d\n", rc);
        sleep(10);
        }
    }

int main(int argc, char *argv[])
{
    IoTPConfig *config = NULL;
    IoTPDevice *device = NULL;
    openlog(NULL, LOG_CONS, LOG_USER);
    
    if ( argc != 5 )
        usage();
    syslog(LOG_INFO,"orgId: %s typeId: %s deviceId: %s authToken: %s",argv[1],argv[2],argv[3],argv[4]);
    
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    
    configCreate(&config, argv);
    
    deviceCreate(&device, config);
    deviceConnect(&device, config);

    deviceSendEvent(device);

    deviceDisconnect(device, config);

    IoTPDevice_destroy(device);
    IoTPConfig_clear(config);
    closelog();
    return 0;
}

