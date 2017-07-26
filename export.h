#ifndef EXPORT_H
#define EXPORT_H

#ifdef _MSC_VER 
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#endif // EXPORT_H
