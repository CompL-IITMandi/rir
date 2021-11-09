f.loadBitcode("f_367.meta")

b <- 10

f <- function(a) {
  a + b;
}

invisible(rir.compile(f))
f(10)
# f(10)
# f(10)
# f(10)
b <- c(1,2,3)
f(10)
# f(10)
# f(10)



# f(c(10,11))
# f(c(10,11))
# f(c(10,11))
# f(10)
# f(10)
# f(10)
# f(10)
# f(10,2)
# f(10,2)
# f(10,2)
# f(10,2)
# f.serialize(f)
