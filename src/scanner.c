#include "tree_sitter/parser.h"


const char HEREDOC_END = '>';
const char LBRACE = '{';
const char RBRACE = '}';
const char TILDE = '~';
const char DOLLAR = '$';
const char ESCAPE = '\\';
const char NEWLINE = '\n';

enum TokenType
{
    COMMAND_CONTENT,
    // Not used in the grammar, but used in the external scanner to check for error state.
    // This relies on the tree-sitter behavior that when an error is encountered the external
    // scanner is called with all symbols marked as valid.
    ERROR,
};

void* tree_sitter_wdl_external_scanner_create(void) {
    // Our scanner does not need state
    return NULL;
}

void tree_sitter_wdl_external_scanner_destroy(void* payload) { }


unsigned tree_sitter_wdl_external_scanner_serialize(void *payload, char *buffer)
{
    return 0;
}

void tree_sitter_wdl_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {}

bool tree_sitter_wdl_external_scanner_scan(
  void *payload,
  TSLexer *lexer,
  const bool *valid_symbols
) {
    if (valid_symbols[COMMAND_CONTENT] && !valid_symbols[ERROR])
    {
        bool has_content = false;
        while (lexer->lookahead)
        {
            if (lexer->lookahead == TILDE)
            {
                lexer->mark_end(lexer);
                lexer->advance(lexer, false);
                if (lexer->lookahead == LBRACE)
                {
                    if (has_content)
                    {
                        lexer->result_symbol = COMMAND_CONTENT;
                        return true;
                    }
                    else
                        return false;
                }
                else
                    has_content = true;
            }
            else if (lexer->lookahead == ESCAPE)
            {
                lexer->mark_end(lexer);
                lexer->advance(lexer, false);
                if (lexer->lookahead == NEWLINE)
                {
                    has_content = true;
                }
                else if (lexer->lookahead == TILDE
                         || lexer->lookahead == RBRACE
                         || lexer->lookahead == HEREDOC_END
                         || lexer->lookahead == ESCAPE)
                {
                    // An escapable WDL thing, \~ \} \> or double backslash \\. End any content
                    // before it; with no content before it, leave it to the escape_sequence token.
                    if (has_content)
                    {
                        lexer->result_symbol = COMMAND_CONTENT;
                        return true;
                    }
                    else
                        return false;
                }
                else
                {
                    // Any other backslash (\n, \t, \. ...) is ordinary shell text. This holds even when
                    // no content precedes it, e.g. directly after a placeholder in `~{x}\n`.
                    has_content = true;
                }
            }
            else if (lexer->lookahead == HEREDOC_END)
            {
                // we're in `command <<<` and the next character is `>` - look for
                // two more
                lexer->mark_end(lexer);
                lexer->advance(lexer, false);
                if (lexer->lookahead == HEREDOC_END)
                {
                    lexer->advance(lexer, false);
                    if (lexer->lookahead == HEREDOC_END)
                    {
                        if (has_content)
                        {
                            lexer->result_symbol = COMMAND_CONTENT;
                            return true;
                        }
                        else
                            return false;
                    }
                }
                has_content = true;
            }
            else
            {
                lexer->advance(lexer, false);
                has_content = true;
            }
        }
    }

    return false;
}
