V		?= 0
ifeq ($(V),0)
Q		:= @
NULL		:= 2>/dev/null
endif
