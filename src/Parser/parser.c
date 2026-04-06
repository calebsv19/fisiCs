// SPDX-License-Identifier: Apache-2.0

#include "parser.h"
#include "parser_main.h"
#include "parser_config.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

ASTNode* parse(Parser* parser) {			// root parsing method (modify for debug)
	return parseProgram(parser);
}


