#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <zookeeper.h>
/* ZooKeeper Znode Data Length (1MB, the max supported) */
#define ZDATALEN    1024 * 1024

static char *host_port;
static zhandle_t *zh;

char thishost[ZDATALEN];
char zoo_path[ZDATALEN];
char zzdata_buf[ZDATALEN];

int run_nodetool_cleanup() {
	fprintf(stderr, "Launching nodetool cleanup...\n\n\n");
	int ret = system("/usr/bin/nodetool cleanup");
	return(WEXITSTATUS(ret));
}

void lock_or_watch(){
	int RET1, RET2, RET3;
	int zzdata_len = ZDATALEN * sizeof(char);
	memset(zzdata_buf,'\0',ZDATALEN * sizeof(char));

	fprintf(stderr, "Attempting to set my lock...\n");
	RET1 = zoo_create( zh, zoo_path, thishost, strlen(thishost), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0);
        if (RET1 == ZOK)
        {
		fprintf(stderr, "Lock set. Safe for this host to proceed running nodetool cleanup.\n");
		RET2 = run_nodetool_cleanup();
		fprintf(stderr, "Nodetool cleanup has finished with return code: %d\n", RET2);
		fprintf(stderr, "Deleting my lock ...\n");
		RET3 = zoo_delete(zh, zoo_path, -1);
		if (RET3 != ZOK)
		{
			fprintf(stderr, "Unable to delete my lock! Return code: %d\n", RET3);
			fprintf(stderr, "Please look into this and also please manually delete the zone\n");
		}
		else
		{
			fprintf(stderr, "I successfully deleted my lock.\n");
		}
		exit(RET2);
       	 }
        else if (RET1 == ZNODEEXISTS)
 	{
		fprintf(stderr, "The following host already has the lock: ");
		RET2 = zoo_get(zh, zoo_path, 1, zzdata_buf, &zzdata_len, NULL); // set a watch
		if (RET2 != ZOK)
		{
			fprintf(stderr, "I am unable to set a watch due to return code: %d\n", RET2);
			fprintf(stderr, "Please investigate this problem and try again. Exiting....\n");
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "%s. Waiting for this host to relenquish the lock ... \n", zzdata_buf);
	}
	else
       	 {
              fprintf(stderr, "Unable to set a lock to due error: %d.\n", RET1);
		fprintf(stderr, "Please investigate this problem and try again.\n");
	       exit(EXIT_FAILURE);
        }
}

void primary_watcher(zhandle_t *zzh, int type, int state, const char *path,
             void* context)
{

	if (type == ZOO_SESSION_EVENT) 
	{
		if (state == ZOO_CONNECTING_STATE)
		{
			fprintf(stderr, "Connecting to Zookeeper ...\n");
		}

		if (state == ZOO_CONNECTED_STATE) 
		{
			fprintf(stderr, "Connected to Zookeeper.\n");
			lock_or_watch();
		} 
	}
	else if (type == ZOO_DELETED_EVENT && state == ZOO_CONNECTED_STATE)
	{
		fprintf(stderr, "The other host has relinquished the lock.");
		lock_or_watch();
	}
	else
	{
		fprintf(stderr, "Event of type %d, state %d, occurred. No action taken.\n", type, state);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 3) 
	{
		fprintf(stderr, "USAGE: %s host:port clustername\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	host_port = argv[1];

	memset(thishost,'\0',ZDATALEN * sizeof(char));
	strcpy(thishost,"UNKNOWN HOST");

	memset(zoo_path,'\0',ZDATALEN * sizeof(char));
	strcpy(zoo_path,"/RunningNodetoolCleanup_on_");
	strcat(zoo_path, argv[2]);

	gethostname(thishost, 100);

	zh = zookeeper_init(host_port, primary_watcher, 10000, 0, 0, 0);
    
	if (zh == NULL) 
	{
		fprintf(stderr, "Error connecting to ZooKeeper server[%d]!\n", errno);
		exit(EXIT_FAILURE);
	}

	pause(); /* main thread waits for interrupt while other threads due their work */

	return 0;
}

