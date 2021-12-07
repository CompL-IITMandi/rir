# 1. Matrix
# invisible(rir.compile(matrix))
# for (i in 1:10) {
#   matrix(1:9,3,3)
#   matrix(1:9,3,3)
#   matrix(1:9,3,3)
#   matrix(1:9)
#   matrix(1:9)
#   matrix(1:9)
#   matrix(1:9)
#   matrix(1:9)
#   matrix(1:9, 3)
#   matrix(1:9, 3)
#   matrix(1:9, 3)
# }
# f.serialize(matrix)

# 2. Lapply : NOT WORKING YET
x <- list(a = 1:10, beta = exp(-3:3), logic = c(TRUE,FALSE,FALSE,TRUE))
invisible(rir.compile(lapply))

lapply(x, mean)
lapply(x, mean)
lapply(x, mean)
lapply(x, mean)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)
lapply(x, mean)
lapply(x, mean)
lapply(x, mean)
lapply(x, mean)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)
lapply(x, quantile, probs = 1:3/4)

f.serialize(lapply)


# 3. substr
# invisible(rir.compile(substr))
# x <- c("asfef", "qwerty", "yuiop[", "b", "stuff.blah.yech")
# substr(x, 2, 5)
# substr(x, 2, 5)
# substr(x, 2, 5)
# substr(x, 2, 5)

# substr(x, 1:2, 2:3)
# substr(x, 1:2, 2:3)
# substr(x, 1:2, 2:3)

# f.serialize(substr)


# 4. Push Context
# f <- function() {
#   x <- "Hello World"
#   substr(x, 2, 5)
# }
# invisible(rir.compile(f))
# f()
# f()
# f()
# f()
# f.serialize(f)

# 5. Static Call
# x <- list(a = 1:10, beta = exp(-3:3), logic = c(TRUE,FALSE,FALSE,TRUE))
# invisible(rir.compile(lapply))

# lapply(x, mean)
# lapply(x, mean)
# lapply(x, mean)
# lapply(x, mean)

# 6. match.fun
# invisible(rir.compile(match.fun))
# f <- function() {
#   10;
# }
# match.fun(f)
# match.fun(f)
# match.fun(f)
# match.fun(f)
# f.serialize(match.fun)

# invisible(rir.compile(match.fun))
# f.initializeBaseLib()
