rir.viz("http://127.0.0.1:3011")


g <- 10L
f <- function(a) {
    g * a
}

invisible(rir.compile(f))
# rir.disassemble(f)
f(10L)
# f(10L)
# f(10L)
# g <- 10
# f(10L)

# source -> RIR Bytecode ... PIR ... LLVM
# source -> RIR Bytecode ... PIR ... bytecode
