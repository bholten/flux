# Embed a source file as a C byte array.
#
# Usage:
#   cmake -DINPUT=<src> -DOUTPUT=<dst> -DSYMBOL=<varname> -P embed_lib.cmake
#
# Emits a header declaring:
#   unsigned char <SYMBOL>[] = { 0xNN, 0xNN, ... };
#   unsigned int <SYMBOL>_len = N;
#
# Symbol-compatible with `xxd -i` — same variable names and types — so
# we drop the xxd build dependency without touching the C consumers.
# We don't bother matching xxd's 12-bytes-per-line wrap; the file is
# gitignored and only the compiler reads it.

if(NOT INPUT OR NOT OUTPUT OR NOT SYMBOL)
  message(FATAL_ERROR "embed_lib.cmake requires -DINPUT=<src> -DOUTPUT=<dst> -DSYMBOL=<name>")
endif()

file(READ "${INPUT}" HEX_CONTENT HEX)
string(LENGTH "${HEX_CONTENT}" HEX_LEN)
math(EXPR BYTE_LEN "${HEX_LEN} / 2")

# Turn "3b3b20..." into "0x3b, 0x3b, 0x20, ..." (one long line — the
# generated header is gitignored and only consumed by the C compiler,
# so we don't bother wrapping it for human readability).
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " C_BYTES "${HEX_CONTENT}")

file(WRITE "${OUTPUT}"
"unsigned char ${SYMBOL}[] = {
  ${C_BYTES}
};
unsigned int ${SYMBOL}_len = ${BYTE_LEN};
")
