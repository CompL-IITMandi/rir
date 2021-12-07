# f.loadBitcode("matrix_11761.meta")
# invisible(rir.compile(matrix))
# start.time <- Sys.time()
# matrix(1:9,3,3)
# for (i in 1:1000) {
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
# end.time <- Sys.time()
# time.taken <- end.time - start.time
# print(paste("execution time: ",time.taken))

# x <- list(a = 1:10, beta = exp(-3:3), logic = c(TRUE,FALSE,FALSE,TRUE))
# compute the list mean for each list element
# f.loadBitcode("lapply_6959.meta")
# invisible(rir.compile(lapply))
# lapply(x, mean)
# lapply(x, mean)
# lapply(x, mean)
# lapply(x, mean)
# lapply(x, mean)
# lapply(x, mean)

# start.time <- Sys.time()
# f.loadBitcode("matrix_11761.meta")
# f.loadBitcode("rnorm_1893.meta")
# invisible(rir.compile(matrix))
# invisible(rir.compile(rnorm))
# invisible(matrix(rnorm(3600), nrow=6))
# invisible(matrix(rnorm(3600), nrow=6))
# invisible(matrix(rnorm(3600), nrow=6))
# invisible(matrix(rnorm(3600), nrow=6))

# invisible(matrix(rnorm(3600), ncol=6))
# invisible(matrix(rnorm(3600), ncol=6))
# invisible(matrix(rnorm(3600), ncol=6))
# invisible(matrix(rnorm(3600), ncol=6))
# end.time <- Sys.time()
# time.taken <- end.time - start.time
# print(paste("execution time: ",time.taken))

# f.loadBitcode("substr_7966.meta")
# start.time <- Sys.time()
# invisible(rir.compile(substr))
# x <- c("asfef", "qwerty", "yuiop[", "b", "stuff.blah.yech")
# for (i in 1:100000) {
#   substr(x, 2, 5)
#   substr(x, 2, 5)
#   substr(x, 2, 5)
#   substr(x, 2, 5)

#   substr(x, 1:2, 2:3)
#   substr(x, 1:2, 2:3)
#   substr(x, 1:2, 2:3)
# }
# end.time <- Sys.time()
# time.taken <- end.time - start.time
# print(paste("execution time: ",time.taken))

fun <- function(num) {
  recurse_fibonacci <- function(n) {
    if(n <= 1) {
      return(n)
    } else {
      return(recurse_fibonacci(n-1) + recurse_fibonacci(n-2))
    }
  }
  recurse_fibonacci(num)
}

fun(10)
fun(10)
fun(10)
fun(10)
f.serialize(fun)
