#include "command.h"
#include "cmd/mk.h"

void cmd_parse_init()
{
	cmd_common_init();
    mk_init();
	cmd_tool_init();
}
