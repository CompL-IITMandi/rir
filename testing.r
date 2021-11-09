# g <- function(b) {
#   b;
# }

# a <- 10
# f <- function() {
#   g(a);
# }

# f.serialize(f, c(15))

# invisible(rir.compile(matrix))

# matrix(10)
# matrix(10)
# matrix(10)
# matrix(c(1,2,3,4))
# matrix(c(1,2,3,4))
# matrix(c(1,2,3,4))
# matrix(c(1,2,3,4), nrow=2)
# matrix(c(1,2,3,4), nrow=2)
# matrix(c(1,2,3,4), nrow=2)

# f.serialize(matrix)

# matrix(10)


# matrix(c(1,2,3,4), ncol=2)
# matrix(c(1,2,3,4), ncol=2)
# matrix(c(1,2,3,4), ncol=2)

b <- 10

f <- function(a) {
  a + b;
}

invisible(rir.compile(f))
f(10)
f(10)
f(10)
f(10)
b <- c(1,2,3)
f(10)
f(10)
f(10)
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
f.serialize(f)
