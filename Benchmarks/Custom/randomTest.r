f <- function() { Sys.sleep(1) }


execute <- function(n) {
	for(i in n) {
		f();
	}
}
