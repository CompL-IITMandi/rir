rir.viz("http://127.0.0.1:3011")

j <- function(a) {
    a*a
}

h <- function(a) {
    a*j(8)
}

g <- function(a){
    j(-1 * h(a))
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
