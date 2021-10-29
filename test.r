f.loadBitcode("f_98.meta")

a <- 10
f <- function() {
  a;
}

invisible(rir.compile(f))

f()
f()
