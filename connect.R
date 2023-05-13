# rir.viz("http://127.0.0.1:3011")

# g <- function(a) {
#     a * a
# }

# f <- function(a, b) {
#     res = a + b
#     g(res)
# }

# invisible(rir.compile(f))

# f(1, 2)

rir.viz("http://127.0.0.1:3011")

g <- function(a){
    -1 * a
}

f <- function(a, b=10) {
    a = g(a)
    res = a + b
    res
}

invisible(rir.compile(f))

f(10,20)
f(10,20)
f(10,20)
f(1L,20)
f(1L,20)
f(1L,2L)
f(1L,2L)