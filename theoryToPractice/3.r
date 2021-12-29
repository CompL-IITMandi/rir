# 2
f <- function() {
  c <- function(...) {
    print("Jello")
  }
  var <- is.vector
}
invisible(rir.compile(f))
pir.compile(f, debugFlags=pir.debugFlags(PrintEarlyPir=TRUE, PrintFinalPir=TRUE, PrintLLVM=TRUE))
