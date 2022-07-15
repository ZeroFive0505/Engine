#pragma once

#ifdef _DEBUG
#define ASSERT(expression) assert(expression)
#endif // _DEBUG

#define SAFE_DELETE(x) \
if (x)	\
{	\
	delete x;	\
	x = nullptr;	\
}

#define SAFE_DELETE_ARRAY(x) \
if(x) \
{	\
	delete[] x;	\
	x = nullptr;	\
}

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif