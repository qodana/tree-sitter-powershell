// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See the LICENSE file in the project root for full license information.

#include <tree_sitter/parser.h>
#include <wctype.h>

enum TOKEN_TYPE {
    STATEMENT_TERMINATOR,
    LONE_DASH,
    DOUBLE_DASH,
    ERROR_SENTINEL
};

/* --- API --- */

void *tree_sitter_powershell_external_scanner_create();

void tree_sitter_powershell_external_scanner_destroy(void *p);

unsigned tree_sitter_powershell_external_scanner_serialize(void *payload, char *buffer);

void tree_sitter_powershell_external_scanner_deserialize(void *payload, const char *buffer, unsigned length);

bool tree_sitter_powershell_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols);

/* --- Internal Functions --- */

static void skip(TSLexer *lexer) { lexer->advance(lexer, true); }

static bool scan_statement_terminator(void *payload, TSLexer *lexer, const bool *valid_symbols)
{
    if (valid_symbols[STATEMENT_TERMINATOR]) {
        lexer->result_symbol = STATEMENT_TERMINATOR;
        // This token has no characters -- everything is lookahead to determine its existence
        lexer->mark_end(lexer);

        for (;;) {
            if (lexer->lookahead == 0) return true;
            if (lexer->lookahead == '}') return true;
            if (lexer->lookahead == ';') return true;
            if (lexer->lookahead == ')') return true;
            if (lexer->lookahead == '\n') return true;
            if (!iswspace(lexer->lookahead)) return false;
            skip(lexer);
        }
    }

    return false;
}

// Whether the character after a dash ends a command argument, so the dash is a word of its own.
static bool ends_argument(int32_t c)
{
    return c == 0 || iswspace(c) || c == ';' || c == '|' || c == ')' || c == '}';
}

// A lone `-` or `--` argument (`python -`, `git checkout -- f`). Only where the grammar expects one, never during
// error recovery (`ERROR_SENTINEL` valid), and only when what follows ends the argument: `-b` and `-1` are left alone.
static bool scan_lone_dash(TSLexer *lexer, const bool *valid_symbols)
{
    if (valid_symbols[ERROR_SENTINEL]) return false;
    if (!valid_symbols[LONE_DASH] && !valid_symbols[DOUBLE_DASH]) return false;
    if (lexer->lookahead != '-') return false;
    lexer->advance(lexer, false);
    bool dashes = false;
    if (lexer->lookahead == '-') {
        dashes = true;
        lexer->advance(lexer, false);
    }
    if (!ends_argument(lexer->lookahead)) return false;
    if (dashes ? !valid_symbols[DOUBLE_DASH] : !valid_symbols[LONE_DASH]) return false;
    lexer->mark_end(lexer);
    lexer->result_symbol = dashes ? DOUBLE_DASH : LONE_DASH;
    return true;
}

/* --- API Implementation --- */

bool tree_sitter_powershell_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols)
{
    // A dash is never a statement terminator, and a failed dash scan has moved the lexer past it.
    if (lexer->lookahead == '-') return scan_lone_dash(lexer, valid_symbols);
    return scan_statement_terminator(payload, lexer, valid_symbols);
}

void *tree_sitter_powershell_external_scanner_create()
{
    return NULL;
}

void tree_sitter_powershell_external_scanner_destroy(void *p)
{
}

unsigned tree_sitter_powershell_external_scanner_serialize(void *payload, char *buffer)
{
    return 0;
}

void tree_sitter_powershell_external_scanner_deserialize(void *payload, const char *buffer, unsigned length)
{
}
