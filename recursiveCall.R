f <- function(num) { # HAST: 3276
  if (num == 1) {
    return (1)
  } else {
    return ( num * f(num-1) )
  }
}

f(5)
f(5)
f(5)
f(5)
f(5)
f(5)

f(c(1,5))


f.serialize(f)
