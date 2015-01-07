#include "command.h"
#include "vtysh_config.h"
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>

#include "mk.h"

#ifndef VTY_SIMPLE_NODE

/* Configration from terminal */
DEFUN (config_terminal,
       config_terminal_cmd,
       "configure terminal",
       "Configuration from vty interface\n"
       "Configuration terminal\n")
{
	vty->node = CONFIG_NODE;
	return CMD_SUCCESS;
}

/* Enable command */
DEFUN (enable, 
       config_enable_cmd,
       "enable",
       "Turn on privileged mode command\n")
{
	/* If enable password is NULL, change to ENABLE_NODE */
	if ((host.enable == NULL && host.enable_encrypt == NULL) ||
		vty->type == VTY_FILE)
		vty->node = ENABLE_NODE;
	else
		vty->node = AUTH_ENABLE_NODE;

	return CMD_SUCCESS;
}

/* Disable command */
DEFUN (disable, 
       config_disable_cmd,
       "disable",
       "Turn off privileged mode command\n")
{
	if (vty->node == ENABLE_NODE)
		vty->node = VIEW_NODE;
	return CMD_SUCCESS;
}

/* Down vty node level. */
DEFUN (config_exit,
       config_exit_cmd,
       "exit",
       "Exit current mode and down to previous mode\n")
{
	switch (vty->node)
	{
		case VIEW_NODE:
            all_versions_finit();
			exit (0);
			break;
		case ENABLE_NODE:
            vty->node = VIEW_NODE;
			break;
		case CONFIG_NODE:
			vty->node = ENABLE_NODE;
			break;
		default:
			break;
	}
	return CMD_SUCCESS;
}

/* End of configuration. */
DEFUN (config_end,
       config_end_cmd,
       "end",
       "End current mode and change to enable mode.")
{
	switch (vty->node)
	{
		case VIEW_NODE:
		case ENABLE_NODE:
			/* Nothing to do. */
			break;
		case CONFIG_NODE:
			vty->node = ENABLE_NODE;
			break;
		default:
			break;
	}
	return CMD_SUCCESS;
}

/* Help display function for all node. */
DEFUN (config_list,
       config_list_cmd,
       "list",
       "Print command list\n")
{
	int i;
	struct cmd_node *cnode = vector_slot (cmdvec, vty->node);
	struct cmd_element *cmd;

	for (i = 0; i < vector_max (cnode->cmd_vector); i++)
		if ((cmd = vector_slot (cnode->cmd_vector, i)) != NULL)
			vty_out (vty, "  %s%s", cmd->string,
				VTY_NEWLINE);
	return CMD_SUCCESS;
}
#endif /*VTY_SIMPLE_NODE*/

DEFUN (config_quit,
	config_quit_cmd,
	"quit",
	"End the current session\n")
{
    all_versions_finit();
	exit(0);
	return CMD_SUCCESS;
}

int cmd_common_init()
{
	cmd_install_element (ENABLE_NODE, &config_quit_cmd);
    
#ifndef VTY_SIMPLE_NODE
	/* Each node's basic commands. */
	cmd_install_element (VIEW_NODE, &config_exit_cmd);
	cmd_install_element (VIEW_NODE, &config_quit_cmd);
	cmd_install_element (VIEW_NODE, &config_enable_cmd);			
	cmd_install_element (ENABLE_NODE, &config_list_cmd);
	cmd_install_element (ENABLE_NODE, &config_exit_cmd);
    cmd_install_element (ENABLE_NODE, &config_disable_cmd);
	cmd_install_element (ENABLE_NODE, &config_terminal_cmd);
    cmd_install_element (CONFIG_NODE, &config_exit_cmd);
    cmd_install_element (CONFIG_NODE, &config_end_cmd);
    cmd_install_element (CONFIG_NODE, &config_quit_cmd);
#endif /*VTY_SIMPLE_NODE*/
    
    /*    
        cmd_install_element (ENABLE_NODE, &show_version_cmd);
        cmd_install_element (ENABLE_NODE, &config_write_terminal_cmd);
        cmd_install_element (ENABLE_NODE, &config_write_file_cmd);
        cmd_install_element (ENABLE_NODE, &config_write_memory_cmd);
        cmd_install_element (ENABLE_NODE, &config_write_cmd);
        cmd_install_element (CONFIG_NODE, &show_version_cmd);
        cmd_install_element (CONFIG_NODE, &show_cpuinfo_cmd);
        cmd_install_element (CONFIG_NODE, &show_meminfo_cmd);
        cmd_install_element (CONFIG_NODE, &show_interruptsinfo_cmd);
        cmd_install_element (CONFIG_NODE, &show_modsinfo_cmd);
        cmd_install_element (CONFIG_NODE, &show_slabinfo_cmd);
        cmd_install_element (CONFIG_NODE, &show_netirq_cmd);
        cmd_install_element (CONFIG_NODE, &show_cputop_cmd);    
        cmd_install_element (CONFIG_NODE, &show_netcard_list_cmd);  
        cmd_install_element (CONFIG_NODE, &show_pciinfo_cmd);
        cmd_install_element (CONFIG_NODE, &show_vminfo_cmd);        
    
        cmd_install_element (ENABLE_NODE, &show_cpuinfo_cmd);
        cmd_install_element (ENABLE_NODE, &show_meminfo_cmd);
        cmd_install_element (ENABLE_NODE, &show_interruptsinfo_cmd);
        cmd_install_element (ENABLE_NODE, &show_modsinfo_cmd);
        cmd_install_element (ENABLE_NODE, &show_slabinfo_cmd);
        cmd_install_element (ENABLE_NODE, &show_netirq_cmd);
        cmd_install_element (ENABLE_NODE, &show_cputop_cmd);    
        cmd_install_element (ENABLE_NODE, &show_netcard_list_cmd);  
        cmd_install_element (ENABLE_NODE, &show_pciinfo_cmd);
        cmd_install_element (ENABLE_NODE, &show_vminfo_cmd);    
        cmd_install_element (ENABLE_NODE, &show_running_config_cmd);
        cmd_install_element (ENABLE_NODE, &show_startup_config_cmd);
        cmd_install_element (CONFIG_NODE, &config_write_terminal_cmd);
        cmd_install_element (CONFIG_NODE, &config_write_file_cmd);
        cmd_install_element (CONFIG_NODE, &config_write_memory_cmd);
        cmd_install_element (CONFIG_NODE, &config_write_cmd);
        cmd_install_element (CONFIG_NODE, &show_running_config_cmd);
        cmd_install_element (CONFIG_NODE, &show_startup_config_cmd);
        cmd_install_element (CONFIG_NODE, &enable_password_cmd);
        cmd_install_element (CONFIG_NODE, &enable_password_text_cmd);
        cmd_install_element (CONFIG_NODE, &no_enable_password_cmd);
    */

	return 0;
}
