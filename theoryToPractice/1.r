# # 1.1
# f <- function() {
#   var <- is.vector
#   var <- print
#   var <- matrix
# }
# invisible(rir.compile(f))
# pir.compile(f, debugFlags=pir.debugFlags(PrintEarlyPir=TRUE, PrintFinalPir=TRUE, PrintLLVM=TRUE))

# # 1.2
# print <- {
#   if (length(sys.frame) > 1)
#   assign("var",envir = sys.frame(-1) , 2)
# }
# # loading variables from the global env does not lead to reflection
# f <- function() {
#   var <- is.vector
#   bomb <- print
#   var
# }
# # invisible(rir.compile(f))
# # pir.compile(f, debugFlags=pir.debugFlags(PrintEarlyPir=TRUE, PrintFinalPir=TRUE, PrintLLVM=TRUE))

# f()
