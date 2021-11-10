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

# invisible(rir.compile(matrix))
# invisible(rir.compile(rnorm))
# invisible(matrix(rnorm(3600), nrow=6))
# invisible(matrix(rnorm(3600), nrow=6))
# invisible(matrix(rnorm(3600), nrow=6))
# invisible(matrix(rnorm(3600), nrow=6))

# invisible(matrix(rnorm(3600)))
# invisible(matrix(rnorm(3600)))
# invisible(matrix(rnorm(3600)))
# invisible(matrix(rnorm(3600)))
# f.serialize(matrix)
# f.serialize(rnorm)


# TODO: Promises within promises

x <- list(a = 1:10, beta = exp(-3:3), logic = c(TRUE,FALSE,FALSE,TRUE))
invisible(rir.compile(lapply))

lapply(x, mean)
lapply(x, mean)
lapply(x, mean)
lapply(x, mean)

f.serialize(lapply)

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
