# ------------------------------------------------------------------
# The Computer Language Shootout
# http://shootout.alioth.debian.org/
#
# Contributed by Leo Osvald
# ------------------------------------------------------------------

lim <- 2
iter <- 50

mandelbrot <- function(size) {
    sum = 0
    byteAcc = 0
    bitNum  = 0

    y = 0

    while (y < size) {
      ci = (2.0 * y / size) - 1.0
      x = 0

      while (x < size) {
        zr   = 0.0
        zrzr = 0.0
        zi   = 0.0
        zizi = 0.0
        cr = (2.0 * x / size) - 1.5

        z = 0
        notDone = TRUE
        escape = 0
        while (notDone && (z < 50)) {
          zr = zrzr - zizi + cr
          zi = 2.0 * zr * zi + ci

          # preserve recalculation
          zrzr = zr * zr
          zizi = zi * zi

          if ((zrzr + zizi) > 4.0) {
            notDone = FALSE
            escape  = 1
          }
          z = z + 1
        }

        byteAcc = bitwShiftL(byteAcc, 1) + escape

        bitNum = bitNum + 1

        # Code is very similar for these cases, but using separate blocks
        # ensures we skip the shifting when it's unnecessary, which is most cases.
        if (bitNum == 8) {
          sum = bitwXor(sum, byteAcc)
          byteAcc = 0
          bitNum  = 0
        } else if (x == (size - 1)) {
          byteAcc = bitwShiftL(byteAcc, 8 - bitNum)
          sum = bitwXor(sum, byteAcc)
          byteAcc = 0
          bitNum  = 0
        }
        x = x + 1
      }
      y = y + 1
    }
    return (sum);
}

# mandelbrot becomes a static site, how to handle that?

# execute <- function(n = 3000L) {
#     mandelbrot(n)
# }

# invisible(rir.compile(bitwShiftL))
# invisible(rir.compile(bitwXor))

# f.printHAST(bitwShiftL);
# f.printHAST(bitwXor);

# invisible(rir.compile(mandelbrot))
# mandelbrot(10)
# mandelbrot(10)
# mandelbrot(10)
# mandelbrot(10)
# mandelbrot(10)
# mandelbrot(10)
# mandelbrot(10)
# mandelbrot(10)
# mandelbrot(10)

# execute(10)
# execute(10)
# execute()
# execute()

# f.serialize(mandelbrot, c(4296032527))
# f.serialize(bitwShiftL, c(12888097295, 4296032271, 8592065039, 15))
# f.serialize(print, c(1099512693007))
# f.serialize(bitwXor, c(8660238351, 204521487, 12888096783, 15))

# invisible(rir.compile(mandelbrot))
# invisible(rir.compile(bitwShiftL))
# invisible(rir.compile(print))
# invisible(rir.compile(bitwXor))

# mandelbrot(10)
# mandelbrot(10)
# mandelbrot(10)
# mandelbrot(10)
# mandelbrot(10)

start.time <- Sys.time()

mandelbrot(300)
# mandelbrot(100)
# mandelbrot(100)
# mandelbrot(100)
# mandelbrot(100)
# mandelbrot(100)
# mandelbrot(100)
# mandelbrot(100)
# mandelbrot(100)
# mandelbrot(100)
# mandelbrot(100)
# mandelbrot(100)

end.time <- Sys.time()
time.taken <- end.time - start.time
print(paste("execution time: ",time.taken))
