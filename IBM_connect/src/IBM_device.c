#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>/ 
#include <iotp_device.h>
#include <syslog.h>
volatile int interrupt = 0;

void usage(void) {
    syslog(LOG_ERR, "Usage: \"orgId\" \"typeId\" \"deviceId\" \"authToken\"");
    exit(1);
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

int main(int argc, char *argv[])
{
    int useEnv = 0;
    int testCycle = 0;
    int rc = 0;
    struct rusage r_usage;
    char data[256];
    IoTPConfig *config = NULL;
    IoTPDevice *device = NULL;
    openlog(NULL, LOG_CONS, LOG_USER);
    syslog(LOG_INFO,"argv[1]: %s argv[2]: %s argv[3]: %s argv[4]: %s",argv[1],argv[2],argv[3],argv[4]);
    
    if ( argc != 5 )
        usage();
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    
    rc = IoTPConfig_create(&config, NULL);
    IoTPConfig_setProperty(config, "identity.orgId", argv[1]);
    IoTPConfig_setProperty(config, "identity.typeId", argv[2]);
    IoTPConfig_setProperty(config, "identity.deviceId", argv[3]);
    IoTPConfig_setProperty(config, "auth.token", argv[4]);
    
    if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to initialize configuration: rc=%d", rc);
        exit(1);
    }
    
    rc = IoTPDevice_create(&device, config);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to configure IoTP device: rc=%d", rc);
        usage();
        exit(1);
    }

    rc = IoTPDevice_setMQTTLogHandler(device, &MQTTTraceCallback);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to set MQTT Trace handler: rc=%d", rc);
    }

    rc = IoTPDevice_connect(device);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to connect to Watson IoT Platform: rc=%d", rc);
        syslog(LOG_ERR, "Returned error reason: %s\n", IOTPRC_toString(rc));
        exit(1);
    }
    
    char *commandName = "+";
    char *format = "+";
    
    IoTPDevice_subscribeToCommands(device, commandName, format);
    while(!interrupt)
    {
        getrusage(RUSAGE_SELF,&r_usage);
        sprintf(data,"{\"Memory usage\": \"%ld Kbs\"}", r_usage.ru_maxrss);
        rc = IoTPDevice_sendEvent(device,"status", data, "json", QoS0, NULL);
        syslog(LOG_INFO, "RC from sendEvent(): %d\n", rc);
        sleep(20);
    }
    
    rc = IoTPDevice_disconnect(device);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to disconnect from  Watson IoT Platform: rc=%d", rc);
        exit(1);
    }
    IoTPDevice_destroy(device);
    IoTPConfig_clear(config);
    closelog();
    return 0;
}

