#ifndef SOL_CLIENT_ERR
#define SOL_CLIENT_ERR

/* Sum type for all possible error causes during a client's execution. */
enum ClientErr
{
	CLIENT_ERR_OK = 0,
	CLIENT_ERR_ALLOC,
	CLIENT_ERR_UNKNOWN_OPTION,
	CLIENT_ERR_REPEATED_P,
	CLIENT_ERR_REPEATED_H,
	CLIENT_ERR_REPEATED_F,
	CLIENT_ERR_MISSING_ARG,
};

#endif
