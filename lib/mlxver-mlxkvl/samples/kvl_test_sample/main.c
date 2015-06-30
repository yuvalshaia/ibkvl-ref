#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>

// mlxkvlincludes
#include <vl.h>
#include <vl_os.h>
#include <kvl.h>
#include <kvl_ksync.h>

#include "common.h"


MODULE_LICENSE("GPL");                     // Get rid of taint message by declaring code as GPL.
MODULE_AUTHOR("SaeedM");                   // Who wrote this module?
MODULE_DESCRIPTION("mlxkvl test example"); // What does this module do?

#define MODULE_NAME TEST_MODULE_NAME
//module parameters example
static int mlnx_command=0;
module_param_named(command, mlnx_command, int, 0644);
MODULE_PARM_DESC(command, "Enable debug tracing if > 0");

// print info about current registerd operation
int user_run(void * kvldata, void *data,void* userbuff,unsigned int len)
{
	struct user_st* udata = NULL;
	if (len != sizeof(struct user_st)) {
		print_err("len [%d] != [%ld] sizeof(struct user_st)\n ",len,sizeof(struct user_st));
		return -1;
	}
	udata = (struct user_st*)userbuff;
	print_info("\nInteger = [%d]\nstr = [%s]\n ",udata->integ,udata->str);
	

	return 0;
}

// print info about current registerd operation
int sync_server(void * kvldata, void *data,void* userbuff,unsigned int len)
{
	//struct kvl_op* op = (struct kvl_op*)kvldata;
	char buffer[100]={0};
	char mydata[100]="I AM SERVER :helloooooooooo";
	sync_sock_t* sock;

	sock = kvl_connect_sync("",true,10);
	if (!sock) {
		print_err( "server : sock create failed\n");
		return 1;
	}
	print_info( "server : sock create passed\n");	
	if(kvl_ksync(sock,buffer,mydata,100)){
		print_err( "server : ssync with client failed\n");
	}

	printk("SERVER : RECVD from client: %s\n",buffer);
	kvl_close_sync(sock);
	return 0;
}

// print info about current registerd operation
int sync_client(void * kvldata, void *data,void* userbuff,unsigned int len)
{
	struct kvl_op* op = (struct kvl_op*)kvldata;
	char buffer[100]={0};
	char mydata[100]="I AM Client : Who Are you ?";
	sync_sock_t* sock;
	char *server_ip = get_param_strval(op,"server_ip");
	sock = kvl_connect_sync(server_ip,false,10);
	if (!sock) {
		print_err("client :sock create failed\n");
		return 1;
	}
	print_info( "client : sock create passed\n");	
	if(kvl_ksync(sock,buffer,mydata,100)){
		print_err( "client : sync with client failed\n");
	}

	printk("CLIENT : RECVD from server: %s\n",buffer);
	kvl_close_sync(sock);
	return 0;
}

int kvltest_sleep(void* kvldata, void *datav,void* userbuff,unsigned int len){
   struct kvl_op* op = (struct kvl_op*)kvldata;
   int sleep = get_param_intval(op,"sleep");
   VL_DATA_TRACE(("sleeping for %d sec ...\n",sleep));
   VL_sleep(sleep);
   return 0;
}

struct kvl_op* sleep_op;
struct kvl_op* sync_client_op;
struct kvl_op* sync_server_op;
struct kvl_op* user_op;

static int init_hello(void)
{
   sleep_op = create_kvlop("kvltest_sleep","sleep test",MODULE_NAME,kvltest_sleep,NULL,NULL);
   add_int_param(sleep_op,"sleep","time to sleep",2);

   sync_client_op = create_kvlop("sync_client","sync_client",MODULE_NAME,sync_client,NULL,NULL);
   add_str_param(sync_client_op,"server_ip","sync_server ip","127.0.0.1");
   sync_server_op = create_kvlop("sync_server","sync_server",MODULE_NAME,sync_server,NULL,NULL);
   user_op = create_kvlop("user_run","user_run",MODULE_NAME,user_run,NULL,"user_run");

   printk(KERN_ALERT "mlxkvl test was loaded\n");
   return 0;
}


static void cleanup_hello(void)
{
   destroy_kvlop(sleep_op);
   destroy_kvlop(sync_client_op);
   destroy_kvlop(sync_server_op);
   destroy_kvlop(user_op);

   printk(KERN_ALERT "mlxkvl test was unloaded\n");
}


module_init(init_hello);
module_exit(cleanup_hello);



