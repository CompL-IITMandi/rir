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

fun <- function() {
    1
}

foo <- function() {
    1
    fun()
}
bar <- function() {
    1
    fun()
}

g <- function(a){
    -1 * a
    foo()
}

h <- function(a){
    -1 * a
    bar()
}

i <- function(a){
    -1 * a
    foo()
}

j <- function(a){
    -1 * a
    bar()
}

k <- function(a){
    -1 * a
    fun()
}

f <- function(a, b=10) {
    g(a)
    h(a)
    i(a)
    j(a)
    k(a)
}

invisible(rir.compile(f))

f(10,20)
