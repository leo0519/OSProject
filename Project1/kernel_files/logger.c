#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
static char *message = NULL;
module_param(message, charp, S_IRUGO|S_IWUSR);
static int __init print_log(void){
  printk(KERN_DEBUG "%s", message);
	return 0;
}
static void __exit exit_log(void){}
module_init(print_log);
module_exit(exit_log);
